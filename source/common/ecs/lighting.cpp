#include "lighting.hpp"
#include <glm/gtc/constants.hpp>

namespace our {

    void Light::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;


        std::string typeStr = data.value("type", "directional");
        if (typeStr == "point") type = LightType::POINT;
        else if (typeStr == "spot")  type = LightType::SPOT;
        else                         type = LightType::DIRECTIONAL;

        if (data.contains("enabled"))
            enabled = data.value("enabled", false);
        
        if (data.contains("ambient"))
            ambient = glm::vec3(data["ambient"][0], data["ambient"][1], data["ambient"][2]);
        if (data.contains("diffuse"))
            diffuse = glm::vec3(data["diffuse"][0], data["diffuse"][1], data["diffuse"][2]);
        if (data.contains("specular"))
            specular = glm::vec3(data["specular"][0], data["specular"][1], data["specular"][2]);
        if (data.contains("position"))
            position = glm::vec3(data["position"][0], data["position"][1], data["position"][2]);
        if (data.contains("direction"))
            direction = glm::vec3(data["direction"][0], data["direction"][1], data["direction"][2]);

        if (data.contains("attenuation")) {
            auto& a = data["attenuation"];
            attenuation.constant = a.value("constant", 1.0f);
            attenuation.linear = a.value("linear", 0.09f);
            attenuation.quadratic = a.value("quadratic", 0.032f);
        }
        if (data.contains("spot_angle")) {
            auto& s = data["spot_angle"];
            spot_angle.inner = glm::radians(s.value("inner", 15.0f));
            spot_angle.outer = glm::radians(s.value("outer", 20.0f));
        }
    }

}