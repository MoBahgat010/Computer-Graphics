#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>

class SettingsState: public our::State {
    our::TexturedMaterial* backgroundMaterial = nullptr;
    our::Mesh* rectangle = nullptr;

    void onInitialize() override {
        backgroundMaterial = new our::TexturedMaterial();
        backgroundMaterial->shader = new our::ShaderProgram();
        backgroundMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        backgroundMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        backgroundMaterial->shader->link();
        // Load the settings screen image
        backgroundMaterial->texture = our::texture_utils::loadImage("assets/images/settingsScreen/SettingsScreen.png");
        backgroundMaterial->tint = glm::vec4(1.0f);

        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }, {
            0, 1, 2, 2, 3, 0,
        });
    }

    void onDraw(double deltaTime) override {
        // Return to start screen if ESC is pressed
        if(getApp()->getKeyboard().justPressed(GLFW_KEY_ESCAPE)) {
            getApp()->changeState("start-screen");
            return;
        }

        glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        backgroundMaterial->setup();
        backgroundMaterial->shader->use();
        backgroundMaterial->shader->set("transform", VP * M);
        rectangle->draw();
    }

    void onDestroy() override {
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
