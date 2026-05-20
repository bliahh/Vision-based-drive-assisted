#pragma once
#include <nlohmann/json.hpp>
#include <vector>
#include <array>
#include <list>
#include <string>

using Point = std::array<float, 2>;
using Polyline = std::vector<Point>;
using PolyVec = std::vector<Polyline>;

struct Object {
    std::string label;
    float confidence = 0.f;
    int pixel_x1 = 0;
    int pixel_y1 = 0;
    int pixel_x2 = 0;
    int pixel_y2 = 0;
    float world_x_m = 0.f;  
    float world_z_m  = 0.f;  
    bool has_world_coords = false;
    int track_id = -1;
};

inline PolyVec parseLaneLines(const nlohmann::json& json_frame) {
    PolyVec result;
    if (!json_frame.contains("lane") || !json_frame["lane"].contains("lane_lines")) {
        return result;
    }
    for (auto& line : json_frame["lane"]["lane_lines"]) {
        Polyline points;
        for (auto& point : line) {
            points.push_back({point[0].get<float>(), point[1].get<float>()});
        }
        if (points.size() >= 2) {
            result.push_back(points);
        }
    }
    return result;
}

inline PolyVec parseDrivableArea(const nlohmann::json& json_frame) {
    PolyVec result;
    if (!json_frame.contains("lane") || !json_frame["lane"].contains("drivable_polygons")) {
        return result;
    }
    for (auto& polygon : json_frame["lane"]["drivable_polygons"]) {
        Polyline points;
        for (auto& point : polygon) {
            points.push_back({point[0].get<float>(), point[1].get<float>()});
        }
        if (points.size() >= 3) {
            result.push_back(points);
        }
    }
    return result;
}

inline std::list<Object> parseDetectedObjects(const nlohmann::json& json_array) {
    std::list<Object> result;
    for (auto& entity : json_array) {
        Object obj;
        obj.label      = entity["label"].get<std::string>();
        obj.confidence = entity["conf"].get<float>();
        obj.pixel_x1   = entity["box"][0];
        obj.pixel_y1   = entity["box"][1];
        obj.pixel_x2   = entity["box"][2];
        obj.pixel_y2   = entity["box"][3];
        if (entity.contains("x_m") && entity.contains("z_m")) {
            obj.world_x_m       = entity["x_m"].get<float>();
            obj.world_z_m       = entity["z_m"].get<float>();
            obj.has_world_coords = true;
        }
        if (entity.contains("track_id")) {
            obj.track_id = entity["track_id"].get<int>();
        }
        result.push_back(obj);
    }
    return result;
}
