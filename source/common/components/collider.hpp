#pragma once

#include "../ecs/component.hpp"

#include <glm/glm.hpp>

namespace our {

    class ColliderComponent : public Component {
    public:
        glm::vec3 halfExtents = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 centerOffset = glm::vec3(0.0f, 0.0f, 0.0f);

        static std::string getID() { return "Collider"; }

        void deserialize(const nlohmann::json& data) override;
    };

}
