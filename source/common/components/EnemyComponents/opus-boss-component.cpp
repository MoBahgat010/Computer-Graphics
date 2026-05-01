#include "opus-boss-component.hpp"
namespace our {
  std::string OpusBossComponent::getID() {
    return "OpusBoss";
  }
  void OpusBossComponent::deserialize(const nlohmann::json& data) {
    EnemySoldierComponent::deserialize(data);
    if(data.is_object()) {
      shieldRadius = data.value("shieldRadius", shieldRadius);
    }
  }
  bool OpusBossComponent::getIsSheildActive() {
    return isSheildActive;
  }
  void OpusBossComponent::setIsSheildActive(bool isSheildActive) {
    this->isSheildActive = isSheildActive;
  }
  float OpusBossComponent::getShieldRadius() const {
    return shieldRadius;
  }
  void OpusBossComponent::setShieldRadius(float radius) {
    shieldRadius = radius;
  }
  void OpusBossComponent::damage(float amount) {
    if(isSheildActive) {
      return;
    }
    
    EnemySoldierComponent::damage(amount);
  }
}