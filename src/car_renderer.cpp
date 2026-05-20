#include "hud_renderer.h"
#include "projection.h"
#include "objloader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <unordered_map>
#include <cstdio>
#include <cmath>

using namespace Projection;


void HudRenderer::loadCarModel(const std::string& obj_path) {
    std::vector<glm::vec3> vertices, normals;
    std::vector<glm::vec2> uvs;

    if (!loadOBJ(obj_path.c_str(), vertices, uvs, normals)) {
        printf("[WARN] Model negasit: %s\n", obj_path.c_str());
        car_model_loaded = false;
        return;
    }

    car_vertex_count = (int)vertices.size();

    glGenVertexArrays(1, &vao_car_model);
    glBindVertexArray(vao_car_model);

    glGenBuffers(1, &vbo_car_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_car_vertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vbo_car_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_car_normals);
    glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    car_model_loaded = true;
    printf("[OK] Model masina: %d vertecsi\n", car_vertex_count);
}


void HudRenderer::drawCarModel(const glm::vec3& position, float yaw, float scale, float r, float g, float b, float alpha) {
    if (!car_model_loaded || car_vertex_count <= 0) return;

    glm::mat4 model_matrix(1.f);
    model_matrix = glm::translate(model_matrix, position);
    model_matrix = glm::rotate(model_matrix, yaw, glm::vec3(0,1,0));
    model_matrix = glm::scale(model_matrix, glm::vec3(scale));

    glm::mat4 mvp = buildMVP(viewport_width, viewport_height) * model_matrix;
    glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));

    glUseProgram(program_model);
    glUniformMatrix4fv(glGetUniformLocation(program_model, "uMVP"),       1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(program_model, "uModel"),     1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniformMatrix3fv(glGetUniformLocation(program_model, "uNormalMat"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
    glUniform4f(glGetUniformLocation(program_model, "uColor"), r, g, b, alpha);
    glUniform3f(glGetUniformLocation(program_model, "uLightDir"), 0.3f, 1.0f, 0.5f);
    glUniform3f(glGetUniformLocation(program_model, "uViewPos"), CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);
    glUniform1f(glGetUniformLocation(program_model, "uTime"), elapsed_time);
    glUniform1i(glGetUniformLocation(program_model, "uFogEnabled"), fog_enabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(program_model, "uFogDensity"), fog_density);

    glBindVertexArray(vao_car_model);
    glDrawArrays(GL_TRIANGLES, 0, car_vertex_count);
    glBindVertexArray(0);
}


struct LaneCandidate {
    const Object* object;
    glm::vec3 position;
    float distance;
};


static void drawLaneCars(
    HudRenderer& renderer,
    std::vector<LaneCandidate>& candidates,
    int max_count,
    float r, float g, float b)
{
    std::sort(candidates.begin(), candidates.end(),
        [](const LaneCandidate& a, const LaneCandidate& b) {
            return a.distance < b.distance;
        });

    int drawn = 0;
    for (LaneCandidate& c : candidates) {
        if (drawn >= max_count) break;

        float alpha = 1.0f - (c.distance / Projection::MAX_OBJECT_Z);
        if (alpha < 0.2f) alpha = 0.2f;

        renderer.drawCarModel(c.position, 0.f, 1.0f, r, g, b, alpha);
        drawn++;
    }
}


void HudRenderer::drawCars(const std::list<Object>& cars) {
    if (!layer_enabled[LAYER_CARS] || !corridor_is_valid) return;

    static std::unordered_map<int, glm::vec3> smoothed;

    float half_lane = LANE_WIDTH * 0.5f;
    float lane_limit = LANE_WIDTH * 1.5f;

    std::vector<LaneCandidate> ego_cars;
    std::vector<LaneCandidate> left_cars;
    std::vector<LaneCandidate> right_cars;

    for (const Object& car : cars) {
        if (car.label == "person") continue;
        if (!car.has_world_coords) continue;

        glm::vec3 raw_position = ipmToWorld(car.world_x_m, car.world_z_m);

        if (raw_position.z < 3.0f || raw_position.z > MAX_OBJECT_Z) continue;

        if (fabsf(raw_position.x) > lane_limit) continue;

        glm::vec3 final_position = raw_position;

        if (car.track_id >= 0) {
            auto it = smoothed.find(car.track_id);
            if (it != smoothed.end()) {
                glm::vec3 old = it->second;
                final_position.x = old.x*0.7f + raw_position.x*0.3f;
                final_position.z = old.z*0.7f + raw_position.z*0.3f;
                it->second = final_position;
            } else {
                smoothed[car.track_id] = raw_position;
            }
        }

        LaneCandidate candidate = {&car, final_position, final_position.z};

        float x = final_position.x;
        if (fabsf(x) <= half_lane) {
            ego_cars.push_back(candidate);
        } else if (x < -half_lane && x >= -lane_limit) {
            left_cars.push_back(candidate);
        } else if (x > half_lane && x <= lane_limit) {
            right_cars.push_back(candidate);
        }
    }

    drawLaneCars(*this, ego_cars,   1, 0.0f, 1.0f, 0.5f);
    drawLaneCars(*this, left_cars,  1, 1.0f, 0.8f, 0.0f);
    drawLaneCars(*this, right_cars, 1, 1.0f, 0.8f, 0.0f);

    std::unordered_map<int, glm::vec3> active;
    for (const Object& car : cars) {
        if (car.label == "person" || car.track_id < 0) continue;
        auto it = smoothed.find(car.track_id);
        if (it != smoothed.end()) {
            active[car.track_id] = it->second;
        }
    }
    smoothed = std::move(active);
}


void HudRenderer::drawEgoCar() {
    glm::vec3 ego_position = {0.f, 0.f, 1.2f};
    drawCarModel(ego_position, 0.f, 1.0f, 0.15f, 0.18f, 0.22f, 0.95f);
}