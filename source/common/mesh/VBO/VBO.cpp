#include "VBO.h"

VBO::VBO()
{
    glGenBuffers(1, &ID);
}

VBO::VBO(std::vector<our::Vertex>& vertices)
{
    glGenBuffers(1, &ID);
    Bind();
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(our::Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VBO::SetVBO(const std::vector<our::Vertex>& vertices)
{
    Bind();
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(our::Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VBO::Bind()
{
    glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VBO::UnBind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::Delete()
{
    glDeleteBuffers(1, &ID);
}

VBO::~VBO()
{
    Delete();
}