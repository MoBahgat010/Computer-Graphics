#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../vertex.hpp"
#include "../VBO/VBO.h"

class VAO {

public:
    GLuint ID;

    VAO();

    void linkAtrribute(VBO& VBO1, GLuint location, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);
    void Bind();
    void UnBind();
    void Delete();

    ~VAO();
};