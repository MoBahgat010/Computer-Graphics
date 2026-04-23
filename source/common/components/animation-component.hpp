#pragma once

#include "../ecs/component.hpp"
#include "../animation/animator.hpp"
#include "../animation/animation.hpp"
#include "../animation/animated-mesh.hpp"

#include <vector>
#include <memory>

namespace our {

    class AnimationComponent : public Component {
    public:
        std::vector<std::unique_ptr<Animation>> animations;
        std::unique_ptr<Animator> animator;
        AnimatedMesh* animatedMesh = nullptr; // Not owned, managed by AssetLoader
        int currentAnimationIndex = 0;

        static std::string getID() { return "Animation"; }

        void deserialize(const nlohmann::json& data) override;

        // Called each frame by the renderer
        void update(float deltaTime);

        // Switch to a different animation by index
        bool playAnimation(int index);

        // Get the final bone matrices for shader upload
        const std::vector<glm::mat4>& getBoneMatrices() const;

        bool hasActiveAnimation() const;
    };

}
