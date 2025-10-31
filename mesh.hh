#pragma once

#include "shader.hh"
#include "utils.hh"
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>
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



//TODO: use this instead of a plain vector of textures in the Mesh struct
struct Material
{
    std::vector<Texture> textures;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    Material material;
    // std::vector<Texture> textures;

    u32 vao;
    u32 vbo;
    u32 ebo;

};

void mesh_init(Mesh& mesh, std::vector<Vertex> v, std::vector<u32> i,
               Material m);

void mesh_draw(const Mesh& mesh);

struct Model
{
    std::vector<Mesh> meshes;
    std::string directory;

    std::unordered_map<std::string, Texture> loaded_textures;
};

void model_init(Model& model, const std::string& path);
void model_draw(const Model& model, Shader& shader);
void model_draw_color_pick(const Model& model);

void model_init_primitive_cube(Model& model, const std::vector<Texture>& textures);

void load_default_textures();
const Texture& get_default_diffuse_texture();
const Texture& get_default_specular_texture();


