#include "hud_renderer.h"
#include "projection.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

using namespace Projection;


/// @brief contruieste un dreptunghi plat (o fata a unei cladiri)
/// @param out vector iesire
/// @param a colt sx-jos
/// @param b colt dx-jos
/// @param c colt sx-sus
/// @param d colt dx-sus
/// @param normal
/// @param u0 colt sx sus tex
/// @param v0 colt sx sus tex
/// @param u1
/// @param v1
static void addQuad(std::vector<float>& out, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 normal, float u0, float v0, float u1, float v1) {

    out.push_back(a.x);
    out.push_back(a.y);
    out.push_back(a.z);
    out.push_back(normal.x);
    out.push_back(normal.y);
    out.push_back(normal.z);
    out.push_back(u0);
    out.push_back(v0);

    out.push_back(b.x);
    out.push_back(b.y);
    out.push_back(b.z);
    out.push_back(normal.x);
    out.push_back(normal.y);
    out.push_back(normal.z);
    out.push_back(u1);
    out.push_back(v0);

    out.push_back(c.x);
    out.push_back(c.y);
    out.push_back(c.z);
    out.push_back(normal.x);
    out.push_back(normal.y);
    out.push_back(normal.z);
    out.push_back(u0);
    out.push_back(v1);

    out.push_back(b.x);
    out.push_back(b.y);
    out.push_back(b.z);
    out.push_back(normal.x);
    out.push_back(normal.y);
    out.push_back(normal.z);
    out.push_back(u1);
    out.push_back(v0);

    out.push_back(d.x);
    out.push_back(d.y);
    out.push_back(d.z);
    out.push_back(normal.x);
    out.push_back(normal.y);
    out.push_back(normal.z);
    out.push_back(u1);
    out.push_back(v1);

    out.push_back(c.x);
    out.push_back(c.y);
    out.push_back(c.z);
    out.push_back(normal.x);
    out.push_back(normal.y);
    out.push_back(normal.z);
    out.push_back(u0);
    out.push_back(v1);
}


/// @brief construieste cladiri procedural
void HudRenderer::drawBuildings() {

    // distanta centru-cladire
    float building_start = LANE_WIDTH * 2.5f + 3.0f;
    //latimea cladirii
    float building_width = 4.0f;
    float building_depth = 6.0f;
    //distanta dintre cladiri
    float building_gap = 2.0f;
    float building_step = building_depth + building_gap;

    std::vector<float> all_building_vertices;

    for (float z_position = 2.0f; z_position < ROAD_Z_END; z_position += building_step) {

        float building_height = 4.0f + fmodf(z_position * 7.3f, 8.0f);
        float z_front = z_position;
        float z_back = z_position + building_depth;

        for (float side : {1.0f, -1.0f}) {

            // cord lui x pt fiecare fata
            float wall_inner = side * building_start;
            float wall_outer = wall_inner + side * building_width;

            //normalele pentru vertex
            glm::vec3 lateral_normal = {-side, 0.0f, 0.0f};
            glm::vec3 frontal_normal = {0.0f, 0.0f, -1.0f};

            // perete lateral
            addQuad(all_building_vertices,
                {wall_inner, 0, z_front},// jos fata
                {wall_inner, 0, z_back}, // jos spate
                {wall_inner, building_height, z_front}, // sus fata
                {wall_inner, building_height, z_back},  // sus spate
                lateral_normal,
                0, 0, 1, 1);

            // perete frontal
            addQuad(all_building_vertices,
                {wall_inner, 0, z_front}, // jos interior
                {wall_outer, 0, z_front}, // jos exterior
                {wall_inner, building_height, z_front}, // sus interior
                {wall_outer, building_height, z_front}, // sus exterior
                frontal_normal,
                0, 0, 1, 1);
        }
    }

    GLuint building_vao, building_vbo;
    glGenVertexArrays(1, &building_vao);
    glGenBuffers(1, &building_vbo);
    glBindVertexArray(building_vao);
    glBindBuffer(GL_ARRAY_BUFFER, building_vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glm::mat4 mvp_matrix = buildMVP(viewport_width, viewport_height);
    glm::mat4 model_matrix(1.0f);

    glUseProgram(program_building);

    GLint loc_mvp = glGetUniformLocation(program_building, "uMVP");
    GLint loc_model = glGetUniformLocation(program_building, "uModel");
    GLint loc_light_pos = glGetUniformLocation(program_building, "lightPos");
    GLint loc_view_pos = glGetUniformLocation(program_building, "viewPos");
    GLint loc_fog_enabled = glGetUniformLocation(program_building, "uFogEnabled");
    GLint loc_fog_density = glGetUniformLocation(program_building, "uFogDensity");
    GLint loc_texture1 = glGetUniformLocation(program_building, "texture1");
    GLint loc_normal_map = glGetUniformLocation(program_building, "normalMapTex");

    glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp_matrix));
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniform3f(loc_light_pos, 0.0f, 10.0f, 30.0f);
    glUniform3f(loc_view_pos, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);
    glUniform1i(loc_fog_enabled, fog_enabled ? 1 : 0);
    glUniform1f(loc_fog_density, fog_density);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_facade_color);
    glUniform1i(loc_texture1, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_facade_normal);
    glUniform1i(loc_normal_map, 1);

    int total_vertices = (int)(all_building_vertices.size() / 8);
    glBufferData(GL_ARRAY_BUFFER, all_building_vertices.size() * sizeof(float), all_building_vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, total_vertices);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &building_vbo);
    glDeleteVertexArrays(1, &building_vao);
}
