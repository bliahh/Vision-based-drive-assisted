#include "hud_renderer.h"
#include "projection.h"
#include "objloader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <iostream>
#include <stack>
using namespace Projection;


struct StreetLightEntry {
glm::vec3 position;
float yaw;
};

void HudRenderer::loadStreetLightModel() {
std::vector<glm::vec3> vertices = {

{-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
{-0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},

{ 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
{ 0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},

{ 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
{ 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},

{-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
{-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},

{-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
{-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},

{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
};

std::vector<glm::vec3> normals = {
{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1},
{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
{1,0,0},{1,0,0},{1,0,0},{1,0,0},{1,0,0},{1,0,0},
{-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},
{0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,1,0},
{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},
};

 streetlight_vertex_count = 36;

glGenVertexArrays(1, &vao_streetlight_model);
glBindVertexArray(vao_streetlight_model);

glGenBuffers(1, &vbo_streetlight_vertices);
glBindBuffer(GL_ARRAY_BUFFER, vbo_streetlight_vertices);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
glEnableVertexAttribArray(0);

glGenBuffers(1, &vbo_streetlight_normals);
glBindBuffer(GL_ARRAY_BUFFER, vbo_streetlight_normals);
glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
glEnableVertexAttribArray(1);

glBindVertexArray(0);
streetlight_model_loaded = true;
}

void HudRenderer::drawStreetLightModel(const glm::vec3& position, float yaw, float scale, float r, float g, float b, float alpha) {
if (!streetlight_model_loaded || streetlight_vertex_count <= 0) return;

glUseProgram(program_model);
glBindVertexArray(vao_streetlight_model);
glm::mat4 mvp_base = buildMVP(viewport_width, viewport_height);

auto drawBox = [&](glm::mat4 model, float cr, float cg, float cb, float ca) {
glm::mat4 mvp = mvp_base * model;
glm::mat3 normal_mat = glm::transpose(glm::inverse(glm::mat3(model)));

GLint loc_mvp = glGetUniformLocation(program_model, "uMVP");
GLint loc_model = glGetUniformLocation(program_model, "uModel");
GLint loc_normal_mat = glGetUniformLocation(program_model, "uNormalMat");
GLint loc_color = glGetUniformLocation(program_model, "uColor");
GLint loc_light_dir = glGetUniformLocation(program_model, "uLightDir");
GLint loc_view_pos = glGetUniformLocation(program_model, "uViewPos");
GLint loc_time = glGetUniformLocation(program_model, "uTime");

glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
glUniformMatrix4fv(loc_model, 1, GL_FALSE, glm::value_ptr(model));
glUniformMatrix3fv(loc_normal_mat, 1, GL_FALSE, glm::value_ptr(normal_mat));
glUniform4f(loc_color, cr, cg, cb, ca);
glUniform3f(loc_light_dir, 0.3f, 1.0f, 0.5f);
glUniform3f(loc_view_pos, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);
glUniform1f(loc_time, elapsed_time);
glDrawArrays(GL_TRIANGLES, 0, streetlight_vertex_count);
};

std::stack<glm::mat4> stiva;
glm::mat4 M(1.f);

M = glm::translate(M, position);
M = glm::rotate(M, yaw, glm::vec3(0, 1, 0));
M = glm::scale(M, glm::vec3(scale));

M = glm::translate(M, glm::vec3(0.f, 1.5f, 0.f));
stiva.push(M);
glm::mat4 stalp = glm::scale(M, glm::vec3(0.2f, 3.0f, 0.2f));
drawBox(stalp, r, g, b, alpha);
M = stiva.top(); stiva.pop();

M = glm::translate(M, glm::vec3(0.f, 1.4f, 0.f));
M = glm::translate(M, glm::vec3(0.6f, 0.f, 0.f));
stiva.push(M);
glm::mat4 brat = glm::scale(M, glm::vec3(1.2f, 0.15f, 0.15f));
drawBox(brat, r, g, b, alpha);
M = stiva.top(); stiva.pop();

M = glm::translate(M, glm::vec3(0.5f, -0.15f, 0.f));
stiva.push(M);
glm::mat4 lampa = glm::scale(M, glm::vec3(0.4f, 0.15f, 0.4f));
drawBox(lampa, r, g, b, alpha);
M = stiva.top(); stiva.pop();

M = glm::translate(M, glm::vec3(0.f, -0.15f, 0.f));
stiva.push(M);
glm::mat4 bec = glm::scale(M, glm::vec3(0.25f, 0.15f, 0.25f));
drawBox(bec, 1.0f, 0.9f, 0.3f, 1.0f);
M = stiva.top(); stiva.pop();




float light_dir = (yaw > 1.0f) ? -1.0f : 1.0f;
float light_x = position.x + light_dir * 1.1f * scale;
float light_z = position.z;
float light_r = 2.5f * scale;

std::vector<float> light_spot;
int seg_count = 12;
for (int i = 0; i < seg_count; i++) {
float a1 = 6.28318f * i / seg_count;
float a2 = 6.28318f * (i + 1) / seg_count;
light_spot.push_back(light_x);
light_spot.push_back(0.03f);
light_spot.push_back(light_z);
light_spot.push_back(0.25f);
light_spot.push_back(light_x + cosf(a1) * light_r);
light_spot.push_back(0.03f);
light_spot.push_back(light_z + sinf(a1) * light_r * 0.6f);
light_spot.push_back(0.02f);
light_spot.push_back(light_x + cosf(a2) * light_r);
light_spot.push_back(0.03f);
light_spot.push_back(light_z + sinf(a2) * light_r * 0.6f);
light_spot.push_back(0.02f);
}

glUseProgram(program_lane);
glm::mat4 mvp_light = buildMVP(viewport_width, viewport_height);

GLint loc_lane_mvp = glGetUniformLocation(program_lane, "uMVP");
GLint loc_lane_time = glGetUniformLocation(program_lane, "uTime");
GLint loc_lane_color = glGetUniformLocation(program_lane, "uColor");
GLint loc_lane_view_pos = glGetUniformLocation(program_lane, "uViewPos");
GLint loc_lane_fog = glGetUniformLocation(program_lane, "uFogEnabled");

glUniformMatrix4fv(loc_lane_mvp, 1, GL_FALSE, glm::value_ptr(mvp_light));
glUniform1f(loc_lane_time, elapsed_time);
glUniform4f(loc_lane_color, 1.0f, 0.9f, 0.4f, 0.45f);
glUniform3f(loc_lane_view_pos, CAMERA_POSITION.x, CAMERA_POSITION.y, CAMERA_POSITION.z);
glUniform1i(loc_lane_fog, 0);

glBindVertexArray(vao_lane);
glBindBuffer(GL_ARRAY_BUFFER, vbo_lane);
glBufferData(GL_ARRAY_BUFFER, light_spot.size() * sizeof(float), light_spot.data(), GL_STREAM_DRAW);
glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(light_spot.size() / 4));
glBindVertexArray(0);

glBindVertexArray(0);
}


void HudRenderer::drawStreetLights() {
if (!streetlight_model_loaded) return;

float spacing = 6.0f;
float offset_x = 8.5f;
float z_start = 5.0f;
float z_end = ROAD_Z_END;
float scale = 1.0f;

for (float z = z_start; z < z_end; z += spacing) {
StreetLightEntry entry = { {-offset_x, 0.0f, z}, 0.f };
drawStreetLightModel(entry.position, entry.yaw, scale, 0.75f, 0.75f, 0.78f, 1.0f);
}

for (float z = z_start; z < z_end; z += spacing) {
StreetLightEntry entry = { { offset_x, 0.0f, z}, 3.14159f };
drawStreetLightModel(entry.position, entry.yaw, scale, 0.75f, 0.75f, 0.78f, 1.0f);
}
}