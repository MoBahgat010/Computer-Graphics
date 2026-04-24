#include "collider.hpp"
#include "../deserialize-utils.hpp"

namespace our {

    void ColliderComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        halfExtents = glm::abs(data.value("halfExtents", halfExtents));
        centerOffset = data.value("centerOffset", centerOffset);
    }

}
