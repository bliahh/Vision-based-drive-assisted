#include "hud_renderer.h"
#include <GL/glew.h>


/// @brief deseneaza cerul, inclusiv cu gradient
void HudRenderer::drawSky() {

    glDisable(GL_DEPTH_TEST);

    float quad[] = {
        -1.f, -1.f,
         1.f, -1.f,
         1.f,  1.f,
        -1.f,  1.f
    };

    glUseProgram(program_sky);
    glBindVertexArray(vao_hud);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_hud);
    glUniform1i(glGetUniformLocation(program_sky, "uFogEnabled"), fog_enabled ? 1 : 0);
    glUniform1f(glGetUniformLocation(program_sky, "uFogDensity"), fog_density);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}
