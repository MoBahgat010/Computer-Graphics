#include "server-component.hpp"
namespace our {
  std::string ServerComponent::getID() {
    return "Server";
  }
  void ServerComponent::deserialize(const nlohmann::json& data) {
    Health = data.value("health", 100.0f);
  }
  void ServerComponent::decreaseHealth(float amount) {
    Health -= amount;
  }
  float ServerComponent::getHealth() {
    return Health;
  }
}