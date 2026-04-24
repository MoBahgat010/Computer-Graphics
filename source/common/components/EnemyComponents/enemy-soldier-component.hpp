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
    private:
      double health=100.0;
      float damage=3.0;
      float attackRange=10.0;
      float detectionRange=20.0;
      float speed=1.0;
      bool isDead =false;
      float attackCooldown=1.0;
      float attackTimer=0.0; // let say it is a stopwatch 
      EnemyState currentState = EnemyState::IDLE;
      
    public:
    // getters 
    float getHealth();
    float getDamage();
    float getAttackRange();
    float getDetectionRange();
    float getSpeed();
    bool getIsDead();
    float getAttackCooldown();
    float getAttackTimer();
    EnemyState getCurrentState();
    // setters
    void setCurrentState(EnemyState state);
    void setIsDead(bool isDead) ;
    void setAttackTimer(float attackTimer);

    void decreaseHealth(float amount);
    
    
    
    
    
      
};


}