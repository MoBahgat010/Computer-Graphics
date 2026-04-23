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
  float speed = 5.0f;
  
  public:
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


    // setters 

    void setIsCrouch(bool isCrouch);
    void setIsDead(bool isDead);
    void setSpeed(float speed);

   
 


    void increaseHealth(int amount);
    void decreaseHealth(int amount);
    void decreaseMagazineAmmo(int amount = 1);
    void increaseTotalAmmo(int amount);
    void reloadWeapon();
};
}