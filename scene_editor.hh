#pragma once

#include "inspector.hh"
#include "mesh.hh"

struct Axis
{
    glm::mat4 model_matrix;
    Model model;

    //x axis is red, y axis green and z axis is blue
    glm::vec3 color;
};

struct SceneEditor
{

    Inspector inspector;

    Axis x_axis_cue;
    Axis y_axis_cue;
    Axis z_axis_cue;

    //shader used to render editor related stuff like axes
    Shader editor_shader;
};


