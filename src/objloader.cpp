#include "objloader.h"
#include <cstdio>
#include <cstring>

bool loadOBJ(
    const char* file_path,
    std::vector<glm::vec3>& out_vertices,
    std::vector<glm::vec2>& out_uvs,
    std::vector<glm::vec3>& out_normals)
{
    printf("[OBJ] Se incarca %s...\n", file_path);

    std::vector<unsigned int> vertex_indices, uv_indices, normal_indices;
    std::vector<glm::vec3>    temp_vertices;
    std::vector<glm::vec2>    temp_uvs;
    std::vector<glm::vec3>    temp_normals;

    FILE* file = fopen(file_path, "r");
    if (!file) {
        printf("[OBJ] Nu am putut deschide: %s\n", file_path);
        return false;
    }

    while (true) {
        char token[128];
        int result = fscanf(file, "%s", token);
        if (result == EOF) break;

        if (strcmp(token, "v") == 0) {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            temp_vertices.push_back(vertex);

        } else if (strcmp(token, "vt") == 0) {
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            temp_uvs.push_back(uv);

        } else if (strcmp(token, "vn") == 0) {
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            temp_normals.push_back(normal);

        } else if (strcmp(token, "f") == 0) {
            unsigned int vi[4], ui[4], ni[4];
            int nr_matches = fscanf(file,
                "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n",
                &vi[0],&ui[0],&ni[0],
                &vi[1],&ui[1],&ni[1],
                &vi[2],&ui[2],&ni[2],
                &vi[3],&ui[3],&ni[3]);

            if (nr_matches == 12) {

                vertex_indices.push_back(vi[0]); uv_indices.push_back(ui[0]); normal_indices.push_back(ni[0]);
                vertex_indices.push_back(vi[1]); uv_indices.push_back(ui[1]); normal_indices.push_back(ni[1]);
                vertex_indices.push_back(vi[2]); uv_indices.push_back(ui[2]); normal_indices.push_back(ni[2]);
                vertex_indices.push_back(vi[0]); uv_indices.push_back(ui[0]); normal_indices.push_back(ni[0]);
                vertex_indices.push_back(vi[2]); uv_indices.push_back(ui[2]); normal_indices.push_back(ni[2]);
                vertex_indices.push_back(vi[3]); uv_indices.push_back(ui[3]); normal_indices.push_back(ni[3]);
            } else if (nr_matches == 9) {

                vertex_indices.push_back(vi[0]); uv_indices.push_back(ui[0]); normal_indices.push_back(ni[0]);
                vertex_indices.push_back(vi[1]); uv_indices.push_back(ui[1]); normal_indices.push_back(ni[1]);
                vertex_indices.push_back(vi[2]); uv_indices.push_back(ui[2]); normal_indices.push_back(ni[2]);
            } else {
                char skip_line[1000];
                fgets(skip_line, 1000, file);
            }
        } else {
            char skip_line[1000];
            fgets(skip_line, 1000, file);
        }
    }

    for (unsigned int i = 0; i < vertex_indices.size(); i++) {
        out_vertices.push_back(temp_vertices[vertex_indices[i] - 1]);
        out_normals.push_back(temp_normals[normal_indices[i] - 1]);

        if (i < uv_indices.size() && uv_indices[i] > 0 && uv_indices[i] <= temp_uvs.size()) {
            out_uvs.push_back(temp_uvs[uv_indices[i] - 1]);
        } else {
            out_uvs.push_back(glm::vec2(0));
        }
    }

    fclose(file);
    printf("[OBJ] Incarcat: %zu vertices, %zu normale\n", out_vertices.size(), out_normals.size());
    return true;
}
