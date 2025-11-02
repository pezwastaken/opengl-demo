#pragma once

#include "mesh.hh"
#include <glm/common.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>

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

struct Rotation
{
    glm::quat inner;
    //works as a buffer to allow editing the rotation property without having
    //to directly interact with quaternion representation
    glm::vec3 rotation_wrt_default_axes;
};

struct WorldObject
{
    glm::mat4 model_matrix;

    glm::vec3 position;
    Rotation rotation;
    glm::vec3 scale;

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
        // rotation = glm::vec3(0.0F);
        rotation = Rotation {
            .inner = glm::quat(glm::vec3(0.0F)),
            .rotation_wrt_default_axes = glm::vec3(0.0F)
        };
    }

    void set_position(glm::vec3 t)
    {
        model_matrix = glm::translate(model_matrix, t);
        position = glm::vec4(t, 1.0F);
    }

    //TODO: Double-check this part
    //
    //r is a vec3 that defines pitch,yaw,roll using degrees.
    // A quaternion is built from this vector. The rotation quaternion is then
    // applied to the model matrix.
    void set_rotation(glm::vec3 r)
    {
        rotation.rotation_wrt_default_axes = r;
        rotation.inner = glm::quat(glm::radians(rotation.rotation_wrt_default_axes));
        model_matrix = model_matrix * glm::mat4_cast(rotation.inner);
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

        glm::quat r = glm::quat(glm::radians(rotation.rotation_wrt_default_axes));
        model_matrix = model_matrix * glm::mat4_cast(r);
        model_matrix = glm::scale(model_matrix, scale);
    }
};

