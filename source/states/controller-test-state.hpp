#pragma once

#include <application.hpp>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

class ControllerTestState : public our::State {
    GLFWgamepadstate previous{};
    bool hasPrevious = false;
    float statusTimer = 0.0f;

    static bool axisChanged(float current, float previous, float epsilon = 0.08f) {
        return std::fabs(current - previous) > epsilon;
    }

public:
    void onInitialize() override {
        std::cout << "[ControllerTest] Entered controller test state" << std::endl;
        hasPrevious = false;
        statusTimer = 0.0f;
    }

    void onDraw(double deltaTime) override {
        if(getApp()->getKeyboard().justPressed(GLFW_KEY_ESCAPE)) {
            getApp()->changeState("start-screen");
            return;
        }

        statusTimer -= static_cast<float>(deltaTime);

        if(!glfwJoystickPresent(GLFW_JOYSTICK_1) || !glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
            if(statusTimer <= 0.0f) {
                std::cout << "[ControllerTest] No gamepad detected" << std::endl;
                statusTimer = 1.0f;
            }
            hasPrevious = false;
            return;
        }

        GLFWgamepadstate current{};
        if(!glfwGetGamepadState(GLFW_JOYSTICK_1, &current)) return;

        if(hasPrevious) {
            for(int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i) {
                if(current.buttons[i] != previous.buttons[i]) {
                    std::cout << "[ControllerTest] Button " << i
                              << (current.buttons[i] ? " pressed" : " released")
                              << std::endl;
                }
            }

            for(int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; ++i) {
                if(axisChanged(current.axes[i], previous.axes[i])) {
                    std::cout << "[ControllerTest] Axis " << i
                              << " = " << current.axes[i]
                              << std::endl;
                }
            }
        } else {
            std::cout << "[ControllerTest] Gamepad detected, printing changes" << std::endl;
        }

        previous = current;
        hasPrevious = true;
    }
};
