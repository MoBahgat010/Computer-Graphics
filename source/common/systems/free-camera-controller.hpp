#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/PlayerComponents/player-component.hpp"
#include "jolt-physics-system.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

namespace our
{

    // The free camera controller system is responsible for moving every entity which contains a FreeCameraControllerComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic. 
    // For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem {
        Application* app; // The application in which the state runs
        bool mouse_locked = false; // Is the mouse locked
        JoltPhysicsSystem* joltPhysics = nullptr; // Optional physics bridge for FPS movement

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app, JoltPhysicsSystem* physics = nullptr){
            this->app = app;
            this->joltPhysics = physics;
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent 
        void update(World* world, float deltaTime) {
            // First of all, we search for an entity containing a CameraComponent.
            // If it also has a FreeCameraControllerComponent, we use its settings.
            // Otherwise, we fallback to default settings so movement still works.
            CameraComponent* camera = nullptr;
            FreeCameraControllerComponent *controller = nullptr;
            for(auto entity : world->getEntities()){
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if(camera) break;
            }
            // If there is no camera entity, we can do nothing.
            if(!camera) return;

            // Default settings if the scene doesn't provide a FreeCameraController component.
            FreeCameraControllerComponent defaultController;
            if(!controller) controller = &defaultController;
            // Get the entity that we found via getOwner of camera (we could use controller->getOwner())
            Entity* entity = camera->getOwner();

            // If the left mouse button is pressed, we lock and hide the mouse. This common in First Person Games.
            if(app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && !mouse_locked){
                app->getMouse().lockMouse(app->getWindow());
                mouse_locked = true;
            // If the left mouse button is released, we unlock and unhide the mouse.
            } else if(!app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && mouse_locked) {
                app->getMouse().unlockMouse(app->getWindow());
                mouse_locked = false;
            }

            // We get a reference to the entity's position and rotation
            glm::vec3& position = entity->localTransform.position;
            glm::vec3& rotation = entity->localTransform.rotation;

            // If the left mouse button is pressed, we get the change in the mouse location
            // and use it to update the camera rotation
            if(app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1)){
                glm::vec2 delta = app->getMouse().getMouseDelta();
                rotation.x -= delta.y * controller->rotationSensitivity; // The y-axis controls the pitch
                rotation.y -= delta.x * controller->rotationSensitivity; // The x-axis controls the yaw
            }

            // We prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
            if(rotation.x < -glm::half_pi<float>() * 0.99f) rotation.x = -glm::half_pi<float>() * 0.99f;
            if(rotation.x >  glm::half_pi<float>() * 0.99f) rotation.x  = glm::half_pi<float>() * 0.99f;
            // This is not necessary, but whenever the rotation goes outside the 0 to 2*PI range, we wrap it back inside.
            // This could prevent floating point error if the player rotates in single direction for an extremely long time. 
            rotation.y = glm::wrapAngle(rotation.y);

            // We update the camera fov based on the mouse wheel scrolling amount
            float fov = camera->fovY + app->getMouse().getScrollOffset().y * controller->fovSensitivity;
            fov = glm::clamp(fov, glm::pi<float>() * 0.01f, glm::pi<float>() * 0.99f); // We keep the fov in the range 0.01*PI to 0.99*PI
            camera->fovY = fov;

            // We get the camera model matrix (relative to its parent) to compute the front, up and right directions
            glm::mat4 matrix = entity->localTransform.toMat4();

            glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, -1, 0)),
                      up = glm::vec3(matrix * glm::vec4(0, 1, 0, 0)), 
                      right = glm::vec3(matrix * glm::vec4(1, 0, 0, 0));

            glm::vec3 current_sensitivity = controller->positionSensitivity;
            // If the LEFT SHIFT key is pressed, we multiply the position sensitivity by the speed up factor
            if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) current_sensitivity *= controller->speedupFactor;

            // Build keyboard movement vector from arrows
            glm::vec3 desiredVelocity(0.0f);

            // Ground-locked horizontal movement for FPS controls.
            glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
            if(glm::length(flatFront) < 1e-6f) flatFront = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 flatRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
            if(glm::length(flatRight) < 1e-6f) flatRight = glm::vec3(1.0f, 0.0f, 0.0f);

            if(app->getKeyboard().isPressed(GLFW_KEY_UP)) desiredVelocity += flatFront * current_sensitivity.z;
            if(app->getKeyboard().isPressed(GLFW_KEY_DOWN)) desiredVelocity -= flatFront * current_sensitivity.z;
            if(app->getKeyboard().isPressed(GLFW_KEY_RIGHT)) desiredVelocity += flatRight * current_sensitivity.x;
            if(app->getKeyboard().isPressed(GLFW_KEY_LEFT)) desiredVelocity -= flatRight * current_sensitivity.x;

            // Normalize diagonal movement so speed is consistent.
            float len = glm::length(desiredVelocity);
            if(len > 1e-6f) {
                float maxSpeed = glm::max(current_sensitivity.x, glm::max(current_sensitivity.y, current_sensitivity.z));
                desiredVelocity = (desiredVelocity / len) * maxSpeed;
            }

            bool canDriveWithPhysics = joltPhysics && joltPhysics->isInitialized() && (entity->getComponent<PlayerComponent>() != nullptr);
            if(canDriveWithPhysics) {
                // Drive the player using Jolt character velocity.
                joltPhysics->setPlayerEntity(entity);
                joltPhysics->setPlayerVelocity(desiredVelocity);
            } else {
                // Fallback: old free camera movement without physics.
                position += desiredVelocity * deltaTime;
            }
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){
            if(mouse_locked) {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
            }
        }

    };

}
