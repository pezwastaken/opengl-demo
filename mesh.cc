#include "mesh.hh"
#include "shader.hh"
#include "utils.hh"
#include <GLES2/gl2.h>
#include <assimp/material.h>
#include <cstddef>
#include <glad/glad.h>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


static Texture default_pink_diffuse_map;
static Texture default_specular_map;

void load_default_textures()
{
    load_texture_from_file("textures/default_pink_diffuse_map.png",
                           default_pink_diffuse_map.id, GL_TEXTURE_2D);
    default_pink_diffuse_map.path = "textures/default_pink_diffuse_map.png";
    default_pink_diffuse_map.kind = Texture::TextureKind::TEXTURE_DIFFUSE;


    load_texture_from_file("textures/default_specular_map.png",
                           default_specular_map.id, GL_TEXTURE_2D);
    default_specular_map.path = "textures/default_specular_map.png";
    default_specular_map.kind = Texture::TextureKind::TEXTURE_SPECULAR;

    std::clog << "loaded default textures\n";
}

const Texture& get_default_diffuse_texture()
{
    return default_pink_diffuse_map;
}

const Texture& get_default_specular_texture()
{
    return default_specular_map;
}

void mesh_setup(Mesh& mesh);

void mesh_init(Mesh& mesh, std::vector<Vertex> v, std::vector<u32> i,
               Material m)
{
    mesh.vertices = v;
    mesh.indices = i;
    mesh.material = m;

    mesh_setup(mesh);
}

void mesh_draw(const Mesh& mesh, Shader& shader)
{
    u32 i_diffuse = 0;
    u32 i_specular = 0;

    assert(mesh.material.textures.size() > 0);

    //TODO: I don't like this. I should probably just have two functions
    if(shader.has_uniform("material.texture_diffuse0"))
    {
        for(u32 i = 0; i < mesh.material.textures.size(); ++i)
        {
            const auto& texture = mesh.material.textures[i];

            // std::clog << "using texture path: " << texture.path << std::endl;

            assert(GL_TEXTURE0 + i <= GL_TEXTURE31);
            glActiveTexture(GL_TEXTURE0 + i);

            std::string uniform_name = "texture_";

            switch (texture.kind)
            {
            case Texture::TEXTURE_DIFFUSE:
                uniform_name += "diffuse" + std::to_string(i_diffuse++);
                break;
            case Texture::TEXTURE_SPECULAR:
                uniform_name += "specular" + std::to_string(i_specular++);
                break;
            default:
                std::cerr << "unexpected texture type" << std::endl;
                UNREACHABLE;
            }

            assert(shader.has_uniform(std::string("material." + uniform_name).c_str()));
            shader.set_int(("material." + uniform_name).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, static_cast<u32>(mesh.indices.size()),
                   GL_UNSIGNED_INT, 0);

    glActiveTexture(GL_TEXTURE0);
}

void mesh_draw_color_pick(const Mesh& mesh)
{
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, static_cast<u32>(mesh.indices.size()),
                   GL_UNSIGNED_INT, 0);
}


void mesh_setup(Mesh& mesh)
{
    glGenVertexArrays(1, &mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex),
                 mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(u32),
                 mesh.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, tex_coords));
}


//assimp to mesh
//
std::vector<Texture> load_material_textures(Model& model,
                                            const aiScene& scene,
                                            aiMaterial* material,
                                            aiTextureType ai_texture_type,
                                            Texture::TextureKind texture_kind);

Mesh mesh_from_assimp_mesh(Model& model, aiMesh* mesh, const aiScene& scene)
{
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::vector<Texture> textures;

    for(u32 i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;

        glm::vec3 position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;
        vertex.position = position;

        glm::vec3 normal;
        normal.x = mesh->mNormals[i].x;
        normal.y = mesh->mNormals[i].y;
        normal.z = mesh->mNormals[i].z;
        vertex.normal = normal;

        glm::vec2 texture_coords (0.0F);
        //assimp allows 8 texture coordinates, we are only interested in the first one
        if (mesh->mTextureCoords[0])
        {
            texture_coords.x = mesh->mTextureCoords[0][i].x;
            texture_coords.y = mesh->mTextureCoords[0][i].y;
        }
        vertex.tex_coords = texture_coords;

        vertices.push_back(vertex);
    }

    //process indices
    //NumFaces represents the number of faces/primitives composing the mesh
    for(u32 i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for(u32 j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    //material
    //The material index is a reference to the material contained in the scene object
    //
    u32 material_index = mesh->mMaterialIndex;
    std::clog << "mesh material index: " << material_index << std::endl;

    assert(scene.mNumMaterials > material_index);
    assert(scene.mMaterials[material_index]);

    aiMaterial* material = scene.mMaterials[material_index];

    std::clog << "\n mesh material diffuse count: " <<
        material->GetTextureCount(aiTextureType_DIFFUSE);
    std::clog << "\n mesh material metallic count: " <<
        material->GetTextureCount(aiTextureType_METALNESS);
    std::clog << "\n mesh material normal count: " <<
        material->GetTextureCount(aiTextureType_NORMALS);
    std::clog << "\n mesh material ambient occlusion count: " <<
        material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION);
    std::clog << "\n mesh material roughness count: " <<
        material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) << "\n\n";

    std::vector<Texture> diffuse_maps =
        load_material_textures(model, scene, material,
                               aiTextureType_DIFFUSE, Texture::TEXTURE_DIFFUSE);

    if(diffuse_maps.size() == 0)
    {
        std::clog << "loading default diffuse map" << std::endl;
        diffuse_maps.push_back(default_pink_diffuse_map);
    }


    std::vector<Texture> specular_maps =
        load_material_textures(model, scene, material,
                               aiTextureType_SPECULAR, Texture::TEXTURE_SPECULAR);

    if(specular_maps.size() == 0)
    {
        std::clog << "loading default specular map" << std::endl;
        specular_maps.push_back(default_specular_map);
    }

    std::clog << "mesh " << mesh << " has " << diffuse_maps.size() << " diffuse maps"
        " and " << specular_maps.size() << " specular maps " << std::endl;

    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());
    textures.insert(textures.end(), specular_maps.begin(), specular_maps.end());

    assert(textures.size() >= 2);

    Mesh converted_mesh;
    Material mesh_material {.textures = textures};
    mesh_init(converted_mesh, vertices, indices, mesh_material);

    return converted_mesh;
}

std::vector<Texture> load_material_textures(Model& model,
                                            const aiScene& scene,
                                            aiMaterial* material,
                                            aiTextureType ai_texture_type,
                                            Texture::TextureKind texture_kind)
{
    std::clog << "loading material texture" << std::endl;

    std::vector<Texture> textures;
    u32 n_texture = material->GetTextureCount(ai_texture_type);
    textures.reserve(n_texture);

    std::clog << "material has " << n_texture << " " << ai_texture_type << " textures " << std::endl;

    for(u32 i = 0; i < n_texture; ++i)
    {
        aiString texture_path;
        material->GetTexture(ai_texture_type, i, &texture_path);

        const char* texture_path_c_str = texture_path.C_Str();
        std::clog << "texture path: " << texture_path_c_str << std::endl;

        Texture texture;

        //check if already loaded a texture with the same path (same texture)
        //
        const auto not_found = model.loaded_textures.end();
        const auto found_texture = model.loaded_textures.find(std::string(texture_path_c_str));
        if (model.loaded_textures.find(std::string(texture_path_c_str)) != not_found)
        {
            texture = found_texture->second;
            std::clog << "texture already loaded: " << texture.id << ", " << texture.path << std::endl;
        }
        else
        {
            bool texture_loaded = false;
            if(texture_path_c_str[0] == '*')
            {
                texture_loaded =
                    load_texture_from_scene(scene, texture_path.C_Str(), texture.id, GL_TEXTURE_2D);
            }
            else
            {
                std::string full_texture_path = model.directory + "/" + texture_path_c_str;
                std::clog << "loading texture from full path: " << full_texture_path << "\n";
                texture_loaded =
                    load_texture_from_file(full_texture_path.c_str(), texture.id, GL_TEXTURE_2D);
            }

            if(!texture_loaded)
            {
                std::cerr << "loading non existing texture: " << texture_path.C_Str() << std::endl;
                TODO;
            }

            texture.kind = texture_kind;
            texture.path = std::string(texture_path.C_Str());

            model.loaded_textures.insert(std::make_pair(texture.path, texture));
        }

        textures.push_back(texture);
    }

    return textures;
}



//model stuff

void model_process_node(Model& model, aiNode* node, const aiScene& scene)
{

    std::clog << "processing model node: " << node << ". It has "
        << node->mNumMeshes << " meshes" << std::endl;

    for(u32 i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene.mMeshes[node->mMeshes[i]];
        model.meshes.push_back(mesh_from_assimp_mesh(model, mesh, scene));
    }

    std::clog << "pushed meshes. Going to the next node\n\n";

    for(u32 i = 0; i < node->mNumChildren; ++i)
    {
        model_process_node(model, node->mChildren[i], scene);
    }

}

void model_draw(const Model& model, Shader& shader)
{
    for (const auto& mesh : model.meshes)
    {
        mesh_draw(mesh, shader);
    }
}

void model_draw_color_pick(const Model& model)
{
    for (const auto& mesh : model.meshes)
    {
        mesh_draw_color_pick(mesh);
    }
}


void model_init(Model& model, const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(path,aiProcess_Triangulate);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::clog << "assimp error: " << importer.GetErrorString() << std::endl;
        return;
    }

    model.directory = path.substr(0, path.find_last_of("/"));

    std::clog << "aiScene has " << scene->mNumMeshes << " meshes." << std::endl;
    std::clog << "aiScene has " << scene->mNumTextures << " embedded textures" << std::endl;
    std::clog << "aiScene has " << scene->mNumAnimations << " animations" << std::endl;

    model_process_node(model, scene->mRootNode, *scene);

    std::clog << "loaded model has " << model.meshes.size() << " meshes" << std::endl;
    std::clog << "loaded model has " << model.loaded_textures.size() << " textures"
        " (excluding default ones)" << std::endl;

}


void model_init_primitive_cube(Model& model, const std::vector<Texture>& textures)
{
    std::vector<Vertex> vertices
    {
        {{-0.5f, -0.5f, -0.5f},  {0.0f,  0.0f, -1.0f},  {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f},  {0.0f,  0.0f, -1.0f},  {1.0f, 0.0f}},
        {{0.5f,  0.5f, -0.5f},  {0.0f,  0.0f, -1.0f},  {1.0f, 1.0f}},
        {{0.5f,  0.5f, -0.5f},  {0.0f,  0.0f, -1.0f},  {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},  {0.0f,  0.0f, -1.0f},  {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f},  {0.0f,  0.0f, -1.0f},  {0.0f, 0.0f}},

        {{-0.5f, -0.5f,  0.5f},  {0.0f,  0.0f, 1.0f},  { 0.0f, 0.0f}},
        {{0.5f, -0.5f,  0.5f},  {0.0f,  0.0f, 1.0f},  { 1.0f, 0.0f}},
        {{0.5f,  0.5f,  0.5f},  {0.0f,  0.0f, 1.0f},  { 1.0f, 1.0f}},
        {{0.5f,  0.5f,  0.5f},  {0.0f,  0.0f, 1.0f},  { 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f},  {0.0f,  0.0f, 1.0f},  { 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f},  {0.0f,  0.0f, 1.0f},  { 0.0f, 0.0f}},

        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},  {1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},  {0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},

        {{0.5f,  0.5f,  0.5f},  {1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},
        {{0.5f,  0.5f, -0.5f},  {1.0f,  0.0f,  0.0f},  {1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f},  {1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f},  {1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},
        {{0.5f, -0.5f,  0.5f},  {1.0f,  0.0f,  0.0f},  {0.0f, 0.0f}},
        {{0.5f,  0.5f,  0.5f},  {1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},

        {{-0.5f, -0.5f, -0.5f},  {0.0f, -1.0f,  0.0f},  {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f},  {0.0f, -1.0f,  0.0f},  {1.0f, 1.0f}},
        {{0.5f, -0.5f,  0.5f},  {0.0f, -1.0f,  0.0f},  {1.0f, 0.0f}},
        {{0.5f, -0.5f,  0.5f},  {0.0f, -1.0f,  0.0f},  {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f},  {0.0f, -1.0f,  0.0f},  {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f},  {0.0f, -1.0f,  0.0f},  {0.0f, 1.0f}},

        {{-0.5f,  0.5f, -0.5f},  {0.0f,  1.0f,  0.0f},  {0.0f, 1.0f}},
        {{0.5f,  0.5f, -0.5f},  {0.0f,  1.0f,  0.0f},  {1.0f, 1.0f}},
        {{0.5f,  0.5f,  0.5f},  {0.0f,  1.0f,  0.0f},  {1.0f, 0.0f}},
        {{0.5f,  0.5f,  0.5f},  {0.0f,  1.0f,  0.0f},  {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f},  {0.0f,  1.0f,  0.0f},  {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f},  {0.0f,  1.0f,  0.0f},  {0.0f, 1.0f}}
    };

    const u32 n = 36;
    std::vector<u32> indices;
    indices.reserve(n);
    for(u32 i = 0; i < n; ++i)
    {
        indices.push_back(i);
    }

    Mesh cube_mesh {.vertices = vertices, .indices = indices};
    mesh_setup(cube_mesh);

    Material material {.textures = textures};
    cube_mesh.material = material;

    model.meshes = {cube_mesh};
    model.directory = "";
    model.loaded_textures = {};
}

