#pragma once
#include <application.hpp>
#include <imgui.h>

class GameOverState : public our::State {
    void onDraw(double deltaTime) override {
        // Clear the screen with a dark red tint
        glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void onImmediateGui() override {
        ImGuiIO& io = ImGui::GetIO();
        
        // Full screen window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
        
        ImGui::Begin("GameOver", nullptr, flags);
        
        // Draw a dark red overlay
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(0, 0), io.DisplaySize, 
            IM_COL32(50, 0, 0, 200)
        );

        // "GAME OVER" Text
        ImGui::SetWindowFontScale(4.0f);
        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x / 2.0f - 180.0f, io.DisplaySize.y / 2.0f - 100.0f));
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "GAME OVER");
        ImGui::SetWindowFontScale(1.0f);

        // Return to Menu Button
        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x / 2.0f - 100.0f, io.DisplaySize.y / 2.0f + 20.0f));
        if (ImGui::Button("Return to Menu", ImVec2(200, 50))) {
            getApp()->changeState("start-screen");
        }

        ImGui::End();
    }
};
