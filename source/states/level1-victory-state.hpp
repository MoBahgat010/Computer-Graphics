#pragma once
#include <application.hpp>
#include <imgui.h>
#include <audio/audio-player.hpp>

class Level1VictoryState : public our::State {
    our::AudioPlayer victoryAudioPlayer;

    void onInitialize() override {
        // You can add a sound here later if you have one:
        // victoryAudioPlayer.play("assets/audio/game/victory.mp3", 0.5f);
        getApp()->getMouse().unlockMouse(getApp()->getWindow());
    }

    void onDestroy() override {
        victoryAudioPlayer.stop();
    }

    void onDraw(double deltaTime) override {
        // A dark cinematic background
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void onImmediateGui() override {
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
        
        ImGui::Begin("Level1Victory", nullptr, flags);

        // Draw a dark background block for styling
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(0, 0), io.DisplaySize, 
            IM_COL32(0, 0, 0, 200)
        );

        // Title text
        const float titleWidth = io.DisplaySize.x * 0.5f;
        const float titleX = (io.DisplaySize.x - titleWidth) * 0.5f;
        const float titleY = io.DisplaySize.y * 0.25f;

        ImGui::SetWindowFontScale(3.0f);
        ImGui::SetCursorPos(ImVec2(titleX, titleY));
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "LEVEL 1 CLEARED!");

        ImGui::SetWindowFontScale(1.5f);
        
        // Buttons
        ImVec2 buttonSize(400.0f, 60.0f);
        float buttonX = (io.DisplaySize.x - buttonSize.x) * 0.5f;
        float startY = io.DisplaySize.y * 0.45f;

        // Button Style
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

        // 1. Continue to Level 2 (Cutscene)
        ImGui::SetCursorPos(ImVec2(buttonX, startY));
        if (ImGui::Button("Continue to Level 2", buttonSize)) {
            getApp()->changeState("level2-cutscene");
        }

        // 2. Continue DEV (Gameplay Direct)
        ImGui::SetCursorPos(ImVec2(buttonX, startY + 90.0f));
        if (ImGui::Button("Continue DEV (Skip Cutscene)", buttonSize)) {
            getApp()->changeState("play-level2");
        }

        // 3. Return to Main Menu
        ImGui::SetCursorPos(ImVec2(buttonX, startY + 180.0f));
        if (ImGui::Button("Return to Main Menu", buttonSize)) {
            getApp()->changeState("menu");
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        ImGui::End();
    }
};
