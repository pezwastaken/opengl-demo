#pragma once

#include <GLES2/gl2.h>
#include <assimp/scene.h>
#include <cassert>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <iostream>

#include "stb_image.h"

using u32 = uint32_t;
using u64 = uint64_t;

#define TODO assert(false)
#define UNREACHABLE assert(false)

#define MAX_ENTITIES 1000U

#define BLUE_MASK 0xFF
#define GREEN_MASK 0xFF00
#define RED_MASK 0xFF0000

#define N_CUBE_VERTICES 36U
static constexpr float cube_vertices[] = {
        // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};


inline void gen_texture(u32& texture, int texture_kind, unsigned char* data,
                        int width, int height, int n_channels)
{
    //generate texture object
    glGenTextures(1, &texture);

    //activate the 0th texture (usually active by default) and then bind the new texture to it
    //bind it so we can configure it
    glBindTexture(texture_kind, texture);

    //wrap mode
    glTexParameteri(texture_kind, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(texture_kind, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //downscaling and upscaling. on downscaling use mipmaps and on upscaling linear filtering
    glTexParameteri(texture_kind, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(texture_kind, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int channels = (n_channels == 4) ? GL_RGBA : GL_RGB;

    //generate texture
    glTexImage2D(texture_kind, 0, GL_RGB, width, height, 0, channels, GL_UNSIGNED_BYTE, data);

    //generate mipmaps automatically
    glGenerateMipmap(texture_kind);

    //free stb image
    stbi_image_free(data);
}

inline bool load_texture_from_file(const char* path, u32& texture, int texture_kind)
{
    assert(path);
    assert(texture_kind >= 0);
    assert(path[0] != '*');

    int width, height, n_channels;
    unsigned char* data;

    //load from file
    data = stbi_load(path, &width, &height, &n_channels, 0);
    if (!data)
    {
        std::clog << "couldn't load image at path: " << path << std::endl;
        return false;
    }

    gen_texture(texture, texture_kind, data, width, height, n_channels);
    return true;
}

//texture_kind: GL_TEXTURE_2D
inline bool load_texture_from_scene(const aiScene& scene, const char* path, u32& texture, int texture_kind)
{
    assert(path);
    assert(texture_kind >= 0);

    //load from memory (embedded texture, glb file for example)
    std::clog << "loading texture from memory" << std::endl;
    std::string texture_index_str = std::string(path + 1);
    std::clog << "texture index: " << texture_index_str << std::endl;

    int texture_index = std::stoi(texture_index_str);
    std::clog << "integer texture index: " << texture_index << std::endl;

    const aiTexture* texture_buffer = scene.mTextures[texture_index];

    std::clog << "texture has width: " << texture_buffer->mWidth << " and height: " <<
        texture_buffer->mHeight << std::endl;

    u32 buffer_size = 0U;
    //if compressed
    //
    if(texture_buffer->mHeight == 0)
    {
        buffer_size = texture_buffer->mWidth;
    }
    else
    {
        buffer_size = texture_buffer->mWidth * texture_buffer->mHeight;
    }

    std::clog << "texture format: " << texture_buffer->achFormatHint << std::endl;

    unsigned char* texels = reinterpret_cast<unsigned char*>(texture_buffer->pcData);

    int width, height, n_channels;
    unsigned char* data = stbi_load_from_memory(texels, buffer_size, &width, &height,
                                 &n_channels, 0);

    std::clog << "loaded texture has width: " << width << ", height: " << height <<
        ". number of channels: " << n_channels << std::endl;

    gen_texture(texture, texture_kind, data, width, height, n_channels);

    return true;
}

inline u32 color_vec3_to_u32(glm::vec3 color)
{
    u32 red = color.x * 255U;
    u32 green = color.y * 255U;
    u32 blue = color.z * 255U;

    return (blue | (green << 8) | (red << 16));
}

inline u32 color_rgb_bytes_to_u32(unsigned char pixel[3])
{
    u32 red = pixel[0];
    u32 green = pixel[1];
    u32 blue = pixel[2];
    return (red << 16) | (green << 8) | (blue);
}

