#include "enemy-soldier-component.hpp"

namespace our {

    float EnemySoldierComponent::getHealth() { return health; }
    float EnemySoldierComponent::getDamage() { return damage; }
    float EnemySoldierComponent::getAttackRange() { return attackRange; }
    float EnemySoldierComponent::getDetectionRange() { return detectionRange; }
    float EnemySoldierComponent::getSpeed() { return speed; }
    bool EnemySoldierComponent::getIsDead() { return isDead; }
    float EnemySoldierComponent::getAttackCooldown() { return attackCooldown; }
    float EnemySoldierComponent::getAttackTimer() { return attackTimer; }
    EnemyState EnemySoldierComponent::getCurrentState() { return currentState; }

    void EnemySoldierComponent::setCurrentState(EnemyState state) { currentState = state; }
    void EnemySoldierComponent::setIsDead(bool dead) { isDead = dead; }
    void EnemySoldierComponent::setAttackTimer(float timer) { attackTimer = timer; }

    void EnemySoldierComponent::decreaseHealth(float amount) {
        health -= amount;
        if(health <= 0) {
            health = 0;
            isDead = true;
            currentState = EnemyState::DEAD;
        }
    }

    void EnemySoldierComponent::deserialize(const nlohmann::json& data) {
        if(!data.is_object()) return;
        
        health = data.value("health", health);
        damage = data.value("damage", damage);
        attackRange = data.value("attackRange", attackRange);
        detectionRange = data.value("detectionRange", detectionRange);
        speed = data.value("speed", speed);
        attackCooldown = data.value("attackCooldown", attackCooldown);
    }

}