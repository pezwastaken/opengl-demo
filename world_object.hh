#pragma once

#include "mesh.hh"
#include <glm/ext/vector_float3.hpp>


// mainly used by point lights.
struct LightData
{
    glm::vec3 direction;
    glm::vec3 source_color;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 reflection;

    float constant;
    float linear;
    float quadratic;

    float innerCutOff;
    float outerCutOff;
};

struct WorldObject
{
    glm::mat4 model_matrix;

    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    LightData light;

    Model model;

    //for color picking
    glm::vec3 unique_color;

    u32 vao;
    u32 shader;
    u32 material_id;


    void reset_transforms()
    {
        model_matrix = glm::mat4(1.0F);
        position = glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
        scale = glm::vec3(1.0F);
        rotation = glm::vec3(0.0F);
    }

    void set_position(glm::vec3 t)
    {
        model_matrix = glm::translate(model_matrix, t);
        position = glm::vec4(t, 1.0F);
    }

    void set_rotation(float angle, glm::vec3 s)
    {
        model_matrix = glm::rotate(model_matrix, angle, s);
        TODO;
    }

    void set_scale(glm::vec3 s)
    {
        model_matrix = glm::scale(model_matrix, s);
        scale = s;
    }

    void update_model_matrix(const glm::mat4& new_model)
    {
        model_matrix = new_model;
    }

    void recompute_model_matrix()
    {
        model_matrix = glm::mat4(1.0F);
        model_matrix = glm::translate(model_matrix, position);
        //TODO: rotation. I think it should be a quaternion, not a vec3.
        //That way I can apply it directly. The debug window would use the usual vec3
        //representation by converting it to a glm::quat behind the scenes
        model_matrix = glm::scale(model_matrix, scale);
    }
};

