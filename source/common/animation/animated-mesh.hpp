#pragma once

#include "../mesh/mesh.hpp"
#include "../mesh/vertex.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace our {

    // Holds a Mesh* along with bone/skeleton metadata needed for animation.
    // The Assimp Importer is kept alive so the aiScene remains valid.
    struct AnimatedMesh {
        Mesh* mesh = nullptr;
        std::map<std::string, BoneInfo> boneInfoMap;
        int boneCounter = 0;
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);

        // Importer kept alive so aiScene* remains valid for Animation loading
        std::unique_ptr<Assimp::Importer> importer;
        const aiScene* scene = nullptr;

        const std::map<std::string, BoneInfo>& GetBoneInfoMap() const { return boneInfoMap; }
        const aiScene* GetScene() const { return scene; }

        ~AnimatedMesh() {
            delete mesh;
            std::cout << "[ANIM] AnimatedMesh destroyed" << std::endl;
        }
    };

} // namespace our
