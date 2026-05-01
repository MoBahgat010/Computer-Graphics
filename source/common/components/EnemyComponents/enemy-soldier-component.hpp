#pragma once
#include "../../ecs/component.hpp"
namespace our {

enum class EnemyState {
    IDLE,
    CHASING, // run after the player , the player in the detection range
    ATTACKING, // the player in the attack range
    DEAD // health <= 0
};


class EnemySoldierComponent : public Component {
    protected:
      double health=100.0;
      float attackDamage=3.0;
      float attackRange=10.0;
      float detectionRange=20.0;
      float speed=1.0;
      bool isDead =false;
      float attackCooldown=1.0;
      float attackTimer=0.0; // let say it is a stopwatch 
      
      // Stamina system variables
      float maxChaseDuration = 5.0f; // Can run for 5 seconds
      float restDuration = 3.0f;     // Needs to rest for 3 seconds
      float chaseTimer = 0.0f;
      bool isResting = false;

      EnemyState currentState = EnemyState::IDLE;
      
    public:

    static std::string getID() { return "EnemySoldier"; }
    void deserialize(const nlohmann::json& data) override;

    // getters 
    float getHealth();
    float getDamage();
    float getAttackRange();
    float getDetectionRange();
    float getSpeed();
    bool getIsDead();
    float getAttackCooldown();
    float getAttackTimer();
    
    float getMaxChaseDuration();
    float getRestDuration();
    float getChaseTimer();
    bool getIsResting();

    EnemyState getCurrentState();
    // setters
    void setCurrentState(EnemyState state);
    void setIsDead(bool isDead) ;
    void setAttackTimer(float attackTimer);
    void setChaseTimer(float timer);
    void setIsResting(bool resting);

    void decreaseHealth(float amount);
    virtual void damage(float amount);
    
    
    
    
    
      
};


}