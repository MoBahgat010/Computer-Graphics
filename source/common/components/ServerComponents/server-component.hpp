#pragma once
#include "../../ecs/component.hpp"
#include <string>

namespace our {
  class ServerComponent : public Component {
    private:
      float Health=100.0;
      bool isDestroyed=false;
    public:
      static std::string getID() { return "Server"; }
      void deserialize(const nlohmann::json& data) override;
      void decreaseHealth(float amount);
      float getHealth();
      bool getIsDestroyed();
      void setIsDestroyed(bool destroyed);
  };
}