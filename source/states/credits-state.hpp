#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <audio/audio-player.hpp>
#include <vector>
#include <string>

class CreditsState : public our::State {
    our::TexturedMaterial* slideMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    our::AudioPlayer creditsAudioPlayer;
    std::vector<our::Texture2D*> slides;
    size_t currentSlide = 0;
    float slideTimer = 0.0f;
    static constexpr float SlideDurationSeconds = 15.0f;

    void onInitialize() override {
        getApp()->getMouse().unlockMouse(getApp()->getWindow());

        slideMaterial = new our::TexturedMaterial();
        slideMaterial->shader = new our::ShaderProgram();
        slideMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        slideMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        slideMaterial->shader->link();
        slideMaterial->tint = glm::vec4(1.0f);

        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }, {
            0, 1, 2, 2, 3, 0,
        });

        const std::vector<std::string> slidePaths = {
            "assets/images/credits/credits-1.png",
            "assets/images/credits/credits-2.png",
            "assets/images/credits/credits-3.png",
            "assets/images/credits/credits-4.png",
            "assets/images/credits/credits-5.png",
            "assets/images/credits/credits-6.png"
        };

        slides.reserve(slidePaths.size());
        for(const auto& path : slidePaths) {
            if(auto* texture = our::texture_utils::loadImage(path)) {
                slides.push_back(texture);
            }
        }

        currentSlide = 0;
        slideTimer = 0.0f;
        (void)creditsAudioPlayer.playLoop("assets/audio/creditsScreen/end-game-audio.mp3", 0.1f);
    }

    void onDestroy() override {
        creditsAudioPlayer.stop();

        delete rectangle;
        rectangle = nullptr;

        for(auto* texture : slides) {
            delete texture;
        }
        slides.clear();

        if(slideMaterial) {
            delete slideMaterial->shader;
            delete slideMaterial;
            slideMaterial = nullptr;
        }
    }

    void onDraw(double deltaTime) override {
        const glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(slides.empty()) {
            return;
        }

        slideTimer += (float)deltaTime;
        while(slideTimer >= SlideDurationSeconds) {
            slideTimer -= SlideDurationSeconds;
            if(currentSlide + 1 >= slides.size()) {
                getApp()->changeState("start-screen");
                return;
            }
            currentSlide++;
        }

        slideMaterial->texture = slides[currentSlide];

        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        slideMaterial->setup();
        slideMaterial->shader->use();
        slideMaterial->shader->set("transform", VP * M);
        rectangle->draw();
    }
};
