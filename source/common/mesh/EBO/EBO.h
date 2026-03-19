#pragma once

#include <vector>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../vertex.hpp"

class EBO {

public:
    GLuint ID;

    EBO();
    EBO(std::vector<GLuint>& vertices);

    void SetEBO(const std::vector<GLuint>& indices);
    void Bind();
    void UnBind();
    void Delete();

    ~EBO();
};