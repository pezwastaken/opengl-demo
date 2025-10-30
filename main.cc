#include "SDL3/SDL_mouse.h"
#include <GLES2/gl2.h>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/random.hpp>
#include <string>
#include <vector>

#define SDL_MAIN_USE_CALLBACKS 1

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <glad/glad.h>

#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "utils.hh"
#include "key_handler.hh"
#include "shader.hh"
#include "mesh.hh"

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

// //vertices with colors
// static constexpr float vertices[] = {
//     0.5F, -0.5F, 0.0F, 1.0F, 0.0F, 0.0F,
//     -0.5F, -0.5F, 0.0F, 0.0F, 1.0F, 0.0F,
//     0.0F, 0.5F, 0.0F, 0.0F, 0.0F, 1.0F
// };

//rectangle with colors and texture coordinates (starting from bottom left, clockwise)
// static constexpr float vertices[] = {
//     0.5F, -0.5F, 0.0F,     1.0F, 0.0F,
//     -0.5F, -0.5F, 0.0F,    0.0F, 0.0F,
//     -0.5F, +0.5F, 0.0F,    0.0F, 1.0F,
//     0.5F, 0.5F, 0.0F,      1.0F, 1.0F
// };

static constexpr float border_color[] = {
    1.0F, 1.0F, 1.0F
};


static glm::mat4 view(1.0F);
static glm::mat4 projection;

static constexpr glm::vec3 origin (0.0, 0.0, 0.0);
static constexpr glm::vec3 world_up (0,1,0);
static constexpr glm::vec3 world_front (0,0,-1);
static constexpr glm::vec3 world_right (1,0,0);

struct Camera
{
    glm::vec3 position;

    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 front;

    glm::mat4 lookat;

    float last_speed;
};

static Camera camera;
static constexpr float camera_speed = 6.0F;
static constexpr float mouse_sens_multiplier = 0.1F;

static bool in_relative_mode = false;

static KeyHandler key_handler {0};

static u64 current_frame_time = 0;
static u64 previous_frame_time = 0;

struct Material
{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 reflection;
	float shininess;
};


// mainly used by point lights.
// direction could be used by spot lights. Same thing goes for cut off.
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

    LightData light;

    Model model;

    u32 vao;
    u32 shader;
    u32 material_id;
};

struct WorldData
{
    std::vector<u32> vaos;
    std::vector<u32> vbos;
    std::vector<u32> ebos;

    // std::vector<std::vector<u32>> objects_draw_indices;

    // std::vector<glm::mat4> models;

    std::vector<Shader> shaders;

    std::vector<WorldObject> objects;
    std::vector<WorldObject> light_emitting_objects;

    std::vector<LightData> directional_lights;

    std::vector<Material> materials;
};

static WorldData context {};

inline void push_floats(std::vector<float>& v, const glm::vec3& x)
{
    v.push_back(x.x);
    v.push_back(x.y);
    v.push_back(x.z);
}
inline void push_floats(std::vector<float>& v, const glm::vec2& x)
{
    v.push_back(x.x);
    v.push_back(x.y);
}

WorldObject gen_terrain(u32 shader_id, u32 vao_id)
{
    glm::mat4 model(1.0F);

    const glm::vec3 position(0,-30,0);
    model = glm::translate(model, position);
    float random_scale = 50.0F;
    model = glm::scale(model, glm::vec3(random_scale));

    WorldObject random_cube {.model_matrix = model,
        .position = position, .vao = vao_id, .shader = shader_id, .material_id = 0};

    return random_cube;
};

//TODO: position should be the model matrix, just like the other objects
WorldObject gen_random_cube(glm::vec3 center, float side, u32 shader_id, u32 vao_id)
{

    (void) side;
    (void) center;

    glm::mat4 random_model(1.0F);
    auto random_vec = glm::ballRand(8.0F);
    random_model = glm::translate(random_model, random_vec);

    auto random_rotation_axis = glm::ballRand(1.0F);
    random_model = glm::rotate(random_model, glm::radians((float)glm::linearRand(0,180)),
                               random_rotation_axis);

    float random_scale = glm::linearRand(0.0F,3.0F);
    random_model = glm::scale(random_model, glm::vec3(random_scale));

    //FIXME: material id
    WorldObject random_cube {.model_matrix = random_model,
        .position = random_vec, .vao = vao_id, .shader = shader_id, .material_id = 0};

    return random_cube;
}


u32 create_shader_program_obj(std::vector<uint32_t> shaders)
{
    u32 shader_program;
    shader_program = glCreateProgram();

    glAttachShader(shader_program, shaders[0]);
    glAttachShader(shader_program, shaders[1]);
    glLinkProgram(shader_program);

    int success;
    char info_log[512];
    glGetShaderiv(shader_program, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader_program, 512, NULL, info_log);
        std::clog << "vertex shader compilation failed: " << info_log << std::endl;
        exit(1);
    }

    return shader_program;
}

void create_light_source_data(u32 light_source_shader_id)
{

    u32 light_vbo;
    glGenBuffers(1, &light_vbo);

    u32 light_vao;
    glGenVertexArrays(1, &light_vao);

    glBindVertexArray(light_vao);

    glBindBuffer(GL_ARRAY_BUFFER, light_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices),
                 cube_vertices, GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    context.vbos.push_back(light_vbo);
    context.vaos.push_back(light_vao);

    const u32 n_light_sources = 1;

    glm::vec3 colors[n_light_sources] = 
    {
        {1,0,0},
            // {0,0,1},{0,0.5,0.5}, {0,0.8,0.8}, {0,0.8, 0.4}
    };

    for (u32 i = 0; i < n_light_sources; ++i)
    {
        WorldObject light_source =
            gen_random_cube(origin, 1.0F, light_source_shader_id, light_vao);

        auto sample_color = colors[i];

        light_source.light = LightData {
            .source_color = sample_color,
            .ambient = 0.2F * sample_color,
            .diffuse = 0.5F * sample_color,
            .reflection = glm::vec3(1.0F),
            .constant = 1.0F,
            .linear = 0.09F,
            .quadratic = 0.032F
        };

        std::clog << "generating light source at: [ " << light_source.position.x
            << ", " << light_source.position.y << ", " << light_source.position.z << "]" << std::endl;

        context.light_emitting_objects.push_back(light_source);
    }
}

void create_directional_light_source(u32 light_source_shader_id)
{
    glm::vec3 sample_color {1,1,1};

    LightData directional_light {
        .direction = glm::normalize(glm::vec3(-1,-1,0)),
        .source_color = sample_color,
        .ambient = 0.2F * sample_color,
        .diffuse = 0.5F * sample_color,
        .reflection = glm::vec3(1.0F),
    };

    context.directional_lights.push_back(directional_light);
}

void update_light_uniforms(Shader& shader)
{
    for(u32 i = 0; i < context.light_emitting_objects.size(); ++i)
    {
        const auto& light_object = context.light_emitting_objects[i];
        const std::string uniform_base_name = "pointLights";
        auto uniform_light = uniform_base_name + "[" + std::to_string(i) + "]";

        shader.set_vec3(uniform_light + ".position", light_object.position);
        shader.set_vec3(uniform_light + ".ambient", light_object.light.ambient);
        shader.set_vec3(uniform_light + ".diffuse", light_object.light.diffuse);
        shader.set_vec3(uniform_light + ".reflection", light_object.light.reflection);
        shader.set_float(uniform_light + ".constant", light_object.light.constant);
        shader.set_float(uniform_light + ".linear", light_object.light.linear);
        shader.set_float(uniform_light + ".quadratic", light_object.light.quadratic);
    }

    const auto& directional_light = context.directional_lights[0];

    shader.set_vec3("directionalLight.direction", directional_light.direction);
    shader.set_vec3("directionalLight.ambient", directional_light.ambient);
    shader.set_vec3("directionalLight.diffuse", directional_light.diffuse);
    shader.set_vec3("directionalLight.reflection", directional_light.reflection);
}


void imgui_init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, NULL);
    ImGui_ImplOpenGL3_Init();

}

static u32 texture1;
static u32 texture2;
static u32 texture3;

SDL_AppResult SDL_AppInit(void** app_state, int argc, char* argv[])
{

    SDL_SetAppMetadata("example", "1.0", "bau");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("couldn't initialize sdl");
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("example", 1920, 1080, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("couldn't create window/renderer");
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderLogicalPresentation(renderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_SetWindowRelativeMouseMode(window, true);

    std::clog << "window/renderer created" <<  std::endl;

    imgui_init();

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::clog << "failed to initialize GLAD" << std::endl;
        return SDL_APP_FAILURE;
    }
    glViewport(0, 0, 1920, 1080);

    glEnable(GL_DEPTH_TEST);

    srand(time(nullptr));



    //create VBO buffer
    u32 VBO;
    glGenBuffers(1, &VBO);

    //VAO stuff
    u32 VAO;
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices,
                 GL_STATIC_DRAW);

    std::clog << "size of cube_vertices: " << N_CUBE_VERTICES << std::endl;

    //vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //normal vectors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    context.vbos.push_back(VBO);
    context.vaos.push_back(VAO);


    //flip the images vertically on load
    stbi_set_flip_vertically_on_load(true);


    //load default pink diffuse map and default specular map.
    //used by models that don't have any
    //
    load_default_textures();

    //
    // //diffuse map
    // // u32 texture2;
    // texture_loaded = load_texture("textures/container2.png", texture2, GL_TEXTURE_2D);
    // if (!texture_loaded)
    // {
    //     return SDL_APP_FAILURE;
    // }
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, texture2);
    //
    // //specular map
    // // u32 texture3;
    // texture_loaded = load_texture("textures/container2_specular.png", texture3, GL_TEXTURE_2D);
    // if (!texture_loaded)
    // {
    //     return SDL_APP_FAILURE;
    // }
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_2D, texture3);


    //bind the correct VAO (suppose multiple objects)
    glBindVertexArray(VAO);


    projection = glm::perspective(glm::radians(45.0F), 600.0F / 400.0F, 0.1F, 100.0F);

    //setup camera
    camera = Camera {.position = glm::vec3(0.0F, 1.0F, 5.0F), };
    camera.right = glm::vec3(1,0,0);
    camera.up = glm::vec3(0,1,0);
    camera.front = glm::vec3(0,0,-1);

    current_frame_time = SDL_GetTicksNS();
    previous_frame_time = current_frame_time;


    //push default material
    Material material {.ambient = {1.0, 0.5, 0.31}, .diffuse = {1.0, 0.5, 0.31},
                       .reflection = {0.5, 0.5, 0.5}, .shininess = 32.0F};

    context.materials.push_back(material);

    //shader used to render light sources
    Shader light_source_shader = Shader("shaders/light_source_vertex.glsl",
                                        "shaders/light_source_fragment.glsl");
    light_source_shader.use();
    light_source_shader.set_vec3("lightColor", glm::vec3(1,1,1));
    light_source_shader.set_mat4("projection", projection);
    context.shaders.push_back(light_source_shader);
    create_light_source_data(context.shaders.size() - 1);
    create_directional_light_source(context.shaders.size() - 1);

    const int n_light_sources = context.light_emitting_objects.size();


    //shader used to render color + lightning
    Shader light_shader = Shader("shaders/lightning_vertex.glsl",
                                 "shaders/lightning_fragment.glsl");
    light_shader.use();
    light_shader.set_mat4("projection", projection);

    // light_shader.set_int("material.texture_diffuse", 1);
    // light_shader.set_int("material.texture_specular", 2);

    light_shader.set_int("nPointLights", n_light_sources);

    update_light_uniforms(light_shader);

    light_shader.set_vec3("spotLight.position", camera.position);
    light_shader.set_vec3("spotLight.direction", camera.front);
    light_shader.set_vec3("spotLight.ambient", glm::vec3(0.2,0.2,0.2));
    light_shader.set_vec3("spotLight.diffuse", glm::vec3(0.5, 0.5, 0.5));
    light_shader.set_vec3("spotLight.reflection", glm::vec3(1, 1, 1));
    // light_shader.set_float("spotLight.constant", 1.0F);
    // light_shader.set_float("spotLight.linear", 0.09F);
    // light_shader.set_float("spotLight.quadratic", 0.032F);
    light_shader.set_float("spotLight.innerCutOff", glm::cos(glm::radians(12.5F)));
    light_shader.set_float("spotLight.outerCutOff", glm::cos(glm::radians(17.5F)));

    context.shaders.push_back(light_shader);

    // u32 n_cubes = 10;
    // const glm::vec3 center = origin;
    // const float side = 1;
    // for (u32 i = 0; i < n_cubes; ++i)
    // {
    //     context.objects.push_back(gen_random_cube(center, side,
    //                                               context.shaders.size() - 1, VAO));
    // }

    // context.objects.push_back(gen_terrain(context.shaders.size() - 1, VAO));

    // setup model 1
    Model model1;
    model_init(model1, "models/diao-chan/source/DiaoChann.glb");

    WorldObject sword;
    sword.model_matrix = glm::mat4(1.0F);
    sword.model_matrix = glm::translate(sword.model_matrix, glm::vec3(0,3,3));
    sword.model_matrix = glm::scale(sword.model_matrix, glm::vec3(100));

    sword.model = model1;
    sword.shader = context.shaders.size() - 1;

    context.objects.push_back(sword);


    std::clog << " -------------- \n\n";

    Model env_model;
    model_init(env_model, "models/environment/scene.gltf");
    WorldObject road;
    road.model_matrix = glm::mat4(1.0F);
    road.model_matrix = glm::translate(road.model_matrix, glm::vec3(0,3,0));
    road.model_matrix = glm::scale(road.model_matrix, glm::vec3(2));

    road.model = env_model;
    road.shader = context.shaders.size() - 1;

    context.objects.push_back(road);



    light_shader.use();

    std::clog << "n world objects: " << context.objects.size() << std::endl;

    return SDL_APP_CONTINUE;
}

void camera_rotate(float delta_x, float delta_y)
{
    glm::quat delta_yaw = glm::angleAxis(glm::radians(delta_x), world_up);
    glm::quat delta_pitch = glm::angleAxis(glm::radians(delta_y), camera.right);

    glm::quat delta_rotation = delta_yaw * delta_pitch;
    delta_rotation = glm::normalize(delta_rotation);

    camera.right = delta_rotation * camera.right;
    camera.up = delta_rotation * camera.up;
    camera.front = delta_rotation * camera.front;
}

SDL_AppResult SDL_AppEvent(void* app_state, SDL_Event* event)
{

    // ImGui_ImplSDL3_ProcessEvent(event);
    // ImGuiIO& io = ImGui::GetIO();

    // if (io.WantCaptureMouse)
    // {
    //     std::clog << "want to capture mouse" << std::endl;
    //     return SDL_APP_CONTINUE;
    // }

    switch (event->type)
    {

        case SDL_EVENT_QUIT:
        {
            std::clog << "exiting" << std::endl;
            return SDL_APP_SUCCESS;
        }

        case SDL_EVENT_WINDOW_RESIZED:
        {
            int w = event->window.data1;
            int h = event->window.data2;

            glViewport(0, 0, w, h);
            std::clog << "gl viewport resized to: " << w << " , " << h << std::endl;
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            set_key_pressed(key_handler, event->key.scancode);
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            set_key_released(key_handler, event->key.scancode);
            break;
        }

        case SDL_EVENT_MOUSE_MOTION:
        {
            //dragging mouse while holding left click rotates camera
            auto delta_x = -event->motion.xrel * mouse_sens_multiplier;
            auto delta_y = -event->motion.yrel * mouse_sens_multiplier;

            // std::clog << "mouse relative movement x: " << event->motion.xrel << "\n";
            // std::clog << "mouse relative movement y: " << event->motion.yrel << std::endl;
            camera_rotate(delta_x, delta_y);
            break;
        }

        default:
            break;
    }

    return SDL_APP_CONTINUE;

}

void move_camera(u64 delta_time)
{
    float speed = camera_speed * (delta_time/10e8);
    if (get_key_state(key_handler, SDL_SCANCODE_W))
    {
        camera.position += speed * camera.front;
    }
    if (get_key_state(key_handler, SDL_SCANCODE_S))
    {
        camera.position -= speed * camera.front;
    }
    if (get_key_state(key_handler, SDL_SCANCODE_D))
    {
        camera.position += speed * camera.right;
    }
    if (get_key_state(key_handler, SDL_SCANCODE_A))
    {
        camera.position -= speed * camera.right;
    }
    if (get_key_state(key_handler, SDL_SCANCODE_E))
    {
        camera.position += speed * camera.up;
    }
    if (get_key_state(key_handler, SDL_SCANCODE_Q))
    {
        camera.position -= speed * camera.up;
    }


    camera.last_speed = speed;
}

void update_camera(u64 delta_time)
{

    move_camera(delta_time);

    //update view matrix
    float radius = 10.0F;

    view = glm::mat4(1.0F);

    // float time = (SDL_GetTicks() / 500.0F);
    // float cam_x = glm::sin(time) * radius;
    // float cam_z = glm::cos(time) * radius;

    view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);

    for (auto& shader : context.shaders)
    {
        shader.use();
        shader.set_mat4("view", view);

    }

    //debug camera position stuff
    static u64 delta_time_acc = 0;
    delta_time_acc += delta_time;
    if (delta_time_acc >= 10e8)
    {
        // std::clog << "camera position: " << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << std::endl;
        // std::clog << "camera speed: " << camera.last_speed << std::endl;
        delta_time_acc = 0;
    }

}

void show_debug_info()
{
    // Transparent background
    ImGui::SetNextWindowBgAlpha(0.35f);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("Debug overlay", NULL, window_flags))
    {
        // IMGUI_DEMO_MARKER("Examples/Simple Overlay");
        ImGui::Text("Debug info\n");
        ImGui::Separator();
        ImGui::Text("Camera position: (%.1f, %.1f, %.1f)",
                    camera.position.x, camera.position.y, camera.position.z);

        ImGui::Text("Camera front direction: (%.1f, %.1f, %.1f)",
                    camera.front.x, camera.front.y, camera.front.z);
    }
    ImGui::End();
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    show_debug_info();

    //IMGUI stuff
    // ImGui::ShowDemoWindow(); // Show demo window! :)

    current_frame_time = SDL_GetTicksNS();
    u64 delta_time = current_frame_time - previous_frame_time;
    previous_frame_time = current_frame_time;

    glClearColor(0.1, 0.1, 0.1, 1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update_camera(delta_time);

    context.shaders[1].set_vec3("viewPosition", camera.position);

    context.shaders[1].set_vec3("spotLight.position", camera.position);
    context.shaders[1].set_vec3("spotLight.direction", camera.front);

    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, texture2);
    //
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_2D, texture3);

    //render objects
    // for(auto& world_object : context.objects)
    // {
    //     Shader& object_shader = context.shaders[world_object.shader];
    //     object_shader.use();
    //     object_shader.set_mat4("model", world_object.model_matrix);
    //
    //     if (object_shader.has_uniform("material.shininess"))
    //     {
    //         const auto& material = context.materials[world_object.material_id];
    //         object_shader.set_vec3("material.texture_specular", material.reflection);
    //         object_shader.set_float("material.shininess", material.shininess);
    //     }
    //
    //     glBindVertexArray(world_object.vao);
    //     glDrawArrays(GL_TRIANGLES, 0, N_CUBE_VERTICES);
    // }

    assert(context.shaders.size() == 2);

    //3d models
    for(auto& world_object : context.objects)
    {
        Shader& object_shader = context.shaders[world_object.shader];
        object_shader.use();
        object_shader.set_mat4("model", world_object.model_matrix);

        object_shader.set_float("material.shininess", 32.0F);

        model_draw(world_object.model, object_shader);
    }



    //render light emitting objects
    for(auto& light_world_object : context.light_emitting_objects)
    {
        Shader& object_shader = context.shaders[light_world_object.shader];
        object_shader.use();
        object_shader.set_mat4("model", light_world_object.model_matrix);
        object_shader.set_vec3("lightColor", light_world_object.light.source_color);

        glBindVertexArray(light_world_object.vao);
        glDrawArrays(GL_TRIANGLES, 0, N_CUBE_VERTICES);
    }


    glBindVertexArray(0);


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // SDL_RenderPresent(renderer);
    SDL_GL_SwapWindow(window);


    glBindVertexArray(0);


    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult res)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}


