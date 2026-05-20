#include "hud_renderer.h"
#include "objloader.h"
#include "projection.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdio>
#include <unordered_map>


void HudRenderer::loadPersonModel(const std::string& obj_path) {
    std::vector<glm::vec3> vertices, surface_normal;
    std::vector<glm::vec2> uvs;

    loadOBJ(obj_path.c_str(), vertices, uvs, surface_normal);

    if (vertices.empty() || surface_normal.empty()) {
        printf("[ERROR] invalid person model\n");
        return;
    }

    person_vertex_count = (int)vertices.size();

    glGenVertexArrays(1, &vao_person_model);
    glBindVertexArray(vao_person_model);

    glGenBuffers(1, &vbo_person_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_person_vertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vbo_person_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_person_normals);
    glBufferData(GL_ARRAY_BUFFER, surface_normal.size() * sizeof(glm::vec3), surface_normal.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    printf("[OK] Model person LOADED: %d vertices\n", person_vertex_count);
}


void HudRenderer::drawPersonModel(const glm::vec3& position, float yaw_radians, float scale, float r, float g, float b, float alpha) {

    glm::mat4 model_matrix(1.f);
    model_matrix = glm::translate(model_matrix, position);
    model_matrix = glm::rotate(model_matrix, yaw_radians, glm::vec3(0, 1, 0));
    model_matrix = glm::rotate(model_matrix, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    model_matrix = glm::scale(model_matrix, glm::vec3(scale));

    glm::mat4 mvp = Projection::buildMVP(viewport_width, viewport_height) * model_matrix;
    glm::mat3 normal_mat = glm::transpose(glm::inverse(glm::mat3(model_matrix)));

    glUseProgram(program_model);

    glUniformMatrix4fv(glGetUniformLocation(program_model, "uMVP"),       1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(program_model, "uModel"),     1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniformMatrix3fv(glGetUniformLocation(program_model, "uNormalMat"), 1, GL_FALSE, glm::value_ptr(normal_mat));
    glUniform4f(glGetUniformLocation(program_model, "uColor"), r, g, b, alpha);
    glUniform3f(glGetUniformLocation(program_model, "uLightDir"), 0.3f, 1.0f, 0.5f);
    glUniform3f(glGetUniformLocation(program_model, "uViewPos"),
        Projection::CAMERA_POSITION.x, Projection::CAMERA_POSITION.y, Projection::CAMERA_POSITION.z);
    glUniform1f(glGetUniformLocation(program_model, "uTime"), elapsed_time);

    if (vao_person_model == 0 || person_vertex_count <= 0) return;

    glBindVertexArray(vao_person_model);
    glDrawArrays(GL_TRIANGLES, 0, person_vertex_count);
    glBindVertexArray(0);
}


void HudRenderer::drawPersons(const std::list<Object>& persons) {

    static std::unordered_map<int, glm::vec3> smoothed_positions;

    for (const Object& person : persons) {
        if (person.label != "person") continue;

        if (!person.has_world_coords) continue;

        glm::vec3 raw_pos = Projection::ipmToWorld(person.world_x_m, person.world_z_m);


        if (raw_pos.z < 1.0f || raw_pos.z > Projection::MAX_OBJECT_Z) continue;

        glm::vec3 world_pos = raw_pos;

        if (person.track_id >= 0) {
            auto it = smoothed_positions.find(person.track_id);
            if (it != smoothed_positions.end()) {
                world_pos = it->second * 0.8f + raw_pos * 0.2f;
                it->second = world_pos;
            } else {
                smoothed_positions[person.track_id] = raw_pos;
                world_pos = raw_pos;
            }
        }

        float scale = 0.01f;
        float alpha = 1.0f - (world_pos.z / Projection::MAX_OBJECT_Z);
        if (alpha < 0.25f) alpha = 0.25f;

        drawPersonModel(world_pos, 0.0f, scale, 0.9f, 0.9f, 1.0f, alpha);
    }

    std::unordered_map<int, glm::vec3> active_positions;
    for (const Object& person : persons) {
        if (person.label == "person" && person.track_id >= 0) {
            auto it = smoothed_positions.find(person.track_id);
            if (it != smoothed_positions.end()) {
                active_positions[person.track_id] = it->second;
            }
        }
    }
    smoothed_positions = std::move(active_positions);
}