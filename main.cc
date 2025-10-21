#include "shader.hh"
#include <GLES2/gl2.h>
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
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

static std::vector<u32> cube_indices;

static Shader shader_program;

static glm::mat4 model(1.0F);
static glm::mat4 view(1.0F);
static glm::mat4 projection;


static constexpr glm::vec3 origin (0.0, 0.0, 0.0);

struct Camera
{
    glm::vec3 position;
    glm::vec3 look_direction;
    glm::vec3 target;

    glm::vec3 right;
    glm::vec3 up;

    glm::mat4 lookat;

};

std::vector<u32> get_indices_from_cube()
{
    std::vector<u32> cube_indices;

    //lower face
    cube_indices.push_back(0);
    cube_indices.push_back(1);
    cube_indices.push_back(2);

    cube_indices.push_back(1);
    cube_indices.push_back(2);
    cube_indices.push_back(3);

    //upper face
    cube_indices.push_back(4);
    cube_indices.push_back(5);
    cube_indices.push_back(6);

    cube_indices.push_back(5);
    cube_indices.push_back(6);
    cube_indices.push_back(7);

    //left side
    cube_indices.push_back(0);
    cube_indices.push_back(7);
    cube_indices.push_back(2);

    cube_indices.push_back(2);
    cube_indices.push_back(7);
    cube_indices.push_back(5);

    //right side
    cube_indices.push_back(4);
    cube_indices.push_back(3);
    cube_indices.push_back(1);

    cube_indices.push_back(4);
    cube_indices.push_back(6);
    cube_indices.push_back(1);

    //front face
    cube_indices.push_back(2);
    cube_indices.push_back(3);
    cube_indices.push_back(5);

    cube_indices.push_back(5);
    cube_indices.push_back(3);
    cube_indices.push_back(4);

    //back face
    cube_indices.push_back(0);
    cube_indices.push_back(1);
    cube_indices.push_back(7);

    cube_indices.push_back(7);
    cube_indices.push_back(1);
    cube_indices.push_back(6);

    return cube_indices;
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

std::vector<float> gen_cube(glm::vec3 center, float side)
{
    std::vector<float> vertices;
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


    return vertices;
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

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::clog << "failed to initialize GLAD" << std::endl;
        return SDL_APP_FAILURE;
    }
    glViewport(0, 0, 600, 400);

    glEnable(GL_DEPTH_TEST);

    std::vector<float> cube_vertices = gen_cube(glm::vec3(0, 0, 0.0), 1.0);

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

    cube_indices = get_indices_from_cube();
    //buffer the data
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_indices.size() * sizeof(u32), cube_indices.data(), GL_STATIC_DRAW);



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

    //view matrix: move everything in front so that it is the same as moving
    //the camera backwards. Also move everything down
    view = glm::translate(view, glm::vec3(0.0F, -1.0F, -5.0F));

    projection = glm::perspective(glm::radians(45.0F), 600.0F / 400.0F, 0.1F, 100.0F);

    //send matrices to the shader
    //
    // u32 model_loc = glGetUniformLocation(shader_program.m_id, "model");
    // glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

    u32 view_loc = glGetUniformLocation(shader_program.m_id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

    u32 projection_loc = glGetUniformLocation(shader_program.m_id, "projection");
    glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));


    Camera camera {.position = glm::vec3(0.0, 0.0, 3.0F), .target = origin};
    camera.look_direction = camera.target - camera.position;

    glm::vec3 up (0,1,0);
    camera.right = glm::normalize(glm::cross(up, camera.look_direction));

    camera.up = glm::normalize(glm::cross(camera.look_direction, camera.right));

    //lookAt computes the lookat matrix which translates any vector to the "camera" space.
    camera.lookat = glm::lookAt(camera.position, camera.target, camera.up);


    return SDL_APP_CONTINUE;
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

    return SDL_APP_CONTINUE;

}

SDL_AppResult SDL_AppIterate(void *appstate)
{

    float time = ((float)SDL_GetTicks()/500.0F);

    //identity matrix
    model = glm::mat4(1.0F);

    //add a translation component to the 'translation' matrix
    // transform = glm::scale(transform, glm::vec3(0.5F, 0.5F, 0.5F));
    model = glm::translate(model, glm::vec3(0.0F, 1, 0.0F));
    //rotate 
    model = glm::rotate(model, glm::radians(50 * time),
                            glm::normalize(glm::vec3(0.5F, 1.0F, 0.0F)));


    u32 model_loc = glGetUniformLocation(shader_program.m_id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

    //set the transform uniform
    // u32 transform_location = glGetUniformLocation(shader_program.m_id, "transform");
    // glUniformMatrix4fv(transform_location, 1, GL_FALSE, glm::value_ptr(transform));


    const double now = ((double)SDL_GetTicks()) / 1000.0;

    const float red = 0.5 + 0.5 * SDL_sin(now);

    glClearColor(red, 0.0,0.0,1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    //draw using vertices from the bound VBO
    // glDrawArrays(GL_TRIANGLES, 0, 3);

    //draw using the indices contained in the active EBO
    glDrawElements(GL_TRIANGLES, cube_indices.size(), GL_UNSIGNED_INT, 0);

    // SDL_SetRenderDrawColorFloat(renderer, red, 0.0F, 0.0F, SDL_ALPHA_OPAQUE_FLOAT);
    // SDL_RenderClear(renderer);
    //
    // SDL_FRect rect;
    // SDL_SetRenderDrawColor(renderer, 0,0,255, SDL_ALPHA_OPAQUE);
    // rect.x = rect.y = 100;
    // rect.w = 100;
    // rect.h = 50;
    // SDL_RenderFillRect(renderer, &rect);
    //

    SDL_RenderPresent(renderer);

    //reset VAO
    // glBindVertexArray(0);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult res)
{
}


