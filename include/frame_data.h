#pragma once
#include "parse_json.h"
#include <list>



struct FrameData {
    PolyVec lane_lines;
    PolyVec drivable_area;
    std::list<Object> detected_cars;
    std::list<Object> detected_signs;
    std::list<Object> detected_people;
    float fps = 0.f;
    bool tcp_ok = false;
};
