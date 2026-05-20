
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <list>
#include <string>
#include "parse_json.h"

class HudRenderer {
public:
    enum Layer {
        LAYER_DRIVABLE_AREA = 0,
        LAYER_LANE_LINES,
        LAYER_CARS,
        LAYER_SIGNS,
        LAYER_DEBUG,
        LAYER_COUNT,
    };

    GLuint tex_bricks;
    GLuint tex_windows;
    GLuint building_texture;

    void init(int viewport_w, int viewport_h, const std::string& asset_dir = ".");
    void resize(int viewport_w, int viewport_h);
    void toggleLayer(Layer layer);

    void drawRoad();
    void drawDrivableArea(const PolyVec& polygons);
    void drawEgoCorridor(const PolyVec& lane_lines, const PolyVec& drivable_area);
    void drawLaneLines(const PolyVec& lines);
    void drawCars(const std::list<Object>& cars);
    void drawSigns(const std::list<Object>& signs);
    void drawEgoCar();
    void drawBuildings();
    void drawStreetLights();

    void loadPersonModel(const std::string& obj_path);
    void drawPersonModel(const glm::vec3& position, float yaw_radians, float scale, float r, float g, float b, float alpha);
    void drawPersons(const std::list<Object>& persons);

    void loadStreetLightModel();
    void drawStreetLightModel(const glm::vec3& position, float yaw_radians, float scale, float r, float g, float b, float alpha);


    GLuint program_road_nm = 0;   
GLuint tex_road_color  = 0;    
GLuint tex_road_normal = 0;    



    GLuint program_building = 0;
 
    GLuint tex_bricks_color  = 0;
    GLuint tex_bricks_normal = 0;
    GLuint tex_facade_color  = 0;
    GLuint tex_facade_normal = 0;



    GLuint program_lane    = 0;
    GLuint program_road    = 0;
    GLuint program_hud     = 0;
    GLuint program_model   = 0;
    GLuint program_stars = 0;
    GLuint vao_lane        = 0, vbo_lane = 0;
    GLuint vao_road        = 0, vbo_road = 0;
    GLuint vao_hud         = 0, vbo_hud  = 0;
    GLuint road_texture;

    GLuint vao_car_model      = 0;
    GLuint vbo_car_vertices   = 0;
    GLuint vbo_car_normals    = 0;

    GLuint vao_person_model   = 0;
    GLuint vbo_person_vertices = 0;
    GLuint vbo_person_normals  = 0;

    GLuint vao_streetlight_model    = 0;
    GLuint vbo_streetlight_vertices = 0;
    GLuint vbo_streetlight_normals  = 0;

    int  person_vertex_count      = 0;
    int  car_vertex_count         = 0;
    int  streetlight_vertex_count = 0;

    bool car_model_loaded         = false;
    bool streetlight_model_loaded = false;

    int   viewport_width  = 1280;
    int   viewport_height = 720;
    float elapsed_time    = 0.f;

    bool layer_enabled[LAYER_COUNT] = {true, true, true, true, false};

    std::string asset_directory;


    GLuint vao_stars = 0, vbo_stars = 0;
    int star_count = 0;
    bool stars_loaded = false;

    void drawStars();
    void loadStars(int count);

    std::vector<glm::vec3> corridor_center_points;
    std::vector<glm::vec3> corridor_left_edge;
    std::vector<glm::vec3> corridor_right_edge;
    float corridor_width    = 3.5f;
    bool  corridor_is_valid = false;

    int   lanes_to_left    = 0;
    int   lanes_to_right   = 0;
    int   total_lane_edges = 0;

    float last_lane_seen_time = 0.f;
    static constexpr float LANE_PERSIST_SECONDS = 3.0f;

    std::vector<float>                   lane_center_positions;
    std::vector<std::vector<glm::vec3>>  lane_edges_current;
    std::vector<std::vector<glm::vec3>>  lane_edges_previous;

    void loadCarModel(const std::string& obj_path);
    void drawCarModel(const glm::vec3& position, float yaw_radians, float scale, float r, float g, float b, float alpha);
    void drawTriangleStrip(GLuint vao, GLuint vbo, const std::vector<float>& vertices, float r, float g, float b, float alpha);
    void drawLines(GLuint vao, GLuint vbo, const std::vector<float>& vertices, float r, float g, float b, float alpha, float line_width, GLenum draw_mode);


    void drawSky();
    GLuint program_sky = 0;


    bool fog_enabled = false;
    float fog_density = 0.025f;


    void drawTrotuar();



    GLuint program_rain = 0;
GLuint vao_rain = 0, vbo_rain = 0;
int rain_count = 0;
bool rain_enabled = false;
void loadRain(int count);
void drawRain();
};