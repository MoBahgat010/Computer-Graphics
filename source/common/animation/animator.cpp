#include "animator.hpp"

#include <cmath>
#include <iostream>

namespace {
    std::string StripAssimpFbxSuffix(const std::string& name) {
        const size_t pos = name.find("_$AssimpFbx$");
        if (pos == std::string::npos) {
            return name;
        }
        return name.substr(0, pos);
    }
}

namespace our {

Animator::Animator(Animation* currentAnimation, const glm::mat4& globalInverse)
        : m_CurrentAnimation(currentAnimation),
          m_GlobalInverseTransform(globalInverse) {
    m_FinalBoneMatrices.reserve(MAX_BONES);
    for (int i = 0; i < MAX_BONES; ++i) {
        m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }
    std::cout << "[ANIM] Animator created, reserved " << MAX_BONES << " bone matrices" << std::endl;
}

void Animator::UpdateAnimation(float deltaTime) {
    m_DeltaTime = deltaTime;
    if (m_CurrentAnimation == nullptr || !m_CurrentAnimation->IsValid()) {
        return;
    }

    const float ticksPerSecond = m_CurrentAnimation->GetTicksPerSecond();
    const float duration = m_CurrentAnimation->GetDuration();
    if (duration <= 0.0f) {
        return;
    }

    m_CurrentTime += ticksPerSecond * deltaTime;
    m_CurrentTime = std::fmod(m_CurrentTime, duration);

    if (!m_LoggedFirstFrame) {
        std::cout << "[ANIM] Animator first tick: currentTime=" << m_CurrentTime
                  << " dt=" << deltaTime << " tps=" << ticksPerSecond
                  << " duration=" << duration << std::endl;
        m_LoggedFirstFrame = true;
    }

    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), false);
}

void Animator::PlayAnimation(Animation* animation) {
    m_CurrentAnimation = animation;
    m_CurrentTime = 0.0f;
    m_LoggedFirstFrame = false;
    if (animation != nullptr) {
        std::cout << "[ANIM] Playing animation: \"" << animation->GetName() << "\"" << std::endl;
    }
}

const std::vector<glm::mat4>& Animator::GetFinalBoneMatrices() const {
    return m_FinalBoneMatrices;
}

bool Animator::HasAnimation() const {
    return m_CurrentAnimation != nullptr && m_CurrentAnimation->IsValid();
}

void Animator::CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform, bool hasBoneParent) {
    if (node == nullptr || m_CurrentAnimation == nullptr) {
        return;
    }

    const std::string nodeName = node->name;
    const std::string strippedNodeName = StripAssimpFbxSuffix(nodeName);
    glm::mat4 nodeTransform = node->transformation;

    Bone* bone = m_CurrentAnimation->FindBone(nodeName);
    if (bone == nullptr && strippedNodeName != nodeName) {
        bone = m_CurrentAnimation->FindBone(strippedNodeName);
    }
    if (bone != nullptr) {
        bone->Update(m_CurrentTime);
        nodeTransform = bone->GetLocalTransform();

        if (m_IgnoreRootTranslation && !hasBoneParent) {
            nodeTransform[3][0] = 0;
            nodeTransform[3][1] = 0;
            nodeTransform[3][2] = 0;
        }
    }

    const glm::mat4 globalTransformation = parentTransform * nodeTransform;

    const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    auto it = boneInfoMap.find(nodeName);
    if (it == boneInfoMap.end() && strippedNodeName != nodeName) {
        it = boneInfoMap.find(strippedNodeName);
    }
    if (it != boneInfoMap.end()) {
        const int index = it->second.id;
        if (index >= 0 && index < MAX_BONES) {
            m_FinalBoneMatrices[index] = m_GlobalInverseTransform * globalTransformation * it->second.offset;
        }
    }

    for (int i = 0; i < node->childrenCount; ++i) {
        CalculateBoneTransform(&node->children[i], globalTransformation, hasBoneParent || (bone != nullptr));
    }
}

}
