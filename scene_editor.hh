#pragma once

#include "inspector.hh"
#include "mesh.hh"
#include "world_object.hh"

struct Axis
{
    glm::mat4 model_matrix;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    Model model;

    //x axis is red, y axis green and z axis is blue
    glm::vec3 color;

    u32 axis_idx;
};


struct AxisCues
{
    Axis x_axis_cue;
    Axis y_axis_cue;
    Axis z_axis_cue;

};

struct SceneEditor
{
    Inspector inspector;

    AxisCues axis_cues;

    WorldObject* currently_selected_obj;

    //shader used to render editor related stuff like axes
    Shader editor_shader;
};

void scene_editor_init(SceneEditor& scene_editor);
void scene_editor_click_on_nothing(SceneEditor& scene_editor);
void scene_editor_update_axis_cues(SceneEditor& scene_editor);
void scene_editor_render_objects(SceneEditor& scene_editor);


