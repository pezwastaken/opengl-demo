#include "SDL3/SDL_mouse.h"
#include "key_handler.hh"
#include "shader.hh"
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

#include "utils.hh"


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


// static std::vector<u32> cube_indices;

// static Shader shader_program;
// static Shader light_shader;
// static Shader light_source_shader;

// static glm::mat4 model(1.0F);
static glm::mat4 view(1.0F);
static glm::mat4 projection;

static constexpr glm::vec3 origin (0.0, 0.0, 0.0);
static constexpr glm::vec3 world_up (0,1,0);
static constexpr glm::vec3 world_front (0,0,-1);
static constexpr glm::vec3 world_right (1,0,0);

struct Camera
{
    glm::vec3 position;
    glm::vec3 look_direction;
    glm::vec3 target;

    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 front;

    glm::mat4 lookat;

    float last_speed;

};

static Camera camera;
static constexpr float camera_speed = 4.0F;
static constexpr float mouse_sens_multiplier = 0.1F;

static bool in_relative_mode = false;

static KeyHandler key_handler {0};

static u64 current_frame_time = 0;
static u64 previous_frame_time = 0;

struct WorldObject
{
    u32 vao;
    u32 shader;
    glm::mat4 model;
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
};

static WorldData context {};

// std::vector<u32> get_indices_from_cube(u32 offset)
// {
//     std::vector<u32> indices;
//     //lower face
//     indices.push_back(offset + 0);
//     indices.push_back(offset + 1);
//     indices.push_back(offset + 2);
//
//     indices.push_back(offset + 1);
//     indices.push_back(offset + 2);
//     indices.push_back(offset + 3);
//
//     //upper face
//     indices.push_back(offset + 4);
//     indices.push_back(offset + 5);
//     indices.push_back(offset + 6);
//
//     indices.push_back(offset + 5);
//     indices.push_back(offset + 6);
//     indices.push_back(offset + 7);
//
//     //left side
//     indices.push_back(offset + 0);
//     indices.push_back(offset + 7);
//     indices.push_back(offset + 2);
//
//     indices.push_back(offset + 2);
//     indices.push_back(offset + 7);
//     indices.push_back(offset + 5);
//
//     //right side
//     indices.push_back(offset + 4);
//     indices.push_back(offset + 3);
//     indices.push_back(offset + 1);
//
//     indices.push_back(offset + 4);
//     indices.push_back(offset + 6);
//     indices.push_back(offset + 1);
//
//     //front face
//     indices.push_back(offset + 2);
//     indices.push_back(offset + 3);
//     indices.push_back(offset + 5);
//
//     indices.push_back(offset + 5);
//     indices.push_back(offset + 3);
//     indices.push_back(offset + 4);
//
//     //back face
//     indices.push_back(offset + 0);
//     indices.push_back(offset + 1);
//     indices.push_back(offset + 7);
//
//     indices.push_back(offset + 7);
//     indices.push_back(offset + 1);
//     indices.push_back(offset + 6);
//
//     return indices;
// }

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

WorldObject gen_random_cube(glm::vec3 center, float side, u32 shader_id, u32 vao_id) {

    (void) side;
    (void) center;

    glm::mat4 random_model(1.0F);
    auto random_vec = glm::ballRand(5.0F);
    random_model = glm::translate(random_model, random_vec);
    float random_scale = glm::linearRand(0.0F,3.0F);
    random_model = glm::scale(random_model, glm::vec3(random_scale));

    WorldObject random_cube {.vao = vao_id, .shader = shader_id, .model = random_model};
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


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    context.vbos.push_back(light_vbo);
    context.vaos.push_back(light_vao);

    context.objects.push_back(gen_random_cube(origin, 1.0F, light_source_shader_id,
                                              light_vao)
                              );
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

SDL_AppResult SDL_AppInit(void** app_state, int argc, char* argv[])
{

    SDL_SetAppMetadata("example", "1.0", "bau");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("couldn't initialize sdl");
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("example", 600, 400, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("couldn't create window/renderer");
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderLogicalPresentation(renderer, 600, 400, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_SetWindowRelativeMouseMode(window, true);

    std::clog << "window/renderer created" <<  std::endl;

    // imgui_init();

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::clog << "failed to initialize GLAD" << std::endl;
        return SDL_APP_FAILURE;
    }
    glViewport(0, 0, 600, 400);

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    context.vbos.push_back(VBO);
    context.vaos.push_back(VAO);


    //flip the images vertically on load
    stbi_set_flip_vertically_on_load(true);

    //load image
    int width, height, n_channels;
    unsigned char* data = stbi_load("textures/container.jpg", &width, &height, &n_channels, 0);
    if (!data)
    {
        std::clog << "couldn't load image " << std::endl;
        return SDL_APP_FAILURE;
    }


    //generate texture object
    u32 texture;
    glGenTextures(1, &texture);

    //activate the 0th texture (usually active by default) and then bind the new texture to it
    glActiveTexture(GL_TEXTURE0);
    //bind it so we can configure it
    glBindTexture(GL_TEXTURE_2D, texture);

    //wrap mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //downscaling and upscaling. on downscaling use mipmaps and on upscaling linear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //generate texture
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //generate mipmaps automatically
    glGenerateMipmap(GL_TEXTURE_2D);

    //free stb image
    stbi_image_free(data);



    //another image
    data = stbi_load("textures/awesomeface.png", &width, &height, &n_channels, 0);
    if (!data)
    {
        std::clog << "couldn't load image " << std::endl;
        return SDL_APP_FAILURE;
    }

    //generate texture object
    u32 texture2;
    glGenTextures(1, &texture2);

    //activate the 1th texture
    glActiveTexture(GL_TEXTURE1);
    //bind it so we can configure it
    glBindTexture(GL_TEXTURE_2D, texture2);

    //wrap mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //downscaling and upscaling. on downscaling use mipmaps and on upscaling linear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //generate texture
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    //generate mipmaps automatically
    glGenerateMipmap(GL_TEXTURE_2D);

    //free stb image
    stbi_image_free(data);


    //bind the correct VAO (suppose multiple objects)
    glBindVertexArray(VAO);


    projection = glm::perspective(glm::radians(45.0F), 600.0F / 400.0F, 0.1F, 100.0F);

    //setup camera
    camera = Camera {.position = glm::vec3(0.0F, 1.0F, 5.0F), .target = origin};
    camera.right = glm::vec3(1,0,0);
    camera.up = glm::vec3(0,1,0);
    camera.front = glm::vec3(0,0,-1);

    current_frame_time = SDL_GetTicksNS();
    previous_frame_time = current_frame_time;

    //shader used to render color + lightning
    Shader light_shader = Shader("shaders/lightning_vertex.glsl",
                                 "shaders/lightning_fragment.glsl");
    light_shader.use();
    light_shader.set_vec3("lightColor", glm::vec3(1,1,1));
    light_shader.set_vec3("objectColor", glm::vec3(1,0.5,0.31));
    light_shader.set_mat4("projection", projection);
    context.shaders.push_back(light_shader);

    u32 n_cubes = 3;
    const glm::vec3 center = origin;
    const float side = 1;
    for (u32 i = 0; i < n_cubes; ++i)
    {
        context.objects.push_back(gen_random_cube(center, side,
                                                  context.shaders.size() - 1, VAO));
    }


    //shader used to render light sources
    Shader light_source_shader = Shader("shaders/light_source_vertex.glsl",
                                        "shaders/light_source_fragment.glsl");
    light_source_shader.use();
    light_source_shader.set_vec3("lightColor", glm::vec3(1,1,1));
    light_source_shader.set_mat4("projection", projection);
    context.shaders.push_back(light_source_shader);
    create_light_source_data(context.shaders.size() - 1);

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
    // view = glm::lookAt(glm::vec3(cam_x, 0.0F, cam_z), camera.target, camera.up);

    view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);

    for (auto& shader : context.shaders)
    {
        shader.use();
        //buffer it
        // u32 view_loc = glGetUniformLocation(shader_program.m_id, "view");
        // glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        shader.set_mat4("view", view);

        // light_source_shader.use();
        // light_source_shader.set_mat4("view", view);
    }

    //debug camera position stuff
    static u64 delta_time_acc = 0;
    delta_time_acc += delta_time;
    if (delta_time_acc >= 10e8)
    {
        // std::clog << "delta time was: " << delta_time << std::endl;
        // std::clog << "coglione camera position: " << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << std::endl;
        // std::clog << "camera speed: " << camera.last_speed << std::endl;
        delta_time_acc = 0;
    }

}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    // ImGui_ImplOpenGL3_NewFrame();
    // ImGui_ImplSDL3_NewFrame();
    // ImGui::NewFrame();
    // ImGui::ShowDemoWindow(); // Show demo window! :)


    current_frame_time = SDL_GetTicksNS();
    u64 delta_time = current_frame_time - previous_frame_time;
    previous_frame_time = current_frame_time;


    glClearColor(0.2, 0.2, 0.2, 1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update_camera(delta_time);

    // glBindVertexArray(context.vaos[0]);


    //render objects

    for(auto& world_object : context.objects)
    {
        Shader& object_shader = context.shaders[world_object.shader];
        object_shader.use();
        object_shader.set_mat4("model", world_object.model);

        glBindVertexArray(world_object.vao);
        glDrawArrays(GL_TRIANGLES, 0, N_CUBE_VERTICES);
    }

    glBindVertexArray(0);

    //light source stuff
    // light_source_shader.use();
    // glBindVertexArray(context.vaos[1]);
    //
    // light_source_shader.set_mat4("model", model);
    //
    // glDrawElements(GL_TRIANGLES, context.objects_draw_indices[1].size(),
    //                GL_UNSIGNED_INT, 0);


    // ImGui::Render();
    // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // SDL_RenderPresent(renderer);
    SDL_GL_SwapWindow(window);


    glBindVertexArray(0);


    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult res)
{
    // ImGui_ImplOpenGL3_Shutdown();
    // ImGui_ImplSDL3_Shutdown();
    // ImGui::DestroyContext();
}


