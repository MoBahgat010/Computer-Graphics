#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <audio/audio-player.hpp>

// Start screen state that draws a fullscreen image and overlays menu buttons.
class StartScreenState: public our::State {
    our::TexturedMaterial* backgroundMaterial = nullptr;
    our::Mesh* rectangle = nullptr;
    our::AudioPlayer backgroundMusic;
    float time = 0.0f;

    void onInitialize() override {
        backgroundMaterial = new our::TexturedMaterial();
        backgroundMaterial->shader = new our::ShaderProgram();
        backgroundMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        backgroundMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        backgroundMaterial->shader->link();
        backgroundMaterial->texture = our::texture_utils::loadImage("assets/images/startScreen/GameStartMenu.png");
        backgroundMaterial->tint = glm::vec4(0.0f);

        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }, {
            0, 1, 2, 2, 3, 0,
        });

        (void)backgroundMusic.playLoop("assets/audio/startScreen/gameMenuAduio.mp3", 0.55f);

        time = 0.0f;
    }

    void onImmediateGui() override {
        auto size = getApp()->getFrameBufferSize();
        const ImVec2 panelSize(330.0f, 250.0f);
        const float margin = 160.0f;
        const ImVec2 panelPos((float)size.x - panelSize.x - margin, ((float)size.y - panelSize.y) * 0.60f);

        // Draw a soft shadow under the menu panel to lift it from the background.
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(panelPos.x + 8.0f, panelPos.y + 10.0f),
            ImVec2(panelPos.x + panelSize.x + 8.0f, panelPos.y + panelSize.y + 10.0f),
            IM_COL32(0, 0, 0, 120),
            16.0f
        );

        ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 18.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.78f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.15f, 0.25f, 0.45f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.97f, 0.99f, 1.00f));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        if(ImGui::Begin("##start_screen_buttons", nullptr, flags)) {
            ImGui::TextColored(ImVec4(1.00f, 0.30f, 0.35f, 1.00f), "MAIN MENU");


            ImVec2 dividerStart = ImGui::GetCursorScreenPos();
            float dividerWidth = ImGui::GetContentRegionAvail().x;
            ImGui::GetWindowDrawList()->AddLine(
                dividerStart,
                ImVec2(dividerStart.x + dividerWidth, dividerStart.y),
                IM_COL32(255, 75, 90, 170),
                2.0f
            );
            ImGui::Dummy(ImVec2(0.0f, 12.0f));

            const ImVec2 buttonSize(-1.0f, 44.0f);
            auto drawStyledButton = [&](const char* label, const ImVec4& base, const ImVec4& hover, const ImVec4& active) {
                ImGui::PushStyleColor(ImGuiCol_Button, base);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
                bool clicked = ImGui::Button(label, buttonSize);
                ImGui::PopStyleColor(3);
                return clicked;
            };

            if(drawStyledButton("Start", ImVec4(0.78f, 0.16f, 0.20f, 0.95f), ImVec4(0.90f, 0.22f, 0.27f, 1.00f), ImVec4(0.67f, 0.12f, 0.16f, 1.00f))) {
                getApp()->changeState("story-beginning");
            }

            if(drawStyledButton("StartDev", ImVec4(0.18f, 0.26f, 0.40f, 0.95f), ImVec4(0.24f, 0.34f, 0.52f, 1.00f), ImVec4(0.14f, 0.20f, 0.31f, 1.00f))) {
                   getApp()->changeState("play");
            }

            if(drawStyledButton("Exit", ImVec4(0.20f, 0.08f, 0.10f, 0.92f), ImVec4(0.28f, 0.10f, 0.13f, 1.00f), ImVec4(0.14f, 0.06f, 0.08f, 1.00f))) {
                getApp()->close();
            }
        }
        ImGui::End();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(6);
    }

    void onDraw(double deltaTime) override {
        glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        time += (float)deltaTime;
        backgroundMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 1.0f, time));
        backgroundMaterial->setup();

        // Explicitly use the shader before setting uniforms to avoid state mismatch.
        backgroundMaterial->shader->use();

        backgroundMaterial->shader->set("transform", VP * M);
        rectangle->draw();
    }

    void onDestroy() override {
        backgroundMusic.stop();

        delete rectangle;
        rectangle = nullptr;

        if(backgroundMaterial) {
            delete backgroundMaterial->texture;
            delete backgroundMaterial->shader;
            delete backgroundMaterial;
            backgroundMaterial = nullptr;
        }
    }
};