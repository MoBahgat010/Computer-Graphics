#include "server-component.hpp"
namespace our {
  // getID is defined as static in the header
  void ServerComponent::deserialize(const nlohmann::json& data) {
    Health = data.value("health", 100.0f);
  }
  void ServerComponent::decreaseHealth(float amount) {
    Health -= amount;
  }
  float ServerComponent::getHealth() {
    return Health;
  }
  bool ServerComponent::getIsDestroyed() {
    return isDestroyed;
  }
  void ServerComponent::setIsDestroyed(bool destroyed) {
    isDestroyed = destroyed;
  }
}