#pragma once

#include <vector>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "../vertex.hpp"

class VBO {

public:
    GLuint ID;

    VBO();
    VBO(std::vector<our::Vertex>& vertices);

    void Bind();
    void SetVBO(const std::vector<our::Vertex>& vertices);
    void UnBind();
    void Delete();

    ~VBO();
};