#include "animation-component.hpp"
#include "../asset-loader.hpp"
#include "../animation/animated-mesh.hpp"

#include <iostream>

namespace our {

    void AnimationComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object() || !data.contains("animatedMesh")) return;

        std::string meshName = data["animatedMesh"].get<std::string>();
        animatedMesh = AssetLoader<AnimatedMesh>::get(meshName);
        if (!animatedMesh) return;

        const aiScene* scene = animatedMesh->GetScene();
        if (scene) {
            for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
                auto anim = std::make_unique<Animation>(animatedMesh, i);
                if (anim->IsValid()) animations.push_back(std::move(anim));
            }
        }

        animator = std::make_unique<Animator>(nullptr, animatedMesh->globalInverseTransform);
        currentAnimationIndex = data.value("animationIndex", 0);
        inPlace = data.value("inPlace", false);
        if (animator) animator->setIgnoreRootTranslation(inPlace);

        if (!animations.empty()) playAnimation(currentAnimationIndex);
    }

    void AnimationComponent::update(float deltaTime) {
        if (animator) {
            animator->UpdateAnimation((paused || !isMoving) ? 0.0f : deltaTime);
        }
    }

    bool AnimationComponent::playAnimation(int index) {
        if (index < 0 || index >= static_cast<int>(animations.size())) return false;
        if (currentAnimationIndex == index && animator && animator->HasAnimation()) return true;
        
        currentAnimationIndex = index;
        if (animator) animator->PlayAnimation(animations[index].get());
        return true;
    }

    void AnimationComponent::setIsMoving(bool moving) {
        this->isMoving = moving;
    }

    const std::vector<glm::mat4>& AnimationComponent::getBoneMatrices() const {
        static const std::vector<glm::mat4> empty;
        if (animator) {
            return animator->GetFinalBoneMatrices();
        }
        return empty;
    }

    bool AnimationComponent::hasActiveAnimation() const {
        return animator && animator->HasAnimation();
    }

}
