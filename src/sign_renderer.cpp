#include "hud_renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <cmath>

enum SignCategory { SIGN_FORB, SIGN_MAND, SIGN_WARN, SIGN_INFO, SIGN_UNKNOWN };


static SignCategory getCategory(const std::string& label) {

    if (label.rfind("forb_", 0) == 0) return SIGN_FORB;
    if (label.rfind("mand_", 0) == 0) return SIGN_MAND;
    if (label.rfind("warn_", 0) == 0) return SIGN_WARN;
    if (label.rfind("info_", 0) == 0) return SIGN_INFO;
    return SIGN_UNKNOWN;
}


static std::vector<float> makeCircle(float cx, float cy, float r, int seg) {

    std::vector<float> pts;

    for (int i = 0; i <= seg; i++) {
        float a = 6.28318f * i / seg;
        pts.push_back(cx + cosf(a) * r);
        pts.push_back(cy + sinf(a) * r);
    }

    return pts;
}


static std::vector<float> makeCircleFan(float cx, float cy, float r, int seg) {

    std::vector<float> pts = {cx, cy};

    for (int i = 0; i <= seg; i++) {
        float a = 6.28318f * i / seg;
        pts.push_back(cx + cosf(a) * r);
        pts.push_back(cy + sinf(a) * r);
    }

    return pts;
}


static void uploadToGPU(GLuint vbo, const std::vector<float>& vertices) {

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
}


static void drawFilledShape(GLuint vbo, GLuint shader, const std::vector<float>& vertices, float r, float g, float b, float a) {

    uploadToGPU(vbo, vertices);
    glUniform4f(glGetUniformLocation(shader, "uColor"), r, g, b, a);
    glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(vertices.size() / 2));
}


static void drawOutline(GLuint vbo, GLuint shader, const std::vector<float>& vertices, float r, float g, float b, float a, float lw) {

    uploadToGPU(vbo, vertices);
    glUniform4f(glGetUniformLocation(shader, "uColor"), r, g, b, a);
    glLineWidth(lw);
    glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)(vertices.size() / 2));
}


static void drawLineSegments(GLuint vbo, GLuint shader, const std::vector<float>& vertices, float r, float g, float b, float a, float lw) {

    uploadToGPU(vbo, vertices);
    glUniform4f(glGetUniformLocation(shader, "uColor"), r, g, b, a);
    glLineWidth(lw);
    glDrawArrays(GL_LINES, 0, (GLsizei)(vertices.size() / 2));
}


void HudRenderer::drawSigns(const std::list<Object>& signs) {

    if (!layer_enabled[LAYER_SIGNS] || signs.empty()) return;

    glDisable(GL_DEPTH_TEST);

    float screen_width = (float)viewport_width, screen_height = (float)viewport_height;

    glm::mat4 ortho(1.f);
    ortho[0][0] =  2.f / screen_width;
    ortho[1][1] = -2.f / screen_height;
    ortho[3][0] = -1.f;
    ortho[3][1] =  1.f;
    ortho[3][3] =  1.f;

    glUseProgram(program_hud);
    glUniformMatrix4fv(glGetUniformLocation(program_hud, "uProj"), 1, GL_FALSE, glm::value_ptr(ortho));
    glBindVertexArray(vao_hud);

    float icon_x = 25.f, icon_y = 80.f, icon_size = 18.f, icon_gap = 48.f;
    int drawn_count = 0;

    for (const Object& sign : signs) {

        if (drawn_count >= 5) break;

        float cx = icon_x + icon_size;
        float cy = icon_y + drawn_count * icon_gap + icon_size;
        float radius = icon_size * 1.1f;
        SignCategory category = getCategory(sign.label);

        if (category == SIGN_FORB) {

            drawFilledShape(vbo_hud, program_hud, makeCircleFan(cx, cy, radius, 16), 0.3f, 0.02f, 0.02f, 0.85f);
            drawOutline(vbo_hud, program_hud, makeCircle(cx, cy, radius, 16), 1.f, 0.15f, 0.15f, 1.f, 3.f);
            float d = radius * 0.7f;
            drawLineSegments(vbo_hud, program_hud, {cx - d, cy - d, cx + d, cy + d}, 1.f, 0.15f, 0.15f, 1.f, 3.f);

        } else if (category == SIGN_WARN) {

            float tx = cx, ty = cy - radius;
            float lx = cx - radius * 0.9f, ly = cy + radius * 0.6f;
            float rx = cx + radius * 0.9f, ry = cy + radius * 0.6f;

            drawFilledShape(vbo_hud, program_hud, {cx, cy, tx, ty, lx, ly, rx, ry}, 0.2f, 0.15f, 0.f, 0.85f);
            drawOutline(vbo_hud, program_hud, {tx, ty, lx, ly, rx, ry}, 1.f, 0.8f, 0.f, 1.f, 2.5f);
            drawLineSegments(vbo_hud, program_hud, {cx, cy - radius * 0.35f, cx, cy + radius * 0.1f}, 1.f, 1.f, 1.f, 1.f, 2.5f);
            drawFilledShape(vbo_hud, program_hud, makeCircleFan(cx, cy + radius * 0.3f, 2.f, 6), 1.f, 1.f, 1.f, 1.f);

        } else if (category == SIGN_INFO) {

            drawFilledShape(vbo_hud, program_hud, {cx, cy, cx - radius, cy - radius, cx + radius, cy - radius, cx + radius, cy + radius, cx - radius, cy + radius}, 0.02f, 0.1f, 0.3f, 0.9f);
            drawOutline(vbo_hud, program_hud, {cx - radius, cy - radius, cx + radius, cy - radius, cx + radius, cy + radius, cx - radius, cy + radius}, 0.3f, 0.7f, 1.f, 1.f, 2.f);
            drawLineSegments(vbo_hud, program_hud, {cx, cy - radius * 0.1f, cx, cy + radius * 0.5f}, 1.f, 1.f, 1.f, 1.f, 2.5f);
            drawFilledShape(vbo_hud, program_hud, makeCircleFan(cx, cy - radius * 0.35f, 2.5f, 6), 1.f, 1.f, 1.f, 1.f);

        } else if (category == SIGN_MAND) {

            drawFilledShape(vbo_hud, program_hud, makeCircleFan(cx, cy, radius, 16), 0.1f, 0.2f, 0.6f, 0.9f);
            drawOutline(vbo_hud, program_hud, makeCircle(cx, cy, radius, 16), 0.4f, 0.7f, 1.f, 1.f, 2.f);
            float aw = icon_size * 0.25f;
            drawLineSegments(vbo_hud, program_hud, {
                cx, cy - radius * 0.6f, cx, cy + radius * 0.3f,
                cx, cy - radius * 0.6f, cx - aw, cy - radius * 0.2f,
                cx, cy - radius * 0.6f, cx + aw, cy - radius * 0.2f
            }, 1.f, 1.f, 1.f, 1.f, 2.5f);

        } else {

            drawFilledShape(vbo_hud, program_hud, makeCircleFan(cx, cy, radius, 12), 0.1f, 0.1f, 0.1f, 0.8f);
            drawOutline(vbo_hud, program_hud, makeCircle(cx, cy, radius, 12), 0.5f, 0.5f, 0.5f, 1.f, 2.f);
        }

        drawn_count++;
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
