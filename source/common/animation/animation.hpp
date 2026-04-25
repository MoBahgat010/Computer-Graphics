#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>

#include "bone.hpp"
#include "animated-mesh.hpp"

namespace our {

    struct AssimpNodeData {
        glm::mat4 transformation = glm::mat4(1.0f);
        std::string name;
        int childrenCount = 0;
        std::vector<AssimpNodeData> children;
    };

    class Animation {
    public:
        Animation(AnimatedMesh* animatedMesh, unsigned int animationIndex = 0);

        Bone* FindBone(const std::string& name);
        float GetTicksPerSecond() const;
        float GetDuration() const;
        const AssimpNodeData& GetRootNode() const;
        const std::map<std::string, BoneInfo>& GetBoneIDMap() const;
        bool IsValid() const;
        std::string GetName() const;

    private:
        void ReadMissingBones(const aiAnimation* animation, AnimatedMesh& animMesh);
        void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);

        float m_Duration = 0.0f;
        float m_TicksPerSecond = 25.0f;
        std::vector<Bone> m_Bones;
        AssimpNodeData m_RootNode;
        std::map<std::string, BoneInfo> m_BoneInfoMap;
        bool m_IsValid = false;
        std::string m_Name;
    };

}
