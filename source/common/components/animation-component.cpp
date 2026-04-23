#include "animation-component.hpp"
#include "../asset-loader.hpp"
#include "../animation/animated-mesh.hpp"

#include <iostream>

namespace our {

    void AnimationComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) {
            std::cerr << "[ANIM] AnimationComponent deserialize called with non-object data" << std::endl;
            return;
        }

        if (!data.contains("animatedMesh")) {
            std::cerr << "[ANIM] AnimationComponent missing required key \"animatedMesh\"" << std::endl;
            return;
        }

        // Get the animated mesh by name from the asset loader
        if (data.contains("animatedMesh")) {
            std::string meshName = data["animatedMesh"].get<std::string>();
            animatedMesh = AssetLoader<AnimatedMesh>::get(meshName);

            if (animatedMesh == nullptr) {
                std::cerr << "[ANIM] AnimationComponent: animated mesh \"" << meshName << "\" not found" << std::endl;
                return;
            }

            std::cout << "[ANIM] AnimationComponent: loaded animated mesh \"" << meshName
                      << "\" with " << animatedMesh->boneCounter << " bones" << std::endl;

            // Load all available animations from the scene
            const aiScene* scene = animatedMesh->GetScene();
            if (scene != nullptr) {
                std::cout << "[ANIM] Scene has " << scene->mNumAnimations << " animation(s) available" << std::endl;
                for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
                    auto anim = std::make_unique<Animation>(animatedMesh, i);
                    if (anim->IsValid()) {
                        std::cout << "[ANIM] Loaded animation[" << i << "]: \"" << anim->GetName() << "\"" << std::endl;
                        animations.push_back(std::move(anim));
                    } else {
                        std::cerr << "[ANIM] Failed to load animation at index " << i << std::endl;
                    }
                }
            }

            // Create animator with global inverse transform
            animator = std::make_unique<Animator>(nullptr, animatedMesh->globalInverseTransform);

            // Select the requested animation
            currentAnimationIndex = data.value("animationIndex", 0);
            if (!animations.empty()) {
                playAnimation(currentAnimationIndex);
                std::cout << "[ANIM] AnimationComponent active animation index is "
                          << currentAnimationIndex << std::endl;
            } else {
                std::cerr << "[ANIM] AnimationComponent: no valid animations were loaded for \""
                          << meshName << "\"" << std::endl;
            }
        }
    }

    void AnimationComponent::update(float deltaTime) {
        if (animator) {
            animator->UpdateAnimation(deltaTime);
        }
    }

    bool AnimationComponent::playAnimation(int index) {
        if (index < 0 || index >= static_cast<int>(animations.size())) {
            std::cerr << "[ANIM] playAnimation: index " << index
                      << " out of range (0-" << animations.size() - 1 << ")" << std::endl;
            return false;
        }
        currentAnimationIndex = index;
        if (animator) {
            animator->PlayAnimation(animations[index].get());
            std::cout << "[ANIM] Switched to animation[" << index << "]: \""
                      << animations[index]->GetName() << "\"" << std::endl;
        }
        return true;
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
