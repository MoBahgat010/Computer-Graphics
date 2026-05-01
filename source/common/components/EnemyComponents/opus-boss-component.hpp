#pragma once
#include "enemy-soldier-component.hpp"
namespace our {
  class OpusBossComponent : public EnemySoldierComponent{
    private:
    bool isSheildActive=true;
    float shieldRadius = 1.35f;
    
    public:
      static std::string getID();
      void deserialize(const nlohmann::json& data) override;
      bool getIsSheildActive();
      void setIsSheildActive(bool isSheildActive);
      float getShieldRadius() const;
      void setShieldRadius(float radius);
      void damage(float amount) override;

  };
}