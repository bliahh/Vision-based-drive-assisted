
#include "hud_renderer.h"
#include "projection.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

using namespace Projection;

static void addQuad(
    std::vector<float>& out,
    glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
    glm::vec3 normal,
    float u0, float v0, float u1, float v1)
{
    out.push_back(a.x); out.push_back(a.y); out.push_back(a.z);
    out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
    out.push_back(u0); out.push_back(v0);

    out.push_back(b.x); out.push_back(b.y); out.push_back(b.z);
    out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
    out.push_back(u1); out.push_back(v0);

    out.push_back(c.x); out.push_back(c.y); out.push_back(c.z);
    out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
    out.push_back(u0); out.push_back(v1);

    out.push_back(b.x); out.push_back(b.y); out.push_back(b.z);
    out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
    out.push_back(u1); out.push_back(v0);

    out.push_back(d.x); out.push_back(d.y); out.push_back(d.z);
    out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
    out.push_back(u1); out.push_back(v1);

    out.push_back(c.x); out.push_back(c.y); out.push_back(c.z);
    out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
    out.push_back(u0); out.push_back(v1);
}

void HudRenderer::drawBuildings() {
    float road_edge = LANE_WIDTH * 2.5f + 3.0f;
    float bw  = 4.0f;
    float bd  = 6.0f;
    float gap = 2.0f;

    std::vector<float> all_verts;

    for (float z = 2.0f; z < ROAD_Z_END; z += bd + gap) {
        float h = 4.0f + fmodf(z * 7.3f, 8.0f);
        float zn = z;
        float zf = z + bd;

        for (float side : {1.0f, -1.0f}) {
            float inner = side * road_edge;
            float outer = inner + side * bw;
            glm::vec3 side_normal = {-side, 0.0f, 0.0f};
            glm::vec3 front_normal = {0.0f, 0.0f, -1.0f};

            addQuad(all_verts,
                {inner, 0, zn}, {inner, 0, zf},
                {inner, h, zn}, {inner, h, zf},
                side_normal,
                0, 0, 1, 1);

            addQuad(all_verts,
                {inner, 0, zn}, {outer, 0, zn},
                {inner, h, zn}, {outer, h, zn},
                front_normal,
                0, 0, 1, 1);
        }
    }

    GLuint tmp_vao, tmp_vbo;
    glGenVertexArrays(1, &tmp_vao);
    glGenBuffers(1, &tmp_vbo);
    glBindVertexArray(tmp_vao);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glm::mat4 mvp = buildMVP(viewport_width, viewport_height);
    glm::mat4 model_matrix(1.0f);

    glUseProgram(program_building);
    glUniformMatrix4fv(glGetUniformLocation(program_building, "uMVP"),   1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(program_building, "uModel"), 1, GL_FALSE, glm::value_ptr(model_matrix));
    glUniform3f(glGetUniformLocation(program_building, "lightPos"), 0.0f, 10.0f, 30.0f);
    glUniform3f(glGetUniformLocation(program_building, "viewPos"),
        CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);
    glUniform1i(glGetUniformLocation(program_building, "uFogEnabled"), fog_enabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(program_building, "uFogDensity"), fog_density);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_facade_color);
    glUniform1i(glGetUniformLocation(program_building, "texture1"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_facade_normal);
    glUniform1i(glGetUniformLocation(program_building, "normalMapTex"), 1);

    glBufferData(GL_ARRAY_BUFFER, all_verts.size()*sizeof(float), all_verts.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(all_verts.size() / 8));

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &tmp_vbo);
    glDeleteVertexArrays(1, &tmp_vao);
}