#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "animation.hpp"
#include "../mesh/vertex.hpp"

namespace our {

    class Animator {
    public:
        explicit Animator(Animation* currentAnimation = nullptr, const glm::mat4& globalInverse = glm::mat4(1.0f));

        void UpdateAnimation(float deltaTime);
        void PlayAnimation(Animation* animation);
        const std::vector<glm::mat4>& GetFinalBoneMatrices() const;
        bool HasAnimation() const;

        void setIgnoreRootTranslation(bool value) { m_IgnoreRootTranslation = value; }

    private:
        void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform, bool hasBoneParent);

        std::vector<glm::mat4> m_FinalBoneMatrices;
        Animation* m_CurrentAnimation = nullptr;
        glm::mat4 m_GlobalInverseTransform = glm::mat4(1.0f);
        float m_CurrentTime = 0.0f;
        float m_DeltaTime = 0.0f;
        bool m_LoggedFirstFrame = false;

        bool m_IgnoreRootTranslation = false;
    };

} // namespace our
