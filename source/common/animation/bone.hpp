#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <vector>

namespace our {

    struct KeyPosition {
        glm::vec3 position;
        float timeStamp;
    };

    struct KeyRotation {
        glm::quat orientation;
        float timeStamp;
    };

    struct KeyScale {
        glm::vec3 scale;
        float timeStamp;
    };

    class Bone {
    public:
        Bone(const std::string& name, int id, const aiNodeAnim* channel);

        void Update(float animationTime);

        glm::mat4 GetLocalTransform() const;
        std::string GetBoneName() const;
        int GetBoneID() const;

    private:
        float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;
        int GetPositionIndex(float animationTime) const;
        int GetRotationIndex(float animationTime) const;
        int GetScaleIndex(float animationTime) const;

        glm::mat4 InterpolatePosition(float animationTime) const;
        glm::mat4 InterpolateRotation(float animationTime) const;
        glm::mat4 InterpolateScaling(float animationTime) const;

        std::vector<KeyPosition> m_Positions;
        std::vector<KeyRotation> m_Rotations;
        std::vector<KeyScale> m_Scales;

        int m_NumPositions = 0;
        int m_NumRotations = 0;
        int m_NumScales = 0;

        glm::mat4 m_LocalTransform = glm::mat4(1.0f);
        std::string m_Name;
        int m_ID = -1;
    };

}
