#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <audio/audio-player.hpp>

#include <string>
#include <vector>

class Level2EndingCutsceneState: public our::State {
    our::TexturedMaterial* slideMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    our::AudioPlayer backgroundMusic;
    std::vector<our::Texture2D*> slides;
    std::vector<std::string> musicPaths;
    size_t currentSlide = 0;
    float time = 0.0f;

    void playCurrentSlideVoice() {
        if(musicPaths.empty()) return;

        const size_t index = currentSlide < musicPaths.size() ? currentSlide : musicPaths.size() - 1;
        (void)backgroundMusic.play(musicPaths[index], 0.85f);
    }

    void onInitialize() override {
        getApp()->getMouse().unlockMouse(getApp()->getWindow());

        slideMaterial = new our::TexturedMaterial();
        slideMaterial->shader = new our::ShaderProgram();
        slideMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        slideMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        slideMaterial->shader->link();
        slideMaterial->tint = glm::vec4(0.0f);

        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }, {
            0, 1, 2, 2, 3, 0,
        });

        const std::vector<std::string> slidePaths = {
            "assets/images/storyEnd/end-1.png",
            "assets/images/storyEnd/end-2.png",
            "assets/images/storyEnd/end-3.png"
        };

        musicPaths = {
            "assets/audio/storyEnd/final1.mp3",
            "assets/audio/storyEnd/final2.mp3",
            "assets/audio/storyEnd/final3.mp3"
        };

        slides.reserve(slidePaths.size());
        for(const auto& path : slidePaths) {
            if(auto* texture = our::texture_utils::loadImage(path)) {
                slides.push_back(texture);
            }
        }

        currentSlide = 0;
        time = 0.0f;
        playCurrentSlideVoice();
    }

    void onImmediateGui() override {
        const glm::ivec2 size = getApp()->getFrameBufferSize();
        const ImVec2 buttonSize(210.0f, 58.0f);
        const float margin = 24.0f;

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

        ImGui::SetNextWindowPos(ImVec2((float)size.x - buttonSize.x - margin, (float)size.y - buttonSize.y - margin), ImGuiCond_Always);
        ImGui::SetNextWindowSize(buttonSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        if(ImGui::Begin("##story_end_continue_button", nullptr, flags)) {
            const bool isLastSlide = slides.empty() || currentSlide >= slides.size() - 1;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.14f, 0.18f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.24f, 0.96f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.62f, 0.11f, 0.15f, 0.98f));
            if(ImGui::Button("Continue", ImVec2(-1.0f, -1.0f))) {
                if(isLastSlide) {
                    getApp()->changeState("credits");
                } else {
                    currentSlide++;
                    time = 0.0f;
                    playCurrentSlideVoice();
                }
            }
            ImGui::PopStyleColor(3);
        }
        ImGui::End();

        ImGui::PopStyleVar(3);
    }

    void onDraw(double deltaTime) override {
        const glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(slides.empty()) {
            return;
        }

        slideMaterial->texture = slides[currentSlide];

        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        time += (float)deltaTime;
        slideMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 0.45f, time));
        slideMaterial->setup();
        slideMaterial->shader->use();
        slideMaterial->shader->set("transform", VP * M);
        rectangle->draw();
    }

    void onDestroy() override {
        backgroundMusic.stop();

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
};
