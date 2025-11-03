#include "scene_editor.hh"
#include "utils.hh"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>

static const glm::quat default_axis_rotations[3] =
{
    glm::quat(glm::vec3(0,0,glm::radians(270.0F))),
    glm::quat(glm::vec3(0,0,0)),
    glm::quat(glm::vec3(glm::radians(90.0F),0,0))
};

void scene_editor_init(SceneEditor& scene_editor)
{
    //init axis cues
    Model axis_arrow_model;
    model_init(axis_arrow_model, "models/editor-models/axis_arrow.glb");

    //x axis
    glm::mat4 model_matrix (1.0F);
    model_matrix = glm::translate(model_matrix, glm::vec3(0,4, 5));
    model_matrix = glm::rotate(model_matrix, glm::radians(90.0F), glm::vec3(0,0,-1));
    Axis x_axis_arrow {
        .model_matrix = model_matrix,
        .model = axis_arrow_model,
        .color = glm::vec3(0.9, 0.3, 0.3)
    };
    scene_editor.axis_cues.x_axis_cue = x_axis_arrow;

    //y axis
    model_matrix = glm::mat4(1.0F);
    model_matrix = glm::translate(model_matrix, glm::vec3(0,4, 5));
    Axis y_axis_arrow {
        .model_matrix = model_matrix,
        .model = axis_arrow_model,
        .color = glm::vec3(0.3, 0.9, 0.3)
    };
    scene_editor.axis_cues.y_axis_cue = y_axis_arrow;

    //z axis
    model_matrix = glm::mat4(1.0F);
    model_matrix = glm::translate(model_matrix, glm::vec3(0,4, 5));
    model_matrix = glm::rotate(model_matrix, glm::radians(90.0F), glm::vec3(1,0,0));
    Axis z_axis_arrow {
        .model_matrix = model_matrix,
        .model = axis_arrow_model,
        .color = glm::vec3(0.3, 0.3, 0.9)
    };
    scene_editor.axis_cues.z_axis_cue = z_axis_arrow;

    //init inspector panel
    scene_editor.inspector.reset();

}

void scene_editor_click_on_nothing(SceneEditor& scene_editor)
{
    scene_editor.currently_selected_obj = nullptr;
    scene_editor.inspector.reset();
}

void scene_editor_update_axis_cues(SceneEditor& scene_editor)
{
    assert(scene_editor.currently_selected_obj != nullptr);

    glm::vec3 selected_obj_position = scene_editor.currently_selected_obj->position;

    //x axis cue
    glm::mat4 model_matrix (1.0F);
    model_matrix = glm::translate(model_matrix, selected_obj_position);
    model_matrix = model_matrix * glm::mat4_cast(default_axis_rotations[0]);
    scene_editor.axis_cues.x_axis_cue.model_matrix = model_matrix;

    //y axis cue
    model_matrix = glm::mat4(1.0F);
    model_matrix = glm::translate(model_matrix, selected_obj_position);
    model_matrix = model_matrix * glm::mat4_cast(default_axis_rotations[1]);
    scene_editor.axis_cues.y_axis_cue.model_matrix = model_matrix;

    //z axis cue
    model_matrix = glm::mat4(1.0F);
    model_matrix = glm::translate(model_matrix, selected_obj_position);
    model_matrix = model_matrix * glm::mat4_cast(default_axis_rotations[2]);
    scene_editor.axis_cues.z_axis_cue.model_matrix = model_matrix;

    //TODO: selected object rotation is ignored for now
}

void scene_editor_render_objects(SceneEditor& scene_editor)
{

    Shader& shader = scene_editor.editor_shader;
    shader.use();

    const auto& x_axis_cue = scene_editor.axis_cues.x_axis_cue;
    shader.set_mat4("model", x_axis_cue.model_matrix);
    shader.set_vec3("color", x_axis_cue.color);
    model_draw(x_axis_cue.model, shader);

    const auto& y_axis_cue = scene_editor.axis_cues.y_axis_cue;
    shader.set_mat4("model", y_axis_cue.model_matrix);
    shader.set_vec3("color", y_axis_cue.color);
    model_draw(y_axis_cue.model, shader);

    const auto& z_axis_cue = scene_editor.axis_cues.z_axis_cue;
    shader.set_mat4("model", z_axis_cue.model_matrix);
    shader.set_vec3("color", z_axis_cue.color);
    model_draw(z_axis_cue.model, shader);
}


