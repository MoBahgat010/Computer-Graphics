#include "bone.hpp"

#include <cassert>
#include <iostream>

namespace {
    glm::vec3 GetGLMVec(const aiVector3D& vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    glm::quat GetGLMQuat(const aiQuaternion& pOrientation) {
        return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
    }
}

namespace our {

    Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel)
        : m_Name(name), m_ID(id) {
        if (channel == nullptr) {
            std::cout << "[ANIM] Bone \"" << name << "\" created with NULL channel" << std::endl;
            return;
        }

        m_NumPositions = static_cast<int>(channel->mNumPositionKeys);
        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex) {
            KeyPosition data;
            data.position = GetGLMVec(channel->mPositionKeys[positionIndex].mValue);
            data.timeStamp = static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
            m_Positions.push_back(data);
        }

        m_NumRotations = static_cast<int>(channel->mNumRotationKeys);
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex) {
            KeyRotation data;
            data.orientation = GetGLMQuat(channel->mRotationKeys[rotationIndex].mValue);
            data.timeStamp = static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
            m_Rotations.push_back(data);
        }

        m_NumScales = static_cast<int>(channel->mNumScalingKeys);
        for (int keyIndex = 0; keyIndex < m_NumScales; ++keyIndex) {
            KeyScale data;
            data.scale = GetGLMVec(channel->mScalingKeys[keyIndex].mValue);
            data.timeStamp = static_cast<float>(channel->mScalingKeys[keyIndex].mTime);
            m_Scales.push_back(data);
        }

        std::cout << "[ANIM] Bone \"" << name << "\" (id=" << id
                << ") created with " << m_NumPositions << "P/"
                << m_NumRotations << "R/" << m_NumScales << "S keyframes" << std::endl;
    }

    void Bone::Update(float animationTime) {
        const glm::mat4 translation = InterpolatePosition(animationTime);
        const glm::mat4 rotation = InterpolateRotation(animationTime);
        const glm::mat4 scale = InterpolateScaling(animationTime);
        m_LocalTransform = translation * rotation * scale;
    }

    glm::mat4 Bone::GetLocalTransform() const {
        return m_LocalTransform;
    }

    std::string Bone::GetBoneName() const {
        return m_Name;
    }

    int Bone::GetBoneID() const {
        return m_ID;
    }

    float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const {
        const float midWayLength = animationTime - lastTimeStamp;
        const float framesDiff = nextTimeStamp - lastTimeStamp;
        if (framesDiff <= 0.0f) {
            return 0.0f;
        }
        return midWayLength / framesDiff;
    }

    int Bone::GetPositionIndex(float animationTime) const {
        for (int index = 0; index < m_NumPositions - 1; ++index) {
            if (animationTime < m_Positions[index + 1].timeStamp) {
                return index;
            }
        }
        return std::max(0, m_NumPositions - 2);
    }

    int Bone::GetRotationIndex(float animationTime) const {
        for (int index = 0; index < m_NumRotations - 1; ++index) {
            if (animationTime < m_Rotations[index + 1].timeStamp) {
                return index;
            }
        }
        return std::max(0, m_NumRotations - 2);
    }

    int Bone::GetScaleIndex(float animationTime) const {
        for (int index = 0; index < m_NumScales - 1; ++index) {
            if (animationTime < m_Scales[index + 1].timeStamp) {
                return index;
            }
        }
        return std::max(0, m_NumScales - 2);
    }

    glm::mat4 Bone::InterpolatePosition(float animationTime) const {
        if (m_NumPositions <= 0) {
            return glm::mat4(1.0f);
        }
        if (m_NumPositions == 1) {
            return glm::translate(glm::mat4(1.0f), m_Positions[0].position);
        }

        const int p0Index = GetPositionIndex(animationTime);
        const int p1Index = p0Index + 1;
        if (p1Index >= m_NumPositions) {
            return glm::translate(glm::mat4(1.0f), m_Positions[p0Index].position);
        }
        const float scaleFactor = GetScaleFactor(
            m_Positions[p0Index].timeStamp,
            m_Positions[p1Index].timeStamp,
            animationTime
        );
        const glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 Bone::InterpolateRotation(float animationTime) const {
        if (m_NumRotations <= 0) {
            return glm::mat4(1.0f);
        }
        if (m_NumRotations == 1) {
            return glm::toMat4(glm::normalize(m_Rotations[0].orientation));
        }

        const int p0Index = GetRotationIndex(animationTime);
        const int p1Index = p0Index + 1;
        if (p1Index >= m_NumRotations) {
            return glm::toMat4(glm::normalize(m_Rotations[p0Index].orientation));
        }
        const float scaleFactor = GetScaleFactor(
            m_Rotations[p0Index].timeStamp,
            m_Rotations[p1Index].timeStamp,
            animationTime
        );
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
        finalRotation = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    glm::mat4 Bone::InterpolateScaling(float animationTime) const {
        if (m_NumScales <= 0) {
            return glm::mat4(1.0f);
        }
        if (m_NumScales == 1) {
            return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);
        }

        const int p0Index = GetScaleIndex(animationTime);
        const int p1Index = p0Index + 1;
        if (p1Index >= m_NumScales) {
            return glm::scale(glm::mat4(1.0f), m_Scales[p0Index].scale);
        }
        const float scaleFactor = GetScaleFactor(
            m_Scales[p0Index].timeStamp,
            m_Scales[p1Index].timeStamp,
            animationTime
        );
        const glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

}
