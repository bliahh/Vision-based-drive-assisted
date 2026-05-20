#include "hud_renderer.h"
#include "projection.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdlib>
#include <chrono>

using namespace Projection;

void HudRenderer::loadRain(int count) {
    std::vector<float> points;
    srand(123);
    for (int i = 0; i < count; i++) {
        float x = ((rand() % 600) - 300) / 10.f;
        float y = (rand() % 300) / 10.f;
        float z = (rand() % 800) / 10.f;
        points.push_back(x);
        points.push_back(y);
        points.push_back(z);
       
        points.push_back(x);
        points.push_back(y - 0.15f);  
        points.push_back(z);
    }
    rain_count = count * 2;
    glGenVertexArrays(1, &vao_rain);
    glBindVertexArray(vao_rain);
    glGenBuffers(1, &vbo_rain);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_rain);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void HudRenderer::drawRain() {
    if (!rain_enabled || rain_count == 0) return;

    static auto start = std::chrono::steady_clock::now();
    float time = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();

    glm::mat4 mvp = buildMVP(viewport_width, viewport_height);
    glUseProgram(program_rain);
    glUniformMatrix4fv(glGetUniformLocation(program_rain, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(glGetUniformLocation(program_rain, "uTime"), time);

    glLineWidth(1.5f);
    glBindVertexArray(vao_rain);
    glDrawArrays(GL_LINES, 0, rain_count);
    glBindVertexArray(0);
}