#include "animation.hpp"

#include <assimp/scene.h>

#include <algorithm>
#include <iostream>

namespace {
    glm::mat4 ConvertMatrixToGLM(const aiMatrix4x4& mat) {
        glm::mat4 glm_mat;
        glm_mat[0][0] = mat.a1; glm_mat[1][0] = mat.a2; glm_mat[2][0] = mat.a3; glm_mat[3][0] = mat.a4;
        glm_mat[0][1] = mat.b1; glm_mat[1][1] = mat.b2; glm_mat[2][1] = mat.b3; glm_mat[3][1] = mat.b4;
        glm_mat[0][2] = mat.c1; glm_mat[1][2] = mat.c2; glm_mat[2][2] = mat.c3; glm_mat[3][2] = mat.c4;
        glm_mat[0][3] = mat.d1; glm_mat[1][3] = mat.d2; glm_mat[2][3] = mat.d3; glm_mat[3][3] = mat.d4;
        return glm_mat;
    }

    std::string StripAssimpFbxSuffix(const std::string& name) {
        const size_t pos = name.find("_$AssimpFbx$");
        if (pos == std::string::npos) {
            return name;
        }
        return name.substr(0, pos);
    }
}

namespace our {

Animation::Animation(AnimatedMesh* animatedMesh, unsigned int animationIndex) {
    if (animatedMesh == nullptr) {
        std::cerr << "[ANIM] Animation: animatedMesh is nullptr" << std::endl;
        return;
    }

    const aiScene* modelScene = animatedMesh->GetScene();
    if (modelScene == nullptr) {
        std::cerr << "[ANIM] Animation: scene is nullptr" << std::endl;
        return;
    }

    std::cout << "[ANIM] Scene has " << modelScene->mNumAnimations << " animation(s)" << std::endl;

    if (modelScene->mNumAnimations == 0) {
        std::cerr << "[ANIM] Animation: no animations in scene" << std::endl;
        return;
    }

    if (animationIndex >= modelScene->mNumAnimations) {
        std::cerr << "[ANIM] Animation: index " << animationIndex
                  << " out of range (max=" << modelScene->mNumAnimations - 1 << ")" << std::endl;
        return;
    }

    const aiAnimation* animation = modelScene->mAnimations[animationIndex];
    if (animation == nullptr) {
        std::cerr << "[ANIM] Animation: aiAnimation is nullptr at index " << animationIndex << std::endl;
        return;
    }

    if (modelScene->mRootNode == nullptr) {
        std::cerr << "[ANIM] Animation: scene root node is nullptr" << std::endl;
        return;
    }

    m_Name = animation->mName.C_Str();
    m_Duration = static_cast<float>(animation->mDuration);
    m_TicksPerSecond = animation->mTicksPerSecond != 0.0 ? static_cast<float>(animation->mTicksPerSecond) : 25.0f;

    std::cout << "[ANIM] Loading animation \"" << m_Name << "\" (index=" << animationIndex
              << ") duration=" << m_Duration << " tps=" << m_TicksPerSecond
              << " channels=" << animation->mNumChannels << std::endl;

    ReadHierarchyData(m_RootNode, modelScene->mRootNode);
    ReadMissingBones(animation, *animatedMesh);

    m_IsValid = true;
    std::cout << "[ANIM] Animation \"" << m_Name << "\" loaded successfully with "
              << m_Bones.size() << " bones" << std::endl;
}

Bone* Animation::FindBone(const std::string& name) {
    auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
        [&name](const Bone& bone) {
            return bone.GetBoneName() == name;
        }
    );
    if (iter == m_Bones.end()) {
        return nullptr;
    }
    return &(*iter);
}

float Animation::GetTicksPerSecond() const {
    return m_TicksPerSecond;
}

float Animation::GetDuration() const {
    return m_Duration;
}

const AssimpNodeData& Animation::GetRootNode() const {
    return m_RootNode;
}

const std::map<std::string, BoneInfo>& Animation::GetBoneIDMap() const {
    return m_BoneInfoMap;
}

bool Animation::IsValid() const {
    return m_IsValid;
}

std::string Animation::GetName() const {
    return m_Name;
}

void Animation::ReadMissingBones(const aiAnimation* animation, AnimatedMesh& animMesh) {
    if (animation == nullptr) {
        return;
    }

    const auto& boneInfoMap = animMesh.GetBoneInfoMap();

    for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
        const aiNodeAnim* channel = animation->mChannels[i];
        if (channel == nullptr) {
            continue;
        }

        const std::string channelName = channel->mNodeName.data;
        std::string boneName;

        auto it = boneInfoMap.find(channelName);
        if (it != boneInfoMap.end()) {
            boneName = channelName;
        } else {
            const std::string strippedName = StripAssimpFbxSuffix(channelName);
            const bool isFbxHelperChannel = strippedName != channelName;
            if (isFbxHelperChannel) {
                continue;
            }

            it = boneInfoMap.find(strippedName);
            if (it == boneInfoMap.end()) {
                continue;
            }
            boneName = strippedName;
        }

        const auto exists = std::find_if(
            m_Bones.begin(),
            m_Bones.end(),
            [&boneName](const Bone& bone) {
                return bone.GetBoneName() == boneName;
            }
        );
        if (exists != m_Bones.end()) {
            continue;
        }

        m_Bones.emplace_back(boneName, it->second.id, channel);
    }

    m_BoneInfoMap = boneInfoMap;

    std::cout << "[ANIM] ReadMissingBones: " << m_Bones.size() << " bones matched from "
              << animation->mNumChannels << " channels" << std::endl;
}

void Animation::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src) {
    if (src == nullptr) {
        return;
    }

    dest.name = src->mName.data;
    dest.transformation = ConvertMatrixToGLM(src->mTransformation);
    dest.childrenCount = static_cast<int>(src->mNumChildren);

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData newData;
        ReadHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

}
