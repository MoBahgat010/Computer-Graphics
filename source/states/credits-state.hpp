#pragma once

#include <application.hpp>
#include <imgui.h>
#include <audio/audio-player.hpp>

class CreditsState : public our::State {
    our::AudioPlayer creditsAudioPlayer;

    void onInitialize() override {
        getApp()->getMouse().unlockMouse(getApp()->getWindow());
        (void)creditsAudioPlayer.playLoop("assets/audio/creditsScreen/end-game-audio.mp3", 0.1f);
    }

    void onDestroy() override {
        creditsAudioPlayer.stop();
    }

    void onDraw(double deltaTime) override {
        glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void onImmediateGui() override {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

        ImGui::Begin("Credits", nullptr, flags);

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(0, 0), io.DisplaySize,
            IM_COL32(6, 8, 12, 255)
        );

        ImGui::SetWindowFontScale(2.5f);
        const char* title = "CREDITS";
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - titleSize.x) * 0.5f, io.DisplaySize.y * 0.16f));
        ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.42f, 1.0f), "%s", title);

        ImGui::SetWindowFontScale(1.4f);
        const char* body = "Career Not Found 404\n\n"
                           "Thanks for playing.\n"
                           "You saved Earth from Opus.";
        ImVec2 bodySize = ImGui::CalcTextSize(body, nullptr, false, io.DisplaySize.x * 0.7f);
        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - bodySize.x) * 0.5f, io.DisplaySize.y * 0.34f));
        ImGui::PushTextWrapPos(io.DisplaySize.x * 0.85f);
        ImGui::TextColored(ImVec4(0.92f, 0.94f, 0.97f, 1.0f), "%s", body);
        ImGui::PopTextWrapPos();

        ImGui::SetWindowFontScale(1.0f);
        ImVec2 buttonSize(280.0f, 56.0f);
        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - buttonSize.x) * 0.5f, io.DisplaySize.y * 0.78f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.14f, 0.18f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.24f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.62f, 0.11f, 0.15f, 0.98f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

        if(ImGui::Button("Back to Main Menu", buttonSize)) {
            getApp()->changeState("start-screen");
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::End();
    }
};
