#include "player.hpp"

namespace our {

    void PlayerComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        maxHealth = data.value("maxHealth", maxHealth);
        health = data.value("health", maxHealth);
    }

}
