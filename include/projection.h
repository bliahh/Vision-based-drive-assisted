#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Projection {

constexpr float SOURCE_WIDTH  = 1920.f;
constexpr float SOURCE_HEIGHT = 1080.f;

// Camera urmareste ego-ul din spate-sus
constexpr glm::vec3 CAMERA_POSITION = {0.f, 5.0f, -3.0f};
constexpr glm::vec3 CAMERA_LOOKAT   = {0.f, 0.0f, 20.f};
constexpr float CAMERA_FOV = 55.f;
constexpr float CLIP_NEAR  = 0.3f;
constexpr float CLIP_FAR   = 200.f;

// Drum
constexpr float LANE_WIDTH   = 3.5f;
constexpr float ROAD_Z_START = -1.5f;
constexpr float ROAD_Z_END   = 80.f;
constexpr float MAX_OBJECT_Z = 22.f;

// Depth Anything -> world: coordonate deja in metri, ego-centrate
// Fara negare — Depth Anything da X cu semnul corect
inline glm::vec3 ipmToWorld(float x_meters, float z_meters) {
    return {-x_meters, 0.0f, z_meters};
}

// Fallback pixeli -> world (pentru obiecte fara depth)
inline glm::vec3 pixelToWorld(float px, float py) {
    float nx = px / SOURCE_WIDTH;
    float ny = py / SOURCE_HEIGHT;
    float below = ny - 0.45f;
    float depth = 70.f;
    if (below > 0.02f) {
        depth = 3.5f / below;
        if (depth < 3.f) depth = 3.f;
        if (depth > 70.f) depth = 70.f;
    }
    float x = (nx - 0.5f) * 20.f;
    return {x, 0.0f, depth};
}

// Matricea View-Projection
inline glm::mat4 buildMVP(int w, int h) {
    float aspect = (float)w / (float)h;
    glm::mat4 proj = glm::perspective(glm::radians(CAMERA_FOV), aspect, CLIP_NEAR, CLIP_FAR);
    glm::mat4 view = glm::lookAt(CAMERA_POSITION, CAMERA_LOOKAT, glm::vec3(0,1,0));
    return proj * view;
}

}