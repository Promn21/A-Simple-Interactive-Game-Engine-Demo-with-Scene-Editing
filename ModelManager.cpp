#include "ModelManager.h"
#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"



namespace SS
{
    Model::Model() = default;

    Model::~Model() {
        for (auto& mesh : meshes) {
            glDeleteVertexArrays(1, &mesh.VAO);
            glDeleteBuffers(1, &mesh.VBO);
            glDeleteBuffers(1, &mesh.EBO);
        }
        for (auto& tex : textures) {
            glDeleteTextures(1, &tex.id);
        }
    }

    bool Model::LoadFromFile(const std::string& filename) {
        tinygltf::TinyGLTF loader;
        std::string err, warn;
        if (!loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename)) {
            std::cerr << "Failed to load glb: " << err << "\n";
            return false;
        }
        if (!warn.empty()) std::cout << "Warn: " << warn << "\n";

        // load textures
        textures.resize(gltfModel.textures.size());
        for (size_t i = 0; i < gltfModel.images.size(); ++i) {
            textures[i].id = LoadTextureImage(gltfModel.images[i]);
        }

        materials.resize(gltfModel.materials.size());
        for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
            const auto& mat = gltfModel.materials[i];
            if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0)
                materials[i].baseColorTexture = mat.pbrMetallicRoughness.baseColorTexture.index;
        }

        // load meshes
        meshes.clear();
        for (const auto& gltfMesh : gltfModel.meshes) {
            for (const auto& prim : gltfMesh.primitives) {
                vertices.clear();
                indices.clear();
                // load attributes
                const auto& posAccessor = gltfModel.accessors[prim.attributes.at("POSITION")];
                const auto& posView = gltfModel.bufferViews[posAccessor.bufferView];
                const auto& posBuffer = gltfModel.buffers[posView.buffer];
                const float* pos = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);
                size_t vc = posAccessor.count;
                bool hasNormal = prim.attributes.count("NORMAL");
                const float* normal = nullptr;
                if (hasNormal) {
                    const auto& nA = gltfModel.accessors.at(prim.attributes.at("NORMAL"));
                    const auto& nv = gltfModel.bufferViews[nA.bufferView];
                    const auto& nb = gltfModel.buffers[nv.buffer];
                    normal = reinterpret_cast<const float*>(&nb.data[nv.byteOffset + nA.byteOffset]);
                }
                bool hasTex = prim.attributes.count("TEXCOORD_0");
                const float* tex = nullptr;
                if (hasTex) {
                    const auto& tA = gltfModel.accessors.at(prim.attributes.at("TEXCOORD_0"));
                    const auto& tv = gltfModel.bufferViews[tA.bufferView];
                    const auto& tb = gltfModel.buffers[tv.buffer];
                    tex = reinterpret_cast<const float*>(&tb.data[tv.byteOffset + tA.byteOffset]);
                }
                for (size_t i = 0; i < vc; ++i) {
                    glm::vec3 p(pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2]);
                    glm::vec3 n(0.f);
                    if (hasNormal) n = glm::vec3(normal[i * 3], normal[i * 3 + 1], normal[i * 3 + 2]);
                    glm::vec2 uv(0.f);
                    if (hasTex) uv = glm::vec2(tex[i * 2], tex[i * 2 + 1]);
                    vertices.push_back({ p,n,uv });
                }
                // load indices
                const auto& idxA = gltfModel.accessors[prim.indices];
                const auto& iv = gltfModel.bufferViews[idxA.bufferView];
                const auto& ib = gltfModel.buffers[iv.buffer];
                size_t count = idxA.count;
                if (idxA.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const unsigned short* buf = reinterpret_cast<const unsigned short*>(&ib.data[iv.byteOffset + idxA.byteOffset]);
                    for (size_t i = 0; i < count; ++i) indices.push_back(buf[i]);
                }
                else if (idxA.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const unsigned int* buf = reinterpret_cast<const unsigned int*>(&ib.data[iv.byteOffset + idxA.byteOffset]);
                    for (size_t i = 0; i < count; ++i) indices.push_back(buf[i]);
                }
                MeshGL meshGL{};
                meshGL.materialIndex = prim.material;
                SetupMesh(vertices, indices, meshGL);
                meshes.push_back(meshGL);
            }
        }
        return true;
    }

    GLuint Model::LoadTextureImage(const tinygltf::Image& image) {
        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        GLenum format = (image.component == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height,
            0, format, GL_UNSIGNED_BYTE, image.image.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return texId;
    }

    void Model::SetupMesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds, MeshGL& mesh) {
        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glGenBuffers(1, &mesh.EBO);

        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));

        glBindVertexArray(0);
        mesh.indexCount = static_cast<GLsizei>(inds.size());
    }

    void Model::Draw(GLuint shaderProgram) const {
        for (const auto& mesh : meshes) {
            GLint loc = glGetUniformLocation(shaderProgram, "hasBaseColor");
            bool hasBase = mesh.materialIndex >= 0 && materials[mesh.materialIndex].baseColorTexture >= 0;
            glUniform1i(loc, hasBase);
            if (hasBase) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures[materials[mesh.materialIndex].baseColorTexture].id);
                glUniform1i(glGetUniformLocation(shaderProgram, "baseColorTexture"), 0);
            }
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        }
    }
}