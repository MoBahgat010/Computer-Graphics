
#include "./player-component.hpp"
#include <algorithm>
namespace our {
  void PlayerComponent::deserialize(const nlohmann::json& data) {
    (void)data; // Intentionally ignored: defaults are defined in the component class.
  }


  // getters for the player component


  // getterts 

  int PlayerComponent::getHealth() const {
    return health;
  }
  int PlayerComponent::getMagazineAmmo() const {
    return currentMagazineAmmo;
  }
  int PlayerComponent::getTotalAmmo() const {
    return currentTotalAmmo;
  }
  int PlayerComponent::getBulletDamage() const {
    return bulletDamage;
  }
  bool PlayerComponent::getIsCrouch() const {
    return isCrouch;
  }

  bool PlayerComponent::getIsDead() const {
    return isDead;
  }






  void PlayerComponent::increaseHealth(int amount) {
    health += amount;
  }

  void PlayerComponent::decreaseHealth(int amount) {
    health -= amount;
    if (health <= 0){
      health = 0;
      isDead=true;
    } 
  }

  void PlayerComponent::decreaseMagazineAmmo(int amount) {
  
      currentMagazineAmmo -= amount;

      currentMagazineAmmo = std::max(currentMagazineAmmo, 0);
  }

  void PlayerComponent::increaseTotalAmmo(int amount) {
    currentTotalAmmo += amount;
    currentTotalAmmo = std::min(currentTotalAmmo, MAX_TOTAL_AMMO);
  }

  void PlayerComponent::reloadWeapon() {
    if(currentMagazineAmmo >= MAX_MAGAZINE_AMMO || currentTotalAmmo <= 0) return;
    int ammoNeeded = MAX_MAGAZINE_AMMO - currentMagazineAmmo;
    int ammoToReload = std::min(ammoNeeded, currentTotalAmmo);
    currentMagazineAmmo += ammoToReload;
    currentTotalAmmo -= ammoToReload;
  }





}