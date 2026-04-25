#pragma once
#include "enemy-soldier-component.hpp"
namespace our {
  class OpusBossComponent : public EnemySoldierComponent{
    private:
    bool isSheildActive=true;
    
    public:
      static std::string getID();
      void deserialize(const nlohmann::json& data) override;
      bool getIsSheildActive();
      void setIsSheildActive(bool isSheildActive);
      void damage(float amount) override;

  };
}