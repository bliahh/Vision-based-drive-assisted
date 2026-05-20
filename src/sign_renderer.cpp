#include "hud_renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <cmath>

enum SignCategory { SIGN_FORB, SIGN_MAND, SIGN_PRIO, SIGN_WARN, SIGN_INFO, SIGN_UNKNOWN };

static SignCategory getCategory(const std::string& label) {
    if (label.rfind("forb_", 0) == 0) return SIGN_FORB;
    if (label.rfind("mand_", 0) == 0) return SIGN_MAND;
    if (label.rfind("prio_", 0) == 0) return SIGN_PRIO;
    if (label.rfind("warn_", 0) == 0) return SIGN_WARN;
    if (label.rfind("info_", 0) == 0) return SIGN_INFO;
    return SIGN_UNKNOWN;
}

static std::vector<float> makeCircle(float cx, float cy, float r, int seg) {
    std::vector<float> pts;
    for (int i = 0; i <= seg; i++) {
        float a = 6.28318f * i / seg;
        pts.push_back(cx + cosf(a)*r);
        pts.push_back(cy + sinf(a)*r);
    }
    return pts;
}

static std::vector<float> makeCircleFan(float cx, float cy, float r, int seg) {
    std::vector<float> pts;
    pts.push_back(cx);
    pts.push_back(cy);
    for (int i = 0; i <= seg; i++) {
        float a = 6.28318f * i / seg;
        pts.push_back(cx + cosf(a)*r);
        pts.push_back(cy + sinf(a)*r);
    }
    return pts;
}

void HudRenderer::drawSigns(const std::list<Object>& signs) {
    if (!layer_enabled[LAYER_SIGNS]) return;
    if (signs.empty()) return;

    glDisable(GL_DEPTH_TEST);

    float w = (float)viewport_width;
    float h = (float)viewport_height;

    glm::mat4 ortho(1.f);
    ortho[0][0] =  2.f/w;
    ortho[1][1] = -2.f/h;
    ortho[3][0] = -1.f;
    ortho[3][1] =  1.f;
    ortho[3][3] =  1.f;

    glUseProgram(program_hud);
    glUniformMatrix4fv(glGetUniformLocation(program_hud, "uProj"), 1, GL_FALSE, glm::value_ptr(ortho));
    glBindVertexArray(vao_hud);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_hud);

    float start_x = 25.f;
    float start_y = 80.f;
    float size = 18.f;
    float gap = 48.f;
    int drawn = 0;
    int max_signs = 5;

    for (const Object& sign : signs) {
        if (drawn >= max_signs) break;

        float cx = start_x + size;
        float cy = start_y + drawn*gap + size;
        SignCategory cat = getCategory(sign.label);
        bool is_stop = (sign.label == "prio_stop");

        float br=0, bg=0, bb=0;       
        float cr=0, cg=1, cb=0.53f;  
        switch (cat) {
            case SIGN_FORB: cr=1.0f; cg=0.15f; cb=0.15f; br=0.3f; bg=0.02f; bb=0.02f; break;
            case SIGN_MAND: cr=0.2f; cg=0.5f;  cb=1.0f;  br=0.02f; bg=0.05f; bb=0.2f; break;
            case SIGN_PRIO: cr=1.0f; cg=0.85f; cb=0.0f;  br=0.2f; bg=0.15f; bb=0.0f;  break;
            case SIGN_WARN: cr=1.0f; cg=0.8f;  cb=0.0f;  br=0.2f; bg=0.15f; bb=0.0f;  break;
            case SIGN_INFO: cr=0.2f; cg=0.7f;  cb=1.0f;  br=0.02f; bg=0.08f; bb=0.2f; break;
            default:        cr=0.5f; cg=0.5f;  cb=0.5f;  br=0.1f; bg=0.1f; bb=0.1f;   break;
        }

        std::vector<float> bg_shape;
        std::vector<float> outline;

        if (is_stop) {
            float r = size * 1.1f;
            bg_shape.push_back(cx); bg_shape.push_back(cy);
            for (int i = 0; i <= 8; i++) {
                float a = 6.28318f * i / 8.f - 6.28318f/16.f;
                bg_shape.push_back(cx + cosf(a)*r);
                bg_shape.push_back(cy + sinf(a)*r);
                outline.push_back(cx + cosf(a)*r);
                outline.push_back(cy + sinf(a)*r);
            }
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.6f, 0.0f, 0.0f, 0.9f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(bg_shape.size()/2));
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(2.5f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)(outline.size()/2));

        } else if (cat == SIGN_FORB) {
            float r = size * 1.0f;
            bg_shape = makeCircleFan(cx, cy, r, 16);
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), br, bg, bb, 0.85f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(bg_shape.size()/2));
            outline = makeCircle(cx, cy, r, 16);
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(3.f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), cr, cg, cb, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)(outline.size()/2));
            float d = r * 0.7f;
            std::vector<float> bar = {cx-d, cy-d, cx+d, cy+d};
            glBufferData(GL_ARRAY_BUFFER, bar.size()*sizeof(float), bar.data(), GL_STREAM_DRAW);
            glLineWidth(3.f);
            glDrawArrays(GL_LINES, 0, 2);

        } else if (cat == SIGN_WARN) {
            float r = size * 1.1f;
            float tx = cx, ty = cy - r;
            float lx = cx - r*0.9f, ly = cy + r*0.6f;
            float rx = cx + r*0.9f, ry = cy + r*0.6f;
            bg_shape = {cx, cy, tx, ty, lx, ly, rx, ry};
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), br, bg, bb, 0.85f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            outline = {tx, ty, lx, ly, rx, ry};
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(2.5f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), cr, cg, cb, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
            std::vector<float> excl = {cx, cy-r*0.35f, cx, cy+r*0.1f};
            glBufferData(GL_ARRAY_BUFFER, excl.size()*sizeof(float), excl.data(), GL_STREAM_DRAW);
            glLineWidth(2.5f);
            glDrawArrays(GL_LINES, 0, 2);
            std::vector<float> dot = makeCircleFan(cx, cy+r*0.3f, 2.f, 6);
            glBufferData(GL_ARRAY_BUFFER, dot.size()*sizeof(float), dot.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(dot.size()/2));

        } else if (cat == SIGN_PRIO) {
            float r = size * 1.1f;
            bg_shape = {cx, cy, cx, cy-r, cx+r*0.7f, cy, cx, cy+r, cx-r*0.7f, cy};
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), br, bg, bb, 0.85f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 5);
            outline = {cx, cy-r, cx+r*0.7f, cy, cx, cy+r, cx-r*0.7f, cy};
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(2.5f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), cr, cg, cb, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 4);

        } else if (cat == SIGN_MAND) {
            float r = size * 1.0f;
            bg_shape = makeCircleFan(cx, cy, r, 16);
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.1f, 0.2f, 0.6f, 0.9f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(bg_shape.size()/2));
            outline = makeCircle(cx, cy, r, 16);
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(2.f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.4f, 0.7f, 1.0f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)(outline.size()/2));            float aw = size*0.25f;
            std::vector<float> arrow = {
                cx, cy-r*0.6f, cx, cy+r*0.3f,           
                cx, cy-r*0.6f, cx-aw, cy-r*0.2f,        
                cx, cy-r*0.6f, cx+aw, cy-r*0.2f,        
            };
            glBufferData(GL_ARRAY_BUFFER, arrow.size()*sizeof(float), arrow.data(), GL_STREAM_DRAW);
            glLineWidth(2.5f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_LINES, 0, 6);

        } else if (cat == SIGN_INFO) {
            float r = size * 0.9f;
            bg_shape = {cx, cy, cx-r, cy-r, cx+r, cy-r, cx+r, cy+r, cx-r, cy+r};
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.02f, 0.1f, 0.3f, 0.9f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 5);
            outline = {cx-r, cy-r, cx+r, cy-r, cx+r, cy+r, cx-r, cy+r};
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(2.f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.3f, 0.7f, 1.0f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            std::vector<float> letter = {cx, cy-r*0.1f, cx, cy+r*0.5f};
            glBufferData(GL_ARRAY_BUFFER, letter.size()*sizeof(float), letter.data(), GL_STREAM_DRAW);
            glLineWidth(2.5f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_LINES, 0, 2);
            std::vector<float> dot = makeCircleFan(cx, cy-r*0.35f, 2.5f, 6);
            glBufferData(GL_ARRAY_BUFFER, dot.size()*sizeof(float), dot.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(dot.size()/2));

        } else {
            float r = size * 1.0f;
            bg_shape = makeCircleFan(cx, cy, r, 12);
            glBufferData(GL_ARRAY_BUFFER, bg_shape.size()*sizeof(float), bg_shape.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.1f, 0.1f, 0.1f, 0.8f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)(bg_shape.size()/2));
            outline = makeCircle(cx, cy, r, 12);
            glBufferData(GL_ARRAY_BUFFER, outline.size()*sizeof(float), outline.data(), GL_STREAM_DRAW);
            glLineWidth(2.f);
            glUniform4f(glGetUniformLocation(program_hud, "uColor"), 0.5f, 0.5f, 0.5f, 1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)(outline.size()/2));
        }

        drawn++;
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}