#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        //TODO: (Req 7) Write this function
        this->pipelineState.setup();
        this->shader->use();
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        if (data.contains("pipelineState")) {
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint 
    void TintedMaterial::setup() const {
        //TODO: (Req 7) Write this function
        our::Material::setup();
        this->shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json& data) {
        Material::deserialize(data);
        if (!data.is_object()) return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex" 
    void TexturedMaterial::setup() const {
        //TODO: (Req 7) Write this function
        TintedMaterial::setup();
        this->shader->set("alphaThreshold", alphaThreshold);
        if (texture != nullptr) {
            glActiveTexture(GL_TEXTURE0);
            texture->bind();
            if (sampler != nullptr)
                sampler->bind(0);
            this->shader->set("tex", 0);
        }
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data) {
        TintedMaterial::deserialize(data);
        if (!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }
    void LitMaterial::setup() const {
        pipelineState.setup();         // set pipeline state directly
        if (ambientShader) ambientShader->use();  // use ambient shader for pass 0
    }

    void LitMaterial::deserialize(const nlohmann::json& data) {
        if (data.contains("pipelineState")) pipelineState.deserialize(data["pipelineState"]);
        transparent = data.value("transparent", false);
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));

        ambientShader = AssetLoader<ShaderProgram>::get(data.value("ambientShader", ""));
        directionalShader = AssetLoader<ShaderProgram>::get(data.value("directionalShader", ""));
        pointShader = AssetLoader<ShaderProgram>::get(data.value("pointShader", ""));
        spotShader = AssetLoader<ShaderProgram>::get(data.value("spotShader", ""));

        shader = ambientShader; 
        if (data.contains("lights") && data["lights"].is_array())
            for (auto& name : data["lights"].get<std::vector<std::string>>())
                if (auto* l = AssetLoader<Light>::get(name)) lights.push_back(l);
    }

}