#include "opus-boss-component.hpp"
namespace our {
  std::string OpusBossComponent::getID() {
    return "OpusBoss";
  }
  void OpusBossComponent::deserialize(const nlohmann::json& data) {
    EnemySoldierComponent::deserialize(data);
  }
  bool OpusBossComponent::getIsSheildActive() {
    return isSheildActive;
  }
  void OpusBossComponent::setIsSheildActive(bool isSheildActive) {
    this->isSheildActive = isSheildActive;
  }
  void OpusBossComponent::damage(float amount) {
    if(isSheildActive) {
      return;
    }
    
    EnemySoldierComponent::damage(amount);
  }
}