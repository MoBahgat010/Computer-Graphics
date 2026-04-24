#pragma once

#include "mesh.hpp"
#include <string>

namespace our {
    struct AnimatedMesh; // forward declaration
}

namespace our::mesh_utils {
    // Load a mesh based on file extension.
    // .obj files use TinyOBJ loader, while other formats (e.g., .fbx) use Assimp.
    Mesh* loadMesh(const std::string& filename);
    // Load a mesh with bone data for animation (keeps Assimp scene alive)
    AnimatedMesh* loadAnimatedMesh(const std::string& filename);
    // Load an ".obj" file into the mesh
    Mesh* loadOBJ(const std::string& filename);
    // Create a sphere (the vertex order in the triangles are CCW from the outside)
    // Segments define the number of divisions on the both the latitude and the longitude
    Mesh* sphere(const glm::ivec2& segments);
}
