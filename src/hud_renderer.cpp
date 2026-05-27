#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "hud_renderer.h"
#include "shader_loader.h"
#include "projection.h"
#include "objloader.h"

#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <cstdio>

using namespace Projection;

static auto g_start = std::chrono::steady_clock::now();


static GLuint loadTexture(const std::string& path) {

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);

    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        printf("[TEX] OK: %s (%dx%d)\n", path.c_str(), w, h);
    } else {
        printf("[TEX] LIPSA: %s\n", path.c_str());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}


void HudRenderer::init(int w, int h, const std::string& asset_dir) {

    viewport_width = w;
    viewport_height = h;
    asset_directory = asset_dir;

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    std::string sd = asset_dir + "/shaders/";
    program_rain = ShaderLoader::loadProgram(sd + "rain.vert", sd + "rain.frag");
    program_lane = ShaderLoader::loadProgram(sd + "world.vert", sd + "lane.frag");
    program_road = ShaderLoader::loadProgram(sd + "world.vert", sd + "road.frag");
    program_hud = ShaderLoader::loadProgram(sd + "hud.vert", sd + "hud.frag");
    program_model = ShaderLoader::loadProgram(sd + "model.vert", sd + "model.frag");
    program_building = ShaderLoader::loadProgram(sd + "building.vert", sd + "building.frag");
    program_road_nm = ShaderLoader::loadProgram(sd + "road_nm.vert", sd + "road_nm.frag");
    program_stars = ShaderLoader::loadProgram(sd + "star.vert", sd + "star.frag");
    program_sky = ShaderLoader::loadProgram(sd + "sky.vert", sd + "sky.frag");

    glGenVertexArrays(1, &vao_lane);
    glGenBuffers(1, &vbo_lane);
    glBindVertexArray(vao_lane);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_lane);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenVertexArrays(1, &vao_road);
    glGenBuffers(1, &vbo_road);
    glBindVertexArray(vao_road);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_road);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    glGenVertexArrays(1, &vao_hud);
    glGenBuffers(1, &vbo_hud);
    glBindVertexArray(vao_hud);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_hud);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    road_texture = loadTexture(asset_dir + "/assets/road.jpg");
    building_texture = loadTexture(asset_dir + "/assets/building.jpg");
    tex_bricks = loadTexture(asset_dir + "/assets/bricks.jpg");
    tex_windows = loadTexture(asset_dir + "/assets/ferestre.jpg");

    tex_facade_color = loadTexture(asset_dir + "/assets/Facade009_4K-PNG_Color.png");
    tex_facade_normal = loadTexture(asset_dir + "/assets/Facade009_4K-PNG_NormalGL.png");

    tex_road_color = loadTexture(asset_dir + "/assets/Road012A_4K-PNG_Color.png");
    tex_road_normal = loadTexture(asset_dir + "/assets/Road012A_4K-PNG_NormalDX.png");

    loadCarModel(asset_dir + "/assets/car.obj");
    loadPersonModel(asset_dir + "/assets/person2.obj");

    printf("[OK] ADAS HUD initializat\n");

    loadStreetLightModel();
    loadStars(6000);
    loadRain(3000);
}


void HudRenderer::resize(int w, int h) {

    viewport_width = w;
    viewport_height = h;
}


void HudRenderer::drawTriangleStrip(GLuint vao, GLuint vbo,
    const std::vector<float>& vertices, float r, float g, float b, float alpha)
{
    elapsed_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - g_start).count();

    glm::mat4 mvp = buildMVP(viewport_width, viewport_height);
    glUseProgram(program_lane);

    GLint loc_mvp = glGetUniformLocation(program_lane, "uMVP");
    GLint loc_time = glGetUniformLocation(program_lane, "uTime");
    GLint loc_color = glGetUniformLocation(program_lane, "uColor");
    GLint loc_fog_enabled = glGetUniformLocation(program_lane, "uFogEnabled");
    GLint loc_fog_density = glGetUniformLocation(program_lane, "uFogDensity");

    glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(loc_time, elapsed_time);
    glUniform4f(loc_color, r, g, b, alpha);
    glUniform1i(loc_fog_enabled, fog_enabled ? 1 : 0);
    glUniform1f(loc_fog_density, fog_density);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)(vertices.size() / 4));
    glBindVertexArray(0);
}


void HudRenderer::drawLines(GLuint vao, GLuint vbo,
    const std::vector<float>& vertices, float r, float g, float b, float alpha,
    float line_width, GLenum draw_mode)
{
    elapsed_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - g_start).count();

    glm::mat4 mvp = buildMVP(viewport_width, viewport_height);
    glUseProgram(program_lane);

    GLint loc_mvp = glGetUniformLocation(program_lane, "uMVP");
    GLint loc_time = glGetUniformLocation(program_lane, "uTime");
    GLint loc_color = glGetUniformLocation(program_lane, "uColor");

    glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(loc_time, elapsed_time);
    glUniform4f(loc_color, r, g, b, alpha);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
    glLineWidth(line_width);
    glDrawArrays(draw_mode, 0, (GLsizei)(vertices.size() / 4));
    glBindVertexArray(0);
}