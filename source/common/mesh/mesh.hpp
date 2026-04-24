#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <vector>
#include "vertex.hpp"
#include <vector>

#include "VAO/VAO.h"
#include "VBO/VBO.h"
#include "EBO/EBO.h"

namespace our {

    #define ATTRIB_LOC_POSITION 0
    #define ATTRIB_LOC_COLOR    1
    #define ATTRIB_LOC_TEXCOORD 2
    #define ATTRIB_LOC_NORMAL   3
    #define ATTRIB_LOC_BONE_IDS     4
    #define ATTRIB_LOC_BONE_WEIGHTS 5

    class Mesh {
    public:
        struct DrawBatch {
            GLuint firstIndex = 0;
            GLsizei indexCount = 0;
            GLuint texture = 0;
            bool hasTexture = false;
        };

    private:
        // Here, we store the object names of the 3 main components of a mesh:
        // A vertex array object, A vertex buffer and an element buffer
        VAO VAO1;
        VBO VBO1;
        EBO EBO1;
        // We need to remember the number of elements that will be draw by glDrawElements 
        GLsizei elementCount;
        // Keep a CPU copy of vertices for gameplay/physics queries (e.g., convex hull generation).
        std::vector<our::Vertex> cpuVertices;
        // Keep a CPU copy of indices for gameplay/physics queries (e.g., triangle mesh collision).
        std::vector<unsigned int> cpuIndices;
        std::vector<DrawBatch> drawBatches;
        std::vector<GLuint> ownedTextures;

    public:

        // The constructor takes two vectors:
        // - vertices which contain the vertex data.
        // - elements which contain the indices of the vertices out of which each rectangle will be constructed.
        // The mesh class does not keep a these data on the RAM. Instead, it should create
        // a vertex buffer to store the vertex data on the VRAM,
        // an element buffer to store the element data on the VRAM,
        // a vertex array object to define how to read the vertex & element buffer during rendering 
        Mesh(
            const std::vector<our::Vertex>& vertices,
            const std::vector<unsigned int>& elements,
            const std::vector<DrawBatch>& drawBatches = {},
            const std::vector<GLuint>& ownedTextures = {}
        ) : drawBatches(drawBatches), ownedTextures(ownedTextures)
        {
            VAO1.Bind();

            VBO1.SetVBO(vertices);
            EBO1.SetEBO(elements);

            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_POSITION, 3, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, position));
            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_COLOR, 4, GL_UNSIGNED_BYTE, sizeof(our::Vertex), (void*)offsetof(our::Vertex, color), GL_TRUE);
            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_TEXCOORD, 2, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, tex_coord));
            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_NORMAL, 3, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, normal));

            // Bone IDs (integer attribute - must use glVertexAttribIPointer)
            VBO1.Bind();
            glVertexAttribIPointer(ATTRIB_LOC_BONE_IDS, MAX_BONE_INFLUENCE, GL_INT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, boneIDs));
            glEnableVertexAttribArray(ATTRIB_LOC_BONE_IDS);
            // Bone Weights (float attribute)
            glVertexAttribPointer(ATTRIB_LOC_BONE_WEIGHTS, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(our::Vertex), (void*)offsetof(our::Vertex, boneWeights));
            glEnableVertexAttribArray(ATTRIB_LOC_BONE_WEIGHTS);
            VBO1.UnBind();

            elementCount = static_cast<GLsizei>(elements.size());

            if(this->drawBatches.empty()) {
                this->drawBatches.push_back({0, elementCount, 0, false});
            }
            cpuVertices = vertices;
            cpuIndices = elements;
        }

        // Returns CPU-side vertex data (read-only).
        const std::vector<our::Vertex>& getVertices() const {
            return cpuVertices;
        }

        // Returns CPU-side index data (read-only).
        const std::vector<unsigned int>& getIndices() const {
            return cpuIndices;
        }

        // this function should render the mesh
        void draw() 
        {
            VAO1.Bind();

            if(drawBatches.empty()) {
                glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, 0);
            } else {
                glActiveTexture(GL_TEXTURE0);
                for(const auto& batch : drawBatches) {
                    if(batch.hasTexture) {
                        glBindTexture(GL_TEXTURE_2D, batch.texture);
                    }
                    glDrawElements(
                        GL_TRIANGLES,
                        batch.indexCount,
                        GL_UNSIGNED_INT,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(batch.firstIndex) * sizeof(GLuint))
                    );
                }
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            VAO1.UnBind();
        }

        // this function should delete the vertex & element buffers and the vertex array object
        ~Mesh(){
            if(!ownedTextures.empty()) {
                glDeleteTextures(static_cast<GLsizei>(ownedTextures.size()), ownedTextures.data());
            }
            // VAO, VBO and EBO are deleted by their own destructors.
        }

        Mesh(Mesh const &) = delete;
        Mesh &operator=(Mesh const &) = delete;
    };

}