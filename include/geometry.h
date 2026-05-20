#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Geometry {


inline std::vector<glm::vec3> buildStraightLine(const std::vector<glm::vec3>& input_points, int nr_output_points = 30){
    
    
    int nr_input = (int)input_points.size();
    if (nr_input < 2) return input_points;

    float sum_z = 0, sum_x = 0, sum_z2 = 0, sum_xz = 0;
    
    for (auto& p : input_points) {
        sum_z  += p.z;
        sum_x  += p.x;
        sum_z2 += p.z * p.z;
        sum_xz += p.x * p.z;
    }

    float denominator = nr_input * sum_z2 - sum_z * sum_z;
    float slope = 0;
    float intercept = sum_x / nr_input;

    if (fabsf(denominator) > nr_input * 0.1f) {
        slope = (nr_input * sum_xz - sum_x * sum_z) / denominator;
        intercept = (sum_x * sum_z2 - sum_z * sum_xz) / denominator;
    }

    float z_min = input_points[0].z;
    float z_max = input_points[0].z;
    
    for (auto& p : input_points) {
        z_min = std::min(z_min, p.z);
        z_max = std::max(z_max, p.z);
    }

    std::vector<glm::vec3> output;

    for (int i = 0; i < nr_output_points; i++) {
        float pas = (float)i / (nr_output_points - 1);
        float z = z_min + pas * (z_max - z_min);

        output.push_back({slope * z + intercept, 0.02f, z});
    }
    return output;
}



inline std::vector<glm::vec3> offsetSpline(const std::vector<glm::vec3>& spline_points, float offset_distance){

    int n = (int)spline_points.size();
    std::vector<glm::vec3> output(n);
    for (int i = 0; i < n; i++) {
        glm::vec3 tangent;
        if (i == 0){
             tangent = spline_points[std::min(1, n-1)] - spline_points[0];
        }
        else if (i == n-1){
             tangent = spline_points[n-1] - spline_points[std::max(0, n-2)];
        }else {
            tangent = spline_points[i+1] - spline_points[i-1];
        }

        float len = glm::length(tangent);
        if (len > 1e-6f){
         tangent /= len;
        }
        else{ 
            tangent = glm::vec3(0, 0, 1);
        }

        output[i] = {
            spline_points[i].x + (-tangent.z) * offset_distance,
            spline_points[i].y,
            spline_points[i].z + tangent.x * offset_distance
        };
    }
    return output;
}



inline std::vector<float> buildTriangleStrip(const std::vector<glm::vec3>& left_edge, const std::vector<glm::vec3>& right_edge, float base_alpha = 1.f){

    std::vector<float> vertices;
    int n = std::min((int)left_edge.size(), (int)right_edge.size());
    for (int i = 0; i < n; i++) {
        float t = (float)i / (n - 1);
        float alpha = base_alpha * (1.f - t * 0.3f);
        
        vertices.push_back(left_edge[i].x);  
        vertices.push_back(left_edge[i].y);
        vertices.push_back(left_edge[i].z);  
        vertices.push_back(alpha);
        
        vertices.push_back(right_edge[i].x); 
        vertices.push_back(right_edge[i].y);
        vertices.push_back(right_edge[i].z); 
        vertices.push_back(alpha);
    }
    return vertices;
}



inline std::vector<float> buildLineVertices(const std::vector<glm::vec3>& spline_points, float alpha = 1.f){
    std::vector<float> vertices;
    int n = (int)spline_points.size();
    for (int i = 0; i < n; i++) {
        float t = (float)i / (n - 1);
        vertices.push_back(spline_points[i].x);
        vertices.push_back(spline_points[i].y);
        vertices.push_back(spline_points[i].z);
        vertices.push_back(alpha * (1.f - t * 0.2f));
    }
    return vertices;
}


inline void smoothPoints(std::vector<glm::vec3>& current,const std::vector<glm::vec3>& target,float blend_factor){
   
    int min_size = std::min((int)current.size(), (int)target.size());
   
    if (min_size == 0) { 
        current = target; 
        return; 
    }
   
    current.resize(min_size);
    for (int i = 0; i < min_size; i++) {
        current[i].x += (target[i].x - current[i].x) * blend_factor;
        current[i].z += (target[i].z - current[i].z) * blend_factor;
        current[i].y  = 0.02f;
    }
}




// inline void buildBoxEdges(const glm::vec3& center,float half_width,float half_height,float half_depth,std::vector<float>& output_vertices){
    
    
//     glm::vec3 corners[8];
//     corners[0] = center + glm::vec3(-half_width, 0.05f, -half_depth);   //sx jos
//     corners[1] = center + glm::vec3(+half_width, 0.05f, -half_depth); //dx jos
//     corners[2] = center + glm::vec3(-half_width, half_height, -half_depth); 
//     corners[3] = center + glm::vec3(+half_width, half_height, -half_depth);

//     corners[4] = center + glm::vec3(-half_width, 0.05f, +half_depth);
//     corners[5] = center + glm::vec3(+half_width, 0.05f, +half_depth);
//     corners[6] = center + glm::vec3(-half_width, half_height, +half_depth);
//     corners[7] = center + glm::vec3(+half_width, half_height, +half_depth);

//     int edge_pairs[12][2] = {
//         {0,1},{1,3},{3,2},{2,0},   
//         {4,5},{5,7},{7,6},{6,4},   
//         {0,4},{1,5},{2,6},{3,7}    
//     };

//     for (int e = 0; e < 12; e++) {
//         int a = edge_pairs[e][0];
//         int b = edge_pairs[e][1];
       
//         output_vertices.push_back(corners[a].x);
//         output_vertices.push_back(corners[a].y);
//         output_vertices.push_back(corners[a].z);
//         output_vertices.push_back(1.f);
        
//         output_vertices.push_back(corners[b].x);
//         output_vertices.push_back(corners[b].y);
//         output_vertices.push_back(corners[b].z); 
//         output_vertices.push_back(1.f);
//     }
// }

} 
