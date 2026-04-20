#pragma once

#include <glm/glm.hpp>
#include <json/json.hpp>

namespace our {
    enum class LightType {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    struct Light {
    public:
        // Here we define our light. First member specifies its type.
        LightType type;
        // We also define the color & intensity of the light for each component of the Phong model (Ambient, Diffuse, Specular).
        glm::vec3 diffuse, specular, ambient;
        glm::vec3 position; // Used for Point and Spot Lights only
        glm::vec3 direction; // Used for Directional and Spot Lights only
        // This affects how the light will dim out as we go further from the light.
        // The formula is light_received = light_emitted / (a*d^2 + b*d + c) where a, b, c are the quadratic, linear and constant factors respectively.
        struct {
            float constant, linear, quadratic;
        } attenuation; // Used for Point and Spot Lights only
        // This specifies the inner and outer cone of the spot light.
        // The light power is 0 outside the outer cone, the light power is full inside the inner cone.
        // The light power is interpolated in between the inner and outer cone.
        struct {
            float inner, outer;
        } spot_angle; // Used for Spot Lights only

        // Deserializes the entity data and components from a json object
        void deserialize(const nlohmann::json&);
    };

}