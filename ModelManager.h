#pragma once
#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "tiny_gltf.h"

namespace SS
{
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
    };

    struct MeshGL {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        GLsizei indexCount = 0;
        int materialIndex = -1;
    };

    struct TextureGL {
        GLuint id = 0;
    };

    struct Material {
        int baseColorTexture = -1;
    };

    class Model {
    public:
        Model();
        ~Model();
        bool LoadFromFile(const std::string& filename);
        void Draw(GLuint shaderProgram) const;

    private:
        std::vector<MeshGL> meshes;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<TextureGL> textures;
        std::vector<Material> materials;
        tinygltf::Model gltfModel;

        void SetupMesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds, MeshGL& mesh);
        GLuint LoadTextureImage(const tinygltf::Image& image);
    };
}
