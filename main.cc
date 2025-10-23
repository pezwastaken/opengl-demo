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
static constexpr float vertices[] = {
    0.5F, -0.5F, 0.0F,     1.0F, 0.0F,
    -0.5F, -0.5F, 0.0F,    0.0F, 0.0F,
    -0.5F, +0.5F, 0.0F,    0.0F, 1.0F,
    0.5F, 0.5F, 0.0F,      1.0F, 1.0F
};

static constexpr float border_color[] = {
    1.0F, 1.0F, 1.0F
};

//triangle
// static constexpr u32 indices[] = {
//     0, 1, 2,
// };

//indices for rectangle
// static constexpr u32 indices[] = {
//     0, 1, 2,
//     2, 3, 0
// };

// static std::vector<u32> cube_indices;

static Shader shader_program;
static Shader light_source_shader;

static glm::mat4 model(1.0F);
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

static KeyHandler key_handler {0};

static u64 current_frame_time = 0;
static u64 previous_frame_time = 0;


struct WorldData
{
    std::vector<u32> vaos;
    std::vector<u32> vbos;
    std::vector<u32> ebos;

    std::vector<std::vector<u32>> objects_draw_indices;
};

static WorldData context {};

std::vector<u32> get_indices_from_cube(std::vector<u32>& indices, u32 offset)
{
    //lower face
    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 2);

    indices.push_back(offset + 1);
    indices.push_back(offset + 2);
    indices.push_back(offset + 3);

    //upper face
    indices.push_back(offset + 4);
    indices.push_back(offset + 5);
    indices.push_back(offset + 6);

    indices.push_back(offset + 5);
    indices.push_back(offset + 6);
    indices.push_back(offset + 7);

    //left side
    indices.push_back(offset + 0);
    indices.push_back(offset + 7);
    indices.push_back(offset + 2);

    indices.push_back(offset + 2);
    indices.push_back(offset + 7);
    indices.push_back(offset + 5);

    //right side
    indices.push_back(offset + 4);
    indices.push_back(offset + 3);
    indices.push_back(offset + 1);

    indices.push_back(offset + 4);
    indices.push_back(offset + 6);
    indices.push_back(offset + 1);

    //front face
    indices.push_back(offset + 2);
    indices.push_back(offset + 3);
    indices.push_back(offset + 5);

    indices.push_back(offset + 5);
    indices.push_back(offset + 3);
    indices.push_back(offset + 4);

    //back face
    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 7);

    indices.push_back(offset + 7);
    indices.push_back(offset + 1);
    indices.push_back(offset + 6);

    return indices;
}

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

void gen_cube(std::vector<float>& vertices,
              glm::vec3 center, float side)
{
    const float half = side/2.0F;

    glm::vec3 min = center - half;

    push_floats(vertices, min);
    push_floats(vertices, glm::vec2(0.0F, 0.0F));

    //lower face
    push_floats(vertices, min + glm::vec3(side, 0.0F, 0.0F));
    push_floats(vertices, glm::vec2(1.0F, 0.0F));

    push_floats(vertices, min + glm::vec3(0.0F, 0.0F, side));
    push_floats(vertices, glm::vec2(0.0F, 1.0F));

    push_floats(vertices, min + glm::vec3(side, 0.0F, side));
    push_floats(vertices, glm::vec2(1.0F, 1.0F));


    //upper face
    glm::vec3 max = center + half;

    push_floats(vertices, max);
    push_floats(vertices, glm::vec2(0.0F, 0.0F));

    push_floats(vertices, max - glm::vec3(side, 0.0F, 0.0F));
    push_floats(vertices, glm::vec2(1.0F, 0.0F));

    push_floats(vertices, max - glm::vec3(0.0F, 0.0F, side));
    push_floats(vertices, glm::vec2(0.0F, 1.0F));

    push_floats(vertices, max - glm::vec3(side, 0.0F, side));
    push_floats(vertices, glm::vec2(1.0F, 1.0F));

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

void create_light_source_data()
{
    std::vector<float> light_cube_vertices;
    std::vector<u32>& light_cube_indices = context.objects_draw_indices[1];
    glm::vec3 center = {1,0,-2};
    float side = 1.0F;
    gen_cube(light_cube_vertices, center, side);
    context.objects_draw_indices.push_back({});
    get_indices_from_cube(light_cube_indices, 0);

    u32 light_vbo;
    glGenBuffers(1, &light_vbo);

    u32 light_vao;
    glGenVertexArrays(1, &light_vao);

    u32 light_ebo;
    glGenBuffers(1, &light_ebo);

    glBindVertexArray(light_vao);

    glBindBuffer(GL_ARRAY_BUFFER, light_vbo);
    glBufferData(GL_ARRAY_BUFFER, light_cube_vertices.size() * sizeof(float),
                 light_cube_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, light_cube_indices.size() * sizeof(u32),
                 light_cube_indices.data(), GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    //probably don't need the ebo since we already have one for the previous cubes
    context.vbos.push_back(light_vbo);
    context.vaos.push_back(light_vao);
    context.ebos.push_back(light_ebo);
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

    std::clog << "window/renderer created" <<  std::endl;

    SDL_SetWindowRelativeMouseMode(window, true);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::clog << "failed to initialize GLAD" << std::endl;
        return SDL_APP_FAILURE;
    }
    glViewport(0, 0, 600, 400);

    glEnable(GL_DEPTH_TEST);

    srand(time(nullptr));

    std::vector<float> cube_vertices;
    u32 n_cubes = 3;
    glm::vec3 centers[3] = {{0,0,0}, glm::sphericalRand(3.0F), glm::sphericalRand(3.0F)};
    for (u32 i = 0; i < n_cubes; ++i)
    {
        float side = glm::linearRand(0.0F,2.0F);
        gen_cube(cube_vertices, centers[i], side);
        context.objects_draw_indices.push_back({});
        get_indices_from_cube(context.objects_draw_indices[0], i*8);
    }


    //create VBO buffer
    u32 VBO;
    glGenBuffers(1, &VBO);

    //VAO stuff
    u32 VAO;
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, cube_vertices.size() * sizeof(float), cube_vertices.data(), GL_STATIC_DRAW);

    std::cout << "size of cube_vertices: " << cube_vertices.size() << std::endl;
    std::cout << "size of vertices: " << sizeof(vertices) << std::endl;

    //vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //EBO stuff
    u32 EBO;
    glGenBuffers(1, &EBO);

    //binding a GL_ELEMENT_ARRAY_BUFFER means that next time we are goind to draw,
    //we are gonna be using its indices.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    //NOTE that by binding the EBO while a certain VAO is bound the EBO gets automatically
    //linked to that EBO. This means that binding the VAO will automatically bind the EBO.

    //buffer the data
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 context.objects_draw_indices[0].size() * sizeof(u32),
                 context.objects_draw_indices[0].data(), GL_STATIC_DRAW);


    context.vbos.push_back(VBO);
    context.vaos.push_back(VAO);
    context.ebos.push_back(EBO);


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


    shader_program = Shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
    shader_program.use();
    shader_program.set_int("texture1", 0);
    shader_program.set_int("texture2", 1);


    //bind the correct VAO (suppose multiple objects)
    glBindVertexArray(VAO);

    //model matrix
    //tilt the objects by -90 degrees along the x axis
    // model = glm::rotate(model, glm::radians(-45.0F),
    //                     glm::normalize(glm::vec3(1.0F, 1.0F, 0.0F)));

    projection = glm::perspective(glm::radians(45.0F), 600.0F / 400.0F, 0.1F, 100.0F);

    u32 projection_loc = glGetUniformLocation(shader_program.m_id, "projection");
    glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));


    //setup camera
    camera = Camera {.position = glm::vec3(0.0F, 1.0F, 0.0F), .target = origin};
    camera.right = glm::vec3(1,0,0);
    camera.up = glm::vec3(0,1,0);
    camera.front = glm::vec3(0,0,-1);

    current_frame_time = SDL_GetTicksNS();
    previous_frame_time = current_frame_time;

    //set up lights
    create_light_source_data();
    light_source_shader = Shader("shaders/light_source_vertex.glsl",
                                 "shaders/light_source_fragment.glsl");
    light_source_shader.use();
    light_source_shader.set_vec3("lightColor", glm::vec3(1,1,1));
    light_source_shader.set_vec3("objectColor", glm::vec3(1,0,0));
    light_source_shader.set_mat4("projection", projection);

    shader_program.use();

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
    if (event->type == SDL_EVENT_QUIT)
    {
        std::clog << "exiting" << std::endl;
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        int w = event->window.data1;
        int h = event->window.data2;

        glViewport(0, 0, w, h);
        std::clog << "gl viewport resized to: " << w << " , " << h << std::endl;
    }

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        set_key_pressed(key_handler, event->key.scancode);
    }
    if (event->type == SDL_EVENT_KEY_UP)
    {
        set_key_released(key_handler, event->key.scancode);
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        auto delta_x = -event->motion.xrel * mouse_sens_multiplier;
        auto delta_y = -event->motion.yrel * mouse_sens_multiplier;

        // std::clog << "mouse relative movement x: " << event->motion.xrel << "\n";
        // std::clog << "mouse relative movement y: " << event->motion.yrel << std::endl;

        camera_rotate(delta_x, delta_y);
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

    shader_program.use();
    //buffer it
    // u32 view_loc = glGetUniformLocation(shader_program.m_id, "view");
    // glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    shader_program.set_mat4("view", view);


    light_source_shader.use();
    light_source_shader.set_mat4("view", view);

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
    current_frame_time = SDL_GetTicksNS();
    u64 delta_time = current_frame_time - previous_frame_time;
    previous_frame_time = current_frame_time;


    update_camera(delta_time);

    //model matrix (uniform)
    model = glm::mat4(1.0F);
    model = glm::translate(model, glm::vec3(0.0F, 1, -2.0F));


    shader_program.use();
    shader_program.set_mat4("model", model);

    glBindVertexArray(context.vaos[0]);
    // glBindBuffer(GL_ARRAY_BUFFER, context.vbos[0]);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context.ebos[0]);

    glClearColor(0.8, 0.8, 1.0, 1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //draw using the indices contained in the active EBO
    glDrawElements(GL_TRIANGLES, context.objects_draw_indices[0].size(),
                   GL_UNSIGNED_INT, 0);


    //light stuff
    light_source_shader.use();
    glBindVertexArray(context.vaos[1]);

    light_source_shader.set_mat4("model", model);

    glDrawElements(GL_TRIANGLES, context.objects_draw_indices[1].size(),
                   GL_UNSIGNED_INT, 0);


    SDL_RenderPresent(renderer);

    glBindVertexArray(0);


    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult res)
{
}


