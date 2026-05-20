#include "hud_renderer.h"
#include "projection.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <projection.h>

using namespace Projection;



void HudRenderer::loadStars(int count) {
    std::vector<glm::vec3> points;
    srand(42); 

    for (int i = 0; i < count; i++) {
        float x = ((rand() % 4000) - 2000) / 10.f; 

        float y = ((rand() % 1000) + 100)  / 10.f;  
        float z = ((rand() % 5000) + 1000) / 10.f;  
        
        points.push_back({x, y, z});
    }

    star_count = count;
    glGenVertexArrays(1, &vao_stars);
    glBindVertexArray(vao_stars);
    glGenBuffers(1, &vbo_stars);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_stars);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    stars_loaded = true;
}



void HudRenderer::drawStars() {
    if (!stars_loaded) return;

    printf("[STARS] drawing count=%d program=%d time=%.2f\n", star_count, program_stars, elapsed_time);

    static auto start = std::chrono::steady_clock::now();
    elapsed_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();

    glUseProgram(program_stars);

    GLint mvpLoc   = glGetUniformLocation(program_stars, "uMVP");
    GLint colorLoc = glGetUniformLocation(program_stars, "uColor");
    GLint timeLoc  = glGetUniformLocation(program_stars, "uTime");


    glm::mat4 mvp = buildMVP(viewport_width, viewport_height);
    
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform4f(colorLoc, 1.0f, 1.0f, 0.9f, 1.0f);
    glUniform1f(timeLoc, (float)elapsed_time);

    glDisable(GL_DEPTH_TEST);
    glPointSize(2.0f);
    
    glBindVertexArray(vao_stars);
    glDrawArrays(GL_POINTS, 0, star_count);
    glBindVertexArray(0);
    
    glEnable(GL_DEPTH_TEST);
}