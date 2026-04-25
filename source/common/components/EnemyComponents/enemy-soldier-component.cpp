#include "enemy-soldier-component.hpp"

namespace our {

    float EnemySoldierComponent::getHealth() { return health; }
    float EnemySoldierComponent::getDamage() { return attackDamage; }
    float EnemySoldierComponent::getAttackRange() { return attackRange; }
    float EnemySoldierComponent::getDetectionRange() { return detectionRange; }
    float EnemySoldierComponent::getSpeed() { return speed; }
    bool EnemySoldierComponent::getIsDead() { return isDead; }
    float EnemySoldierComponent::getAttackCooldown() { return attackCooldown; }
    float EnemySoldierComponent::getAttackTimer() { return attackTimer; }
    float EnemySoldierComponent::getMaxChaseDuration() { return maxChaseDuration; }
    float EnemySoldierComponent::getRestDuration() { return restDuration; }
    float EnemySoldierComponent::getChaseTimer() { return chaseTimer; }
    bool EnemySoldierComponent::getIsResting() { return isResting; }

    EnemyState EnemySoldierComponent::getCurrentState() { return currentState; }

    void EnemySoldierComponent::setCurrentState(EnemyState state) { currentState = state; }
    void EnemySoldierComponent::setIsDead(bool dead) { isDead = dead; }
    void EnemySoldierComponent::setAttackTimer(float timer) { attackTimer = timer; }
    void EnemySoldierComponent::setChaseTimer(float timer) { chaseTimer = timer; }
    void EnemySoldierComponent::setIsResting(bool resting) { isResting = resting; }

    void EnemySoldierComponent::decreaseHealth(float amount) {
        health -= amount;
        if(health <= 0) {
            health = 0;
            isDead = true;
            currentState = EnemyState::DEAD;
        }
    }

    void EnemySoldierComponent::damage(float amount) {
        decreaseHealth(amount);
    }

    void EnemySoldierComponent::deserialize(const nlohmann::json& data) {
        if(!data.is_object()) return;
        
        health = data.value("health", health);
        attackDamage = data.value("damage", attackDamage);
        attackRange = data.value("attackRange", attackRange);
        detectionRange = data.value("detectionRange", detectionRange);
        speed = data.value("speed", speed);
        attackCooldown = data.value("attackCooldown", attackCooldown);
        maxChaseDuration = data.value("maxChaseDuration", maxChaseDuration);
        restDuration = data.value("restDuration", restDuration);
    }

}