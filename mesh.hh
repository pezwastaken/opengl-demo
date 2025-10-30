#pragma once

#include "shader.hh"
#include "utils.hh"
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <map>
#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};

struct Texture
{
    u32 id;
    enum TextureKind
    {
        TEXTURE_DIFFUSE,
        TEXTURE_SPECULAR,
    };

    TextureKind kind;
    std::string path;

};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::vector<Texture> textures;

    u32 vao;
    u32 vbo;
    u32 ebo;

};

void mesh_init(Mesh& mesh, std::vector<Vertex> v, std::vector<u32> i,
               std::vector<Texture> t);

void mesh_draw(const Mesh& mesh);

struct Model
{
    std::vector<Mesh> meshes;
    std::string directory;

    std::map<std::string, Texture> loaded_textures;

    glm::vec3 unique_color;
};

void model_init(Model& model, const std::string& path);
void model_draw(const Model& model, Shader& shader);
void model_draw_color_pick(const Model& model);

void load_default_textures();


