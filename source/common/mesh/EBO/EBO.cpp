#include "EBO.h"

EBO::EBO()
{
    glGenBuffers(1, &ID);
}

EBO::EBO(std::vector<GLuint>& vertices)
{
    glGenBuffers(1, &ID);
    Bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertices.size() * sizeof(GLuint), vertices.data(), GL_STATIC_DRAW);
}

void EBO::SetEBO(const std::vector<GLuint>& indices)
{
    Bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
}

void EBO::Bind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void EBO::UnBind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::Delete()
{
    glDeleteBuffers(1, &ID);
}

EBO::~EBO()
{
    Delete();
}