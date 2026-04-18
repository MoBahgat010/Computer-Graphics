#pragma once

#include "../ecs/component.hpp"

namespace our {

    class PlayerComponent : public Component {
    public:
        float health = 100.0f;
        float maxHealth = 100.0f;

        static std::string getID() { return "Player"; }

        void deserialize(const nlohmann::json& data) override;
    };

}
