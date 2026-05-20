#pragma once
#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace ShaderLoader {

inline std::string readFileToString(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[SHADER] Nu am putut deschide: " << file_path << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline GLuint compileShader(GLenum shader_type, const std::string& source_code, const std::string& shader_name){
    GLuint shader_id = glCreateShader(shader_type);
    const char* source_ptr = source_code.c_str();
    glShaderSource(shader_id, 1, &source_ptr, nullptr);
    glCompileShader(shader_id);

    GLint compile_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_ok);
    if (!compile_ok) {
        char error_log[1024];
        glGetShaderInfoLog(shader_id, 1024, nullptr, error_log);
        std::cerr << "[SHADER] Eroare compilare " << shader_name << ":\n" << error_log << "\n";
    }
    return shader_id;
}

inline GLuint loadProgram(const std::string& vert_path, const std::string& frag_path) {
    std::string vert_source = readFileToString(vert_path);
    std::string frag_source = readFileToString(frag_path);
    if (vert_source.empty() || frag_source.empty()) return 0;

    GLuint vert_shader = compileShader(GL_VERTEX_SHADER,   vert_source, vert_path);
    GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_source, frag_path);

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vert_shader);
    glAttachShader(program_id, frag_shader);
    glLinkProgram(program_id);

    GLint link_ok;
    glGetProgramiv(program_id, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        char error_log[1024];
        glGetProgramInfoLog(program_id, 1024, nullptr, error_log);
        std::cerr << "[SHADER] Eroare link:\n" << error_log << "\n";
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    printf("[SHADER] Incarcat: %s + %s\n", vert_path.c_str(), frag_path.c_str());
    return program_id;
}

} 
