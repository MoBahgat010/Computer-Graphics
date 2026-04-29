#pragma once
#include "../../ecs/component.hpp"

namespace our {
class PlayerComponent : public Component {
  
  private:
  int health = 100;
  const int MAX_MAGAZINE_AMMO = 30;
  const int MAX_TOTAL_AMMO = 180;
  int currentMagazineAmmo = 30;

  int currentTotalAmmo = 180;
  int bulletDamage = 25; 
  bool isCrouch = false;
  bool isDead = false;
  float speed = 2.0f;
  float mouseSensitivity = 0.003f;
  float jumpSpeed = 2.5f;

  public:
  float damageIndicatorTimer = 0.0f;
  float painSoundTimer = 0.0f; // Cooldown timer for the pain sound
  
    static std::string getID() { return "Player"; }
    void deserialize(const nlohmann::json& data) override;

    

    // getters
    int getHealth() const;
    int getMagazineAmmo() const;
    int getTotalAmmo() const;
    int getBulletDamage() const;
    bool getIsCrouch()  const;
    bool getIsDead() const;
    float getSpeed() const;
    float getMouseSensitivity() const;
    float getJumpSpeed() const;


    // setters 

    void setIsCrouch(bool isCrouch);
    void setSpeed(float speed);

   
 


    void increaseHealth(int amount);
    void decreaseHealth(int amount);
    void decreaseMagazineAmmo(int amount = 1);
    void increaseTotalAmmo(int amount);
    void reloadWeapon();

    void receiveDamage(int amount); // Sets timer
};
}