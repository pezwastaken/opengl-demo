#pragma once

#include <GLES2/gl2.h>
#include <fstream>
#include <glad/glad.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "utils.hh"

struct Shader
{
    u32 m_id;

    explicit Shader()
    {

    }

    explicit Shader(const char* vertex_path, const char* fragment_path)
    {
        std::string vertex_code;
        std::string frag_code;

        std::ifstream vertex_shader_file (vertex_path);
        if (!vertex_shader_file)
        {
            std::clog << "couldn't open vertex shader file: " << vertex_path << std::endl;
            exit(1);
        }

        std::ifstream frag_shader_file (fragment_path);
        if (!frag_shader_file)
        {
            std::clog << "couldn't open fragment shader file: " << fragment_path << std::endl;
            exit(1);
        }

        std::stringstream vertex_shader_stream, frag_shader_stream;
        vertex_shader_stream << vertex_shader_file.rdbuf();
        frag_shader_stream << frag_shader_file.rdbuf();

        vertex_shader_file.close();
        frag_shader_file.close();

        vertex_code = vertex_shader_stream.str();
        frag_code = frag_shader_stream.str();

        u32 vertex_id, frag_id;
        int success;
        char info[512];

        const char* vertex_code_c_string = vertex_code.c_str();
        const char* frag_code_c_string = frag_code.c_str();

        vertex_id = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_id, 1, &vertex_code_c_string, NULL);
        glCompileShader(vertex_id);
        glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vertex_id, 512, NULL, info);
            std::clog << "vertex shader compilation failed: " << info << std::endl;
            exit(1);
        }

        frag_id = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag_id, 1, &frag_code_c_string, NULL);
        glCompileShader(frag_id);
        glGetShaderiv(frag_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(frag_id, 512, NULL, info);
            std::clog << "fragment shader compilation failed: " << info << std::endl;
            exit(1);
        }

        //link the two
        m_id = Shader::create_shader_program_obj({vertex_id, frag_id});
        glDeleteShader(vertex_id);
        glDeleteShader(frag_id);
    }

    void use()
    {
        glUseProgram(m_id);
    }

    void set_bool(const std::string& name, bool value)
    {
        glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void set_int(const std::string& name, int value)
    {
        glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void set_float(const std::string& name, float value)
    {
        glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void set_vec3(const std::string& name, glm::vec3 value)
    {
        glUniform3f(glGetUniformLocation(m_id, name.c_str()),
                    value.x, value.y, value.z);
    }

    void set_vec4(const std::string& name, glm::vec4 value)
    {
        glUniform4f(glGetUniformLocation(m_id, name.c_str()),
                    value.x, value.y, value.z, value.w);
    }

    void set_mat4(const std::string& name, const glm::mat4& value)
    {
        glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()),
                    1, GL_FALSE, glm::value_ptr(value));
    }

    inline bool has_uniform(const char* name) const
    {
        return glGetUniformLocation(m_id, name) != -1;
    }

    //create a new shader program by linking the given shaders
    static u32 create_shader_program_obj(std::vector<u32> shaders)
    {
        u32 shader_program;
        shader_program = glCreateProgram();

        glAttachShader(shader_program, shaders[0]);
        glAttachShader(shader_program, shaders[1]);
        glLinkProgram(shader_program);

        int success;
        char info_log[512];
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader_program, 512, NULL, info_log);
            std::clog << "program link failed: " << info_log << std::endl;
            exit(1);
        }

        return shader_program;
    }

};

