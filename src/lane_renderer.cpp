#include "hud_renderer.h"
#include "projection.h"
#include "geometry.h"
#include <algorithm>

using namespace Projection;
using namespace Geometry;


static float computePolylineAverageX(const Polyline& polyline) {
    float sum = 0;
    for (const Point& p : polyline)
    {
         sum += p[0];
    }
    return sum / polyline.size();
}


static void findEgoLanePair(const std::vector<const Polyline*>& sorted_lines,bool has_previous_corridor,float previous_center_x,int& out_left_idx,int& out_right_idx){
   
    int count = (int)sorted_lines.size();
    float best_distance = 1e9f;
    out_left_idx = 0;
    out_right_idx = 1;

    for (int i = 0; i < count - 1; i++) {
        float avg_left = computePolylineAverageX(*sorted_lines[i]);
        float avg_right = computePolylineAverageX(*sorted_lines[i + 1]);
        float center_x = (avg_left + avg_right) * 0.5f;

        float distance;
        if (has_previous_corridor) {

            distance = fabsf(-(center_x - 0.5f) * 16.f - previous_center_x);
        } 
        else {
            distance = fabsf(center_x - 0.5f);
        }

        if (distance < best_distance) {
            best_distance = distance;
            out_left_idx  = i;
            out_right_idx = i + 1;
        }
    }
}

void HudRenderer::drawEgoCorridor(const PolyVec& lane_lines, const PolyVec& drivable_area) {
    const float SMOOTHING_FACTOR = 0.10f;
    bool corridor_updated = false;

    if (lane_lines.size() >= 2) {
        std::vector<const Polyline*> sorted_lines;
        for (const Polyline& line : lane_lines) {
            if (line.size() >= 3) sorted_lines.push_back(&line);
        }
        std::sort(sorted_lines.begin(), sorted_lines.end(),[](const Polyline* a, const Polyline* b) {
                return computePolylineAverageX(*a) < computePolylineAverageX(*b);
            });

        int nr_lines = (int)sorted_lines.size();
        if (nr_lines >= 2) {
            float previous_center_x = corridor_is_valid && !corridor_center_points.empty() ? corridor_center_points[0].x : 0.f;

            int ego_left_idx = 0;
            int ego_right_idx = 1;
            findEgoLanePair(sorted_lines, corridor_is_valid, previous_center_x, ego_left_idx, ego_right_idx);

            std::vector<glm::vec3> left_ctrl, right_ctrl;
            for (const Point& p : *sorted_lines[ego_left_idx])  left_ctrl.push_back(ipmToWorld(p[0], p[1]));
            for (const Point& p : *sorted_lines[ego_right_idx]) right_ctrl.push_back(ipmToWorld(p[0], p[1]));

            std::vector<glm::vec3> new_left_edge  = buildStraightLine(left_ctrl,  30);
            std::vector<glm::vec3> new_right_edge = buildStraightLine(right_ctrl, 30);

            int min_pts = std::min((int)new_left_edge.size(), (int)new_right_edge.size());
            std::vector<glm::vec3> new_center(min_pts);
            float new_width = 0;
            for (int i = 0; i < min_pts; i++) {
                new_center[i] = {
                    (new_left_edge[i].x + new_right_edge[i].x) * 0.5f,
                    0.02f,
                    (new_left_edge[i].z + new_right_edge[i].z) * 0.5f
                };
                new_width += fabsf(new_right_edge[i].x - new_left_edge[i].x);
            }
            new_width /= min_pts;

            if (!corridor_is_valid) {
                corridor_center_points = new_center;
                corridor_width = new_width;
                corridor_left_edge = new_left_edge;
                corridor_right_edge = new_right_edge;
                corridor_is_valid = true;
            } else {
                smoothPoints(corridor_center_points, new_center, SMOOTHING_FACTOR);
                smoothPoints(corridor_left_edge,new_left_edge, SMOOTHING_FACTOR);
                smoothPoints(corridor_right_edge, new_right_edge,SMOOTHING_FACTOR);
                corridor_width += (new_width - corridor_width) * SMOOTHING_FACTOR;
            }

            lane_edges_current.clear();
            for (int i = 0; i < nr_lines; i++) {
                std::vector<glm::vec3> ctrl_points;
                for (const Point& p : *sorted_lines[i]) ctrl_points.push_back(ipmToWorld(p[0], p[1]));
                std::vector<glm::vec3> straight = buildStraightLine(ctrl_points, 30);
                if (i < (int)lane_edges_previous.size()) {
                    smoothPoints(lane_edges_previous[i], straight, SMOOTHING_FACTOR);
                    lane_edges_current.push_back(lane_edges_previous[i]);
                } else {
                    lane_edges_current.push_back(straight);
                }
            }
            lane_edges_previous = lane_edges_current;

            lanes_to_left = ego_left_idx;
            lanes_to_right = nr_lines - 1 - ego_right_idx;
            total_lane_edges = nr_lines;
            last_lane_seen_time = elapsed_time;
            corridor_updated = true;
        }
    }

    if (!corridor_updated && corridor_is_valid) {
        if ((elapsed_time - last_lane_seen_time) > LANE_PERSIST_SECONDS) {
            corridor_is_valid = false;
            lane_center_positions.clear();
            return;
        }
    }

    if (!corridor_updated && !drivable_area.empty() && !corridor_is_valid) {
        for (const Polyline& poly : drivable_area) {
            if (poly.size() < 3) continue;
            std::vector<glm::vec3> world_points;
            for (const Point& p : poly) world_points.push_back(ipmToWorld(p[0], p[1]));
            std::sort(world_points.begin(), world_points.end(),
                [](const glm::vec3& a, const glm::vec3& b) { return a.z < b.z; });
            if (world_points.size() >= 4) {
                corridor_center_points = buildStraightLine(world_points, 30);
                corridor_width         = 4.f;
                corridor_left_edge     = offsetSpline(corridor_center_points, -2.f);
                corridor_right_edge    = offsetSpline(corridor_center_points,  2.f);
                corridor_is_valid      = true;
                break;
            }
        }
    }

    if (!corridor_is_valid) return;

    float half_lane_width = LANE_WIDTH * 0.5f;
    int nr_lanes  = lanes_to_left + 1 + lanes_to_right;
    int nr_edges  = nr_lanes + 1;
    int nr_points = 30;

    std::vector<float> edge_x_positions(nr_edges);
    edge_x_positions[lanes_to_left]     = -half_lane_width;
    edge_x_positions[lanes_to_left + 1] =  half_lane_width;
    for (int i = lanes_to_left - 1; i >= 0; i--) {
        edge_x_positions[i] = edge_x_positions[i + 1] - LANE_WIDTH;
    }
    for (int i = lanes_to_left + 2; i < nr_edges; i++) {
        edge_x_positions[i] = edge_x_positions[i - 1] + LANE_WIDTH;
    }

    lane_center_positions.resize(nr_lanes);
    for (int lane = 0; lane < nr_lanes; lane++) {
        lane_center_positions[lane] = (edge_x_positions[lane] + edge_x_positions[lane + 1]) * 0.5f;
    }

    for (int lane = 0; lane < nr_lanes; lane++) {
        float x_left  = edge_x_positions[lane];
        float x_right = edge_x_positions[lane + 1];
        bool is_ego_lane = (lane == lanes_to_left);

        std::vector<glm::vec3> left_edge(nr_points);
        std::vector<glm::vec3> right_edge(nr_points);
        for (int i = 0; i < nr_points; i++) {
            float z = ROAD_Z_START + (float)i / (nr_points - 1) * (ROAD_Z_END - ROAD_Z_START);
            left_edge[i]  = {x_left,  0.02f, z};
            right_edge[i] = {x_right, 0.02f, z};
        }

        if (is_ego_lane) {
            drawTriangleStrip(vao_lane, vbo_lane, buildTriangleStrip(left_edge, right_edge, 0.15f),
                              0.f, 1.f, 0.53f, 1.f);
            drawLines(vao_lane, vbo_lane, buildLineVertices(left_edge,  0.9f),
                      1.f, 1.f, 1.f, 0.85f, 3.f, GL_LINE_STRIP);
            drawLines(vao_lane, vbo_lane, buildLineVertices(right_edge, 0.9f),
                      1.f, 1.f, 1.f, 0.85f, 3.f, GL_LINE_STRIP);
        } else {
            drawLines(vao_lane, vbo_lane, buildLineVertices(left_edge,  0.7f),
                      1.f, 1.f, 1.f, 0.65f, 2.5f, GL_LINE_STRIP);
            drawLines(vao_lane, vbo_lane, buildLineVertices(right_edge, 0.7f),
                      1.f, 1.f, 1.f, 0.65f, 2.5f, GL_LINE_STRIP);
        }
    }
}

