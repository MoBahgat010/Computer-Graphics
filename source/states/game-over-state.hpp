#pragma once
#include <application.hpp>
#include <imgui.h>
#include <array>
#include <random>

class GameOverState : public our::State {
    struct QuoteEntry {
        const char* line;
        const char* author;
    };

    std::array<QuoteEntry, 10> quotes = {{
        {"Go back to LeetCode. Your arrays are still out of bounds.", "The Tech Lead"},
        {"Unfortunately, we are moving forward with candidates who didn't die here.", "Automated HR Reply"},
        {"Skill issue. This is why you are still a Junior Developer.", "The Senior Engineer"},
        {"You exited with status code 1. Should have containerized your health.", "The DevOps Team"},
        {"Your system design lacked a failover strategy. Clearly.", "The Interviewer"},
        {"Vector search failed: No survival skills retrieved.", "Your RAG Pipeline"},
        {"We are looking for someone with 5+ years of not-dying experience.", "The Recruiter"},
        {"git reset --hard HEAD. Try again.", "Version Control"},
        {"Never push to production on a Friday. Or play this game.", "The Sysadmin"},
        {"Error 500: Player motivation not found. Please restart server.", "Backend Error Log"}
    }};

    int selectedQuote = 0;

    void onInitialize() override {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> pick(0, (int)quotes.size() - 1);
        selectedQuote = pick(rng);
    }

    void onDraw(double deltaTime) override {
        // Cinematic pitch-black background.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

        // Keep the frame fully dark.
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(0, 0), io.DisplaySize, 
            IM_COL32(0, 0, 0, 255)
        );

        const float quoteWidth = io.DisplaySize.x * 0.68f;
        const float quoteX = (io.DisplaySize.x - quoteWidth) * 0.5f;
        const float quoteY = io.DisplaySize.y * 0.33f;

        ImGui::SetWindowFontScale(1.5f);
        ImGui::PushTextWrapPos(quoteX + quoteWidth);
        ImGui::SetCursorPos(ImVec2(quoteX, quoteY));
        ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "%s", quotes[selectedQuote].line);
        ImGui::PopTextWrapPos();

        ImGui::SetWindowFontScale(1.0f);
        ImGui::SetCursorPos(ImVec2(quoteX, quoteY + 72.0f));
        ImGui::TextColored(ImVec4(0.72f, 0.72f, 0.72f, 1.0f), "-- %s", quotes[selectedQuote].author);

        // Metallic rustic button at bottom center.
        ImVec2 buttonSize(250.0f, 56.0f);
        ImVec2 buttonPos((io.DisplaySize.x - buttonSize.x) * 0.5f, io.DisplaySize.y - 110.0f);
        ImGui::SetCursorPos(buttonPos);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.33f, 0.30f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.47f, 0.44f, 0.40f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.26f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.70f, 0.66f, 0.58f, 0.80f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.4f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        if (ImGui::Button("Back to Menu", buttonSize)) {
            getApp()->changeState("start-screen");
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        ImGui::End();
    }
};
