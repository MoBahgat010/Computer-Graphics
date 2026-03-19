#pragma once

#include <glad/gl.h>
#include "vertex.hpp"

#include "VAO/VAO.h"
#include "VBO/VBO.h"
#include "EBO/EBO.h"

namespace our {

    #define ATTRIB_LOC_POSITION 0
    #define ATTRIB_LOC_COLOR    1
    #define ATTRIB_LOC_TEXCOORD 2
    #define ATTRIB_LOC_NORMAL   3

    class Mesh {
        // Here, we store the object names of the 3 main components of a mesh:
        // A vertex array object, A vertex buffer and an element buffer
        VAO VAO1;
        VBO VBO1;
        EBO EBO1;
        // We need to remember the number of elements that will be draw by glDrawElements 
        GLsizei elementCount;
    public:

        // The constructor takes two vectors:
        // - vertices which contain the vertex data.
        // - elements which contain the indices of the vertices out of which each rectangle will be constructed.
        // The mesh class does not keep a these data on the RAM. Instead, it should create
        // a vertex buffer to store the vertex data on the VRAM,
        // an element buffer to store the element data on the VRAM,
        // a vertex array object to define how to read the vertex & element buffer during rendering 
        Mesh(const std::vector<our::Vertex>& vertices, const std::vector<unsigned int>& elements)
        {
            //TODO: (Req 2) Write this function
            // remember to store the number of elements in "elementCount" since you will need it for drawing
            // For the attribute locations, use the constants defined above: ATTRIB_LOC_POSITION, ATTRIB_LOC_COLOR, etc
            VAO1.Bind();

            VBO1.SetVBO(vertices);
            EBO1.SetEBO(elements);

            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_POSITION, 3, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, position));
            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_COLOR, 4, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, color));
            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_TEXCOORD, 2, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, tex_coord));
            VAO1.linkAtrribute(VBO1, ATTRIB_LOC_NORMAL, 3, GL_FLOAT, sizeof(our::Vertex), (void*)offsetof(our::Vertex, normal));

            elementCount = elements.size();
        }

        // this function should render the mesh
        void draw() 
        {
            VAO1.Bind();
            glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, 0);
            VAO1.UnBind();
        }

        // this function should delete the vertex & element buffers and the vertex array object
        ~Mesh(){
            //TODO: (Req 2) Write this function
            // Handled by the destructors of the VAO, VBO and EBO classes
        }

        Mesh(Mesh const &) = delete;
        Mesh &operator=(Mesh const &) = delete;
    };

}