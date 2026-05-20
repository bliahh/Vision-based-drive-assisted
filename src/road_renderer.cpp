#include "hud_renderer.h"
#include "projection.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

using namespace Projection;

void HudRenderer::drawRoad() {
    float half_w = 14.f;
    float depth  = 120.f;
    float y      = -0.02f;

    float tile_u = 4.f;
    float tile_v = 8.f;

    std::vector<float> road_verts = {
        -half_w, y, 0.f,         0,1,0,          0.f,    0.f,
         half_w, y, 0.f,         0,1,0,          tile_u, 0.f,
        -half_w, y, depth,       0,1,0,          0.f,    tile_v,
         half_w, y, depth,       0,1,0,          tile_u, tile_v,
    };

    glBindVertexArray(vao_road);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_road);
    glBufferData(GL_ARRAY_BUFFER, road_verts.size()*sizeof(float), road_verts.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glm::mat4 mvp = buildMVP(viewport_width, viewport_height);
    glm::mat4 model_matrix(1.f);

    glUseProgram(program_road_nm);

    glUniformMatrix4fv(glGetUniformLocation(program_road_nm, "uMVP"),   1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(program_road_nm, "uModel"), 1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniform3f(glGetUniformLocation(program_road_nm, "lightPos"),  0.f, 10.f, 30.f);
    glUniform3f(glGetUniformLocation(program_road_nm, "viewPos"),   CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_road_color);
    glUniform1i(glGetUniformLocation(program_road_nm, "texture1"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_road_normal);
    glUniform1i(glGetUniformLocation(program_road_nm, "normalMapTex"), 1);


    glUniform1i(glGetUniformLocation(program_road_nm, "uFogEnabled"), fog_enabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(program_road_nm, "uFogDensity"), fog_density);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    glBindVertexArray(vao_road);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_road);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(4*sizeof(float)));
    glBindVertexArray(0);
}


void HudRenderer::drawDrivableArea(const PolyVec& polygons) {
    if (!layer_enabled[LAYER_DRIVABLE_AREA]) return;

    for (const Polyline& polygon : polygons) {
        if (polygon.size() < 3) continue;

        std::vector<float> vertices;
        vertices.reserve(polygon.size() * 4);

        for (const Point& point : polygon) {
            glm::vec3 wp = ipmToWorld(point[0], point[1]);
            vertices.push_back(wp.x);
            vertices.push_back(wp.y + 0.01f);
            vertices.push_back(wp.z);
            vertices.push_back(0.3f);
        }

        drawTriangleStrip(vao_lane, vbo_lane, vertices, 0.f, 0.5f, 0.35f, 0.08f);
    }
}



void HudRenderer::drawTrotuar(){
    float trotuar_y = 0.12f;
    float trotuar_width = 2.0f;
    float road_edge = LANE_WIDTH * 2.5f;
    float depth = 120.f;

    std::vector<float> trotuar_left = {
        -road_edge - trotuar_width, trotuar_y, 0.f,     1.f, 0.f, 0.f,
        -road_edge,                 trotuar_y, 0.f,     1.f, 1.f, 0.f,
        -road_edge - trotuar_width, trotuar_y, depth,   1.f, 0.f, 8.f,
        -road_edge,                 trotuar_y, depth,   1.f, 1.f, 8.f,
    };

    std::vector<float> trotuar_right = {
        road_edge,                  trotuar_y, 0.f,     1.f, 0.f, 0.f,
        road_edge + trotuar_width,  trotuar_y, 0.f,     1.f, 1.f, 0.f,
        road_edge,                  trotuar_y, depth,   1.f, 0.f, 8.f,
        road_edge + trotuar_width,  trotuar_y, depth,   1.f, 1.f, 8.f,
    };

    std::vector<float> bordura_left = {
        -road_edge, -0.02f, 0.f,    1.f, 0.f, 0.f,
        -road_edge, trotuar_y, 0.f,  1.f, 1.f, 0.f,
        -road_edge, -0.02f, depth,   1.f, 0.f, 8.f,
        -road_edge, trotuar_y, depth, 1.f, 1.f, 8.f,
    };

    std::vector<float> bordura_right = {
        road_edge, -0.02f, 0.f,     1.f, 0.f, 0.f,
        road_edge, trotuar_y, 0.f,   1.f, 1.f, 0.f,
        road_edge, -0.02f, depth,    1.f, 0.f, 8.f,
        road_edge, trotuar_y, depth,  1.f, 1.f, 8.f,
    };

    glUseProgram(program_road);


    glUniform3f(glGetUniformLocation(program_road, "uViewPos"), CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);
glUniform1i(glGetUniformLocation(program_road, "uFogEnabled"), fog_enabled ? 1 : 0);
glUniform1f(glGetUniformLocation(program_road, "uFogDensity"), fog_density);
    glUniformMatrix4fv(glGetUniformLocation(program_road, "uMVP"), 1, GL_FALSE, glm::value_ptr(buildMVP(viewport_width, viewport_height)));
    glUniform4f(glGetUniformLocation(program_road, "uColor"), 0.7f, 0.7f, 0.7f, 1.f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, road_texture);
    glUniform1i(glGetUniformLocation(program_road, "uTex"), 0);

    glBindVertexArray(vao_road);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_road);

    glBufferData(GL_ARRAY_BUFFER, trotuar_left.size()*sizeof(float), trotuar_left.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBufferData(GL_ARRAY_BUFFER, trotuar_right.size()*sizeof(float), trotuar_right.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUniform4f(glGetUniformLocation(program_road, "uColor"), 0.4f, 0.4f, 0.4f, 1.f);

    glBufferData(GL_ARRAY_BUFFER, bordura_left.size()*sizeof(float), bordura_left.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBufferData(GL_ARRAY_BUFFER, bordura_right.size()*sizeof(float), bordura_right.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

}