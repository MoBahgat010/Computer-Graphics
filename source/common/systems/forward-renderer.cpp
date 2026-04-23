#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../animation/animated-mesh.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unordered_set>

namespace our {

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json& config){
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if(config.contains("sky")){
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));

            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram* skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            //TODO: (Req 10) Pick the correct pipeline state to draw the sky
            // Hints: the sky will be draw after the opaque objects so we would need depth testing but which depth funtion should we pick?
            // We will draw the sphere from the inside, so what options should we pick for the face culling.
            PipelineState skyPipelineState{};
            // Enable depth testing with GL_LEQUAL so the sky passes when depth == 1.0 (far plane)
            // (we will set sky z to always equal w, so NDC z = 1 after perspective divide)
            skyPipelineState.depthTesting.enabled = true;
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            // We are drawing the sphere from the inside, so we must cull the FRONT face
            // (the outside faces point away from us when we are inside the sphere)
            skyPipelineState.faceCulling.enabled = true;
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.faceCulling.frontFace = GL_CCW;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky
            Sampler* skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if(config.contains("postprocess")){
            //TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

            //TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            // Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            // The depth format can be (Depth component with 24 bits).

            // Create and attach the color texture (RGBA8)
            colorTarget = texture_utils::empty(GL_RGBA8, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);

            // Create and attach the depth texture (DEPTH_COMPONENT24)
            depthTarget = texture_utils::empty(GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

            //TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }
    }

    void ForwardRenderer::destroy(){
        // Delete all objects related to the sky
        if(skyMaterial){
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if(postprocessMaterial){
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::render(World* world){
        static std::unordered_set<const Entity*> loggedMissingMaterial;
        static std::unordered_set<const Entity*> loggedMissingMesh;
        static std::unordered_set<const AnimationComponent*> loggedMissingAnimatedMesh;
        static std::unordered_set<const AnimationComponent*> loggedInactiveAnimation;
        static std::unordered_set<const AnimationComponent*> loggedActiveAnimation;
        static std::unordered_set<const AnimationComponent*> loggedBoneUpload;

        // Compute delta time
        static float lastFrameTime = 0.0f;
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f; // Clamp to avoid huge jumps

        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent* camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();
        for(auto entity : world->getEntities()){
            // If we hadn't found a camera yet, we look for a camera in this entity
            if(!camera) camera = entity->getComponent<CameraComponent>();
            // If this entity has a mesh renderer component
            if(auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer){
                std::string entityName = entity->name.empty() ? "<unnamed>" : entity->name;
                if(meshRenderer->material == nullptr){
                    if(loggedMissingMaterial.insert(entity).second){
                        std::cerr << "[ANIM] Skipping entity \"" << entityName
                                  << "\": missing material on Mesh Renderer component." << std::endl;
                    }
                    continue;
                }

                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;

                // Check if this entity also has an AnimationComponent
                auto animComp = entity->getComponent<AnimationComponent>();
                if (animComp) {
                    command.animationComponent = animComp;
                    // Use the AnimatedMesh's Mesh* for rendering
                    if (animComp->animatedMesh && animComp->animatedMesh->mesh) {
                        command.mesh = animComp->animatedMesh->mesh;
                    } else if(loggedMissingAnimatedMesh.insert(animComp).second) {
                        std::cerr << "[ANIM] Entity \"" << entityName
                                  << "\" has Animation component but no animated mesh is bound." << std::endl;
                    }

                    if(animComp->hasActiveAnimation()){
                        if(loggedActiveAnimation.insert(animComp).second){
                            std::cout << "[ANIM] Entity \"" << entityName << "\" is using animation index "
                                      << animComp->currentAnimationIndex << " with "
                                      << animComp->animations.size() << " loaded animation(s)." << std::endl;
                        }
                    } else if(loggedInactiveAnimation.insert(animComp).second) {
                        std::cerr << "[ANIM] Entity \"" << entityName
                                  << "\" has Animation component but no active animation. Loaded animations: "
                                  << animComp->animations.size() << std::endl;
                    }
                }

                if(command.mesh == nullptr){
                    if(loggedMissingMesh.insert(entity).second){
                        std::cerr << "[ANIM] Skipping entity \"" << entityName
                                  << "\": no mesh available for rendering." << std::endl;
                    }
                    continue;
                }

                // if it is transparent, we add it to the transparent commands list
                if(command.material->transparent){
                    transparentCommands.push_back(command);
                } else {
                // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if(camera == nullptr) return;

        //TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        // HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this one
        auto M = camera->getOwner()->getLocalToWorldMatrix();
        glm::vec3 cameraForward = glm::vec3(M * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand& first, const RenderCommand& second){
            //TODO: (Req 9) Finish this function
            // HINT: the following return should return true "first" should be drawn before "second".
            // Painter's algorithm: draw furthest objects first.
            return glm::dot(first.center, cameraForward) > glm::dot(second.center, cameraForward);
        });

        //TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        //TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize
        glViewport(0, 0, windowSize.x, windowSize.y);

        //TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0);

        //TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        // If there is a postprocess material, bind the framebuffer
        if(postprocessMaterial){
            //TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);
        }

        //TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //TODO: (Req 9) Draw all the opaque commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for(const auto& command : opaqueCommands){
            command.material->setup();
            command.material->shader->set("transform", VP * command.localToWorld);

            // Handle animation bone matrices
            if (command.animationComponent && command.animationComponent->hasActiveAnimation()) {
                command.animationComponent->update(deltaTime);
                const auto& boneMatrices = command.animationComponent->getBoneMatrices();
                if(loggedBoneUpload.insert(command.animationComponent).second){
                    std::string entityName = command.animationComponent->getOwner() && !command.animationComponent->getOwner()->name.empty()
                        ? command.animationComponent->getOwner()->name
                        : "<unnamed>";
                    std::cout << "[ANIM] Uploading "
                              << std::min(static_cast<int>(boneMatrices.size()), MAX_BONES)
                              << " bone matrices for entity \"" << entityName << "\"." << std::endl;
                }
                command.material->shader->set("useAnimation", (GLint)1);
                for (int i = 0; i < static_cast<int>(boneMatrices.size()) && i < MAX_BONES; ++i) {
                    command.material->shader->set("boneMatrices[" + std::to_string(i) + "]", boneMatrices[i]);
                }
            } else {
                command.material->shader->set("useAnimation", (GLint)0);
            }

            command.mesh->draw();
        }

        // If there is a sky material, draw the sky
        if(this->skyMaterial){
            //TODO: (Req 10) setup the sky material
            skyMaterial->setup();

            //TODO: (Req 10) Get the camera position
            // The camera position in world space is M * (0,0,0,1)
            glm::vec3 cameraPosition = glm::vec3(M * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            //TODO: (Req 10) Create a model matrix for the sky such that it always follows the camera (sky sphere center = camera position)
            glm::mat4 skyModelMatrix = glm::translate(glm::mat4(1.0f), cameraPosition);

            //TODO: (Req 10) We want the sky to be drawn behind everything (in NDC space, z=1)
            // We can achieve this by multiplying by an extra matrix after the projection.
            // We need clip_z = clip_w so that NDC z = clip_z / clip_w = 1 always.
            // The matrix below replaces z with w (column-major in GLM):
            //   row 0: [1, 0, 0, 0]  -> x_new = x
            //   row 1: [0, 1, 0, 0]  -> y_new = y
            //   row 2: [0, 0, 0, 1]  -> z_new = w  (so NDC z = w/w = 1)
            //   row 3: [0, 0, 0, 1]  -> w_new = w
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,  // col 0
                0.0f, 1.0f, 0.0f, 0.0f,  // col 1
                0.0f, 0.0f, 0.0f, 0.0f,  // col 2
                0.0f, 0.0f, 1.0f, 1.0f   // col 3
            );

            //TODO: (Req 10) set the "transform" uniform
            skyMaterial->shader->set("transform", alwaysBehindTransform * VP * skyModelMatrix);

            //TODO: (Req 10) draw the sky sphere
            skySphere->draw();
        }

        //TODO: (Req 9) Draw all the transparent commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for(const auto& command : transparentCommands){
            command.material->setup();
            command.material->shader->set("transform", VP * command.localToWorld);

            // Handle animation bone matrices for transparent objects too
            if (command.animationComponent && command.animationComponent->hasActiveAnimation()) {
                command.animationComponent->update(deltaTime);
                const auto& boneMatrices = command.animationComponent->getBoneMatrices();
                command.material->shader->set("useAnimation", (GLint)1);
                for (int i = 0; i < static_cast<int>(boneMatrices.size()) && i < MAX_BONES; ++i) {
                    command.material->shader->set("boneMatrices[" + std::to_string(i) + "]", boneMatrices[i]);
                }
            } else {
                command.material->shader->set("useAnimation", (GLint)0);
            }

            command.mesh->draw();
        }

        // If there is a postprocess material, apply postprocessing
        if(postprocessMaterial){
            //TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            //TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup();
            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

}
