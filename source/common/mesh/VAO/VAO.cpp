#include "VAO.h"
#include <iostream>
using namespace std;

VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

void VAO::linkAtrribute(VBO& VBO1, GLuint location, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset) {
    VBO1.Bind();
    glVertexAttribPointer(location, numComponents, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(location);
    VBO1.UnBind();
}

void VAO::Bind()
{
    glBindVertexArray(ID);
}

void VAO::UnBind()
{
    glBindVertexArray(0);
}

void VAO::Delete()
{
    glDeleteVertexArrays(1, &ID);
}

VAO::~VAO()
{
    Delete();
}