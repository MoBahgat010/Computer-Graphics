#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../animation/animated-mesh.hpp"
#include "../components/PlayerComponents/player-component.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unordered_set>
#include "../components/EnemyComponents/opus-boss-component.hpp"

namespace our {

    // Must be declared BEFORE render() so it can be called inside it
    static void uploadBoneMatrices(ShaderProgram* shader,
        AnimationComponent* animComp,
        float deltaTime) {
        if (!animComp || !animComp->hasActiveAnimation()) {
            shader->set("useAnimation", (GLint)0);
            return;
        }
        animComp->update(deltaTime);
        const auto& boneMatrices = animComp->getBoneMatrices();
        shader->set("useAnimation", (GLint)1);
        for (int i = 0; i < static_cast<int>(boneMatrices.size()) && i < MAX_BONES; ++i)
            shader->set("boneMatrices[" + std::to_string(i) + "]", boneMatrices[i]);
    }

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json& config) {
        this->windowSize = windowSize;

        if (config.contains("sky")) {
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));

            ShaderProgram* skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            PipelineState skyPipelineState{};
            skyPipelineState.depthTesting.enabled = true;
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            skyPipelineState.faceCulling.enabled = true;
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.faceCulling.frontFace = GL_CCW;

            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, false);

            Sampler* skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
        }

        // Opus shield resources
        opusShieldMesh = mesh_utils::sphere(glm::ivec2(24, 24));

        ShaderProgram* shieldShader = new ShaderProgram();
        shieldShader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        shieldShader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        shieldShader->link();

        PipelineState shieldPipelineState{};
        shieldPipelineState.depthTesting.enabled = true;
        shieldPipelineState.depthTesting.function = GL_LEQUAL;
        shieldPipelineState.depthMask = false; // transparent pass should not write depth

        shieldPipelineState.faceCulling.enabled = false; // view from all angles

        shieldPipelineState.blending.enabled = true;
        shieldPipelineState.blending.equation = GL_FUNC_ADD;
        shieldPipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        shieldPipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        opusShieldMaterial = new TintedMaterial();
        opusShieldMaterial->shader = shieldShader;
        opusShieldMaterial->pipelineState = shieldPipelineState;
        opusShieldMaterial->transparent = true;

        // Stronger blue, semi-transparent
        opusShieldMaterial->tint = glm::vec4(0.2f, 0.55f, 1.0f, 0.55f);
        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess")) {
            //TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

            colorTarget = texture_utils::empty(GL_RGBA8, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);

            depthTarget = texture_utils::empty(GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glGenVertexArrays(1, &postProcessVertexArray);

            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            postprocessMaterial->pipelineState.depthMask = false;
        }
    }

    void ForwardRenderer::destroy() {
        if (skyMaterial) {
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        if (opusShieldMaterial) {
            delete opusShieldMesh;
            delete opusShieldMaterial->shader;
            delete opusShieldMaterial;
            opusShieldMesh = nullptr;
            opusShieldMaterial = nullptr;
        }
        // Delete all objects related to post processing
        if (postprocessMaterial) {
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::render(World* world) {
        static std::unordered_set<const Entity*> loggedMissingMaterial;
        static std::unordered_set<const Entity*> loggedMissingMesh;
        static std::unordered_set<const AnimationComponent*> loggedMissingAnimatedMesh;
        static std::unordered_set<const AnimationComponent*> loggedInactiveAnimation;
        static std::unordered_set<const AnimationComponent*> loggedActiveAnimation;
        static std::unordered_set<const AnimationComponent*> loggedBoneUpload;

        static float lastFrameTime = 0.0f;
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        CameraComponent* camera = nullptr;
        PlayerComponent* player = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();

        for (auto entity : world->getEntities()) {
            if (!camera) camera = entity->getComponent<CameraComponent>();
            if (!player) player = entity->getComponent<PlayerComponent>();

            if (auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer) {
                std::string entityName = entity->name.empty() ? "<unnamed>" : entity->name;
                if (opusShieldMesh && opusShieldMaterial) {
                    if (auto* opusBoss = entity->getComponent<OpusBossComponent>()) {
                        if (opusBoss->getIsSheildActive()) { // keep existing project spelling
                            RenderCommand shieldCommand;
                            const glm::mat4 opusModel = entity->getLocalToWorldMatrix();

                            const float shieldRadius = opusBoss->getShieldRadius();
                            const glm::vec3 opusPosition = glm::vec3(opusModel[3]);
                            const glm::mat4 shieldTransform =
                                glm::translate(glm::mat4(1.0f), opusPosition) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(shieldRadius));
                            shieldCommand.localToWorld = shieldTransform;

                            shieldCommand.center = opusPosition;
                            shieldCommand.mesh = opusShieldMesh;
                            shieldCommand.material = opusShieldMaterial;
                            shieldCommand.animationComponent = nullptr;

                            transparentCommands.push_back(shieldCommand);
                        }
                    }
                }
                if (meshRenderer->material == nullptr) {
                    if (loggedMissingMaterial.insert(entity).second) {
                        std::cerr << "[ANIM] Skipping entity \"" << entityName
                            << "\": missing material on Mesh Renderer component." << std::endl;
                    }
                    continue;
                }

                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                command.animationComponent = nullptr;

                auto animComp = entity->getComponent<AnimationComponent>();
                if (animComp) {
                    command.animationComponent = animComp;
                    if (animComp->animatedMesh && animComp->animatedMesh->mesh) {
                        command.mesh = animComp->animatedMesh->mesh;
                    }
                    else if (loggedMissingAnimatedMesh.insert(animComp).second) {
                        std::cerr << "[RENDERER] \"" << entityName << "\": no animated mesh.\n";
                    }

                    if (animComp->hasActiveAnimation()) {
                        if (loggedActiveAnimation.insert(animComp).second)
                            std::cout << "[RENDERER] \"" << entityName << "\" playing anim "
                            << animComp->currentAnimationIndex << ".\n";
                    }
                    else if (loggedInactiveAnimation.insert(animComp).second) {
                        std::cerr << "[RENDERER] \"" << entityName << "\": no active animation.\n";
                    }
                }

                if (command.mesh == nullptr) {
                    if (loggedMissingMesh.insert(entity).second)
                        std::cerr << "[RENDERER] Skipping \"" << entityName << "\": no mesh.\n";
                    continue;
                }

                if (command.material->transparent)
                    transparentCommands.push_back(command);
                else
                    opaqueCommands.push_back(command);
            }
        }

        if (camera == nullptr) return;

        auto M = camera->getOwner()->getLocalToWorldMatrix();
        glm::vec3 cameraForward = glm::vec3(M * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        glm::vec3 cameraPos = glm::vec3(M[3]); // camera world position

        std::sort(transparentCommands.begin(), transparentCommands.end(),
            [cameraForward](const RenderCommand& first, const RenderCommand& second) {
            return glm::dot(first.center, cameraForward) > glm::dot(second.center, cameraForward);
        });

        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        glViewport(0, 0, windowSize.x, windowSize.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        if (postprocessMaterial)
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // OPAQUE PASS
        for (const auto& command : opaqueCommands) {

            auto* litMat = dynamic_cast<LitMaterial*>(command.material);

            // ── Non-lit: single draw ─────────────────────────────────────────
            if (!litMat) {
                command.material->setup();
                command.material->shader->set("transform", VP * command.localToWorld);

                if (command.animationComponent && command.animationComponent->hasActiveAnimation()) {
                    command.animationComponent->update(deltaTime);
                    const auto& boneMatrices = command.animationComponent->getBoneMatrices();
                    if (loggedBoneUpload.insert(command.animationComponent).second) {
                        std::string eName = command.animationComponent->getOwner()
                            ? command.animationComponent->getOwner()->name : "<unnamed>";
                        std::cout << "[RENDERER] Uploading "
                            << std::min((int)boneMatrices.size(), MAX_BONES)
                            << " bones for \"" << eName << "\".\n";
                    }
                    command.material->shader->set("useAnimation", (GLint)1);
                    for (int i = 0; i < (int)boneMatrices.size() && i < MAX_BONES; ++i)
                        command.material->shader->set("boneMatrices[" + std::to_string(i) + "]", boneMatrices[i]);
                }
                else {
                    command.material->shader->set("useAnimation", (GLint)0);
                }

                command.mesh->draw();
                continue; // <-- skip the lit path below
            }

            bool anyLightEnabled = false;
            for (const auto* light : litMat->lights) {
                if (light->enabled) { anyLightEnabled = true; break; }
            }
            if (!anyLightEnabled) {
                // If no lights are enabled, skip multipass and draw as unlit
                command.material->setup();
                command.material->shader->set("transform", VP * command.localToWorld);
                command.mesh->draw();
                continue;
            }

            // ── Lit material: multipass Phong ────────────────────────────────
            const glm::mat4 Mmodel = command.localToWorld;
            const glm::mat4 MVP = VP * Mmodel;
            const glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(Mmodel)));

            litMat->pipelineState.setup();

            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);   // first path must write to depth buffer
            glDepthFunc(GL_LESS);   
            glDisable(GL_BLEND);   

            // PASS 0: AMBIENT ONLY
            ShaderProgram* as = litMat->ambientShader;
            as->use();
            as->set("transform", MVP);
            as->set("model", Mmodel);
            as->set("normalMatrix", normalMat);
            as->set("cameraPos", cameraPos);
            uploadBoneMatrices(as, command.animationComponent, deltaTime);

            // Find the directional light for ambient pass
            const Light* dirLight = nullptr;
            for (const auto* light : litMat->lights) {
                if (light->enabled && light->type == LightType::DIRECTIONAL) {
                    dirLight = light;
                    break;
                }
            }

            for (const auto& batch : command.mesh->getDrawBatches()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, batch.diffuseTexture);
                if (litMat->sampler) litMat->sampler->bind(0);
                as->set("diffuseMap", 0);

                as->set("materialAmbient", batch.ambient);
                as->set("materialDiffuse", glm::vec3(0.0f));
                as->set("materialSpecular", glm::vec3(0.0f));
                as->set("materialShininess", batch.shininess);

                // Use actual light's ambient color
                if (dirLight) {
                    as->set("directional_light.ambient", dirLight->ambient);
                    as->set("directional_light.direction", glm::normalize(dirLight->direction));
                }
                else {
                    as->set("directional_light.ambient", glm::vec3(0.3f)); // Default fallback
                    as->set("directional_light.direction", glm::vec3(0, -1, 0));
                }
                as->set("directional_light.diffuse", glm::vec3(0.0f));
                as->set("directional_light.specular", glm::vec3(0.0f));

                command.mesh->drawBatch(batch);
            }

            // PASS 1: ONE DRAW PER LIGHT (additive)
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE, GL_ONE);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);

            for (const auto* light : litMat->lights) {
                if (!light->enabled) continue;  // <-- ADD THIS CHECK

                ShaderProgram* ls = nullptr;
                switch (light->type) {
                case LightType::DIRECTIONAL: ls = litMat->directionalShader; break;
                case LightType::POINT:       ls = litMat->pointShader;       break;
                case LightType::SPOT:        ls = litMat->spotShader;        break;
                }
                if (!ls) continue;

                ls->use();
                ls->set("transform", MVP);
                ls->set("model", Mmodel);
                ls->set("normalMatrix", normalMat);
                ls->set("cameraPos", cameraPos);
                uploadBoneMatrices(ls, command.animationComponent, deltaTime);

                switch (light->type) {
                case LightType::DIRECTIONAL: {
                    glm::vec3 dir = (glm::length(light->direction) > 0.001f) ? glm::normalize(light->direction) : glm::vec3(0, -1, 0);
                    ls->set("directional_light.ambient", light->ambient);
                    ls->set("directional_light.diffuse", light->diffuse);
                    ls->set("directional_light.specular", light->specular);
                    ls->set("directional_light.direction", dir);
                    break;
                }
                case LightType::POINT: {
                    ls->set("light.ambient", light->ambient);
                    ls->set("light.diffuse", light->diffuse);
                    ls->set("light.specular", light->specular);
                    ls->set("light.position", light->position);
                    ls->set("light.attenuation_constant", light->attenuation.constant);
                    ls->set("light.attenuation_linear", light->attenuation.linear);
                    ls->set("light.attenuation_quadratic", light->attenuation.quadratic);
                    break;
                }
                case LightType::SPOT: {
                    ls->set("light.ambient", light->ambient);
                    ls->set("light.diffuse", light->diffuse);
                    ls->set("light.specular", light->specular);
                    ls->set("light.position", light->position);
                    ls->set("light.direction", glm::normalize(light->direction));
                    ls->set("light.attenuation_constant", light->attenuation.constant);
                    ls->set("light.attenuation_linear", light->attenuation.linear);
                    ls->set("light.attenuation_quadratic", light->attenuation.quadratic);
                    ls->set("light.inner_angle", light->spot_angle.inner);
                    ls->set("light.outer_angle", light->spot_angle.outer);
                    break;
                }
                }

                for (const auto& batch : command.mesh->getDrawBatches()) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, batch.diffuseTexture);
                    if (litMat->sampler) litMat->sampler->bind(0);
                    ls->set("diffuseMap", 0);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, batch.specularTexture);
                    if (litMat->sampler) litMat->sampler->bind(1);
                    ls->set("specularMap", 1);

                    ls->set("materialAmbient", glm::vec3(0.0f));
                    ls->set("materialDiffuse", batch.diffuse);
                    ls->set("materialSpecular", batch.specular);
                    ls->set("materialShininess", batch.shininess);

                    command.mesh->drawBatch(batch);
                }
            }

            // Restore state
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

        } 

        // SKY
        if (this->skyMaterial) {
            skyMaterial->setup();
            glm::vec3 cameraPosition = glm::vec3(M * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            glm::mat4 skyModelMatrix = glm::translate(glm::mat4(1.0f), cameraPosition);
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 1.0f
            );
            skyMaterial->shader->set("transform", alwaysBehindTransform * VP * skyModelMatrix);
            skySphere->draw();
        }

        // TRANSPARENT PASS
        for (const auto& command : transparentCommands) {
            auto* litMat = dynamic_cast<LitMaterial*>(command.material);

            if (!litMat) {
                command.material->setup();
                command.material->shader->set("transform", VP * command.localToWorld);
                if (command.animationComponent && command.animationComponent->hasActiveAnimation()) {
                    command.animationComponent->update(deltaTime);
                    const auto& bm = command.animationComponent->getBoneMatrices();
                    command.material->shader->set("useAnimation", (GLint)1);
                    for (int i = 0; i < (int)bm.size() && i < MAX_BONES; ++i)
                        command.material->shader->set("boneMatrices[" + std::to_string(i) + "]", bm[i]);
                }
                else {
                    command.material->shader->set("useAnimation", (GLint)0);
                }
                command.mesh->draw();
                continue;
            }

            bool anyLightEnabled = false;
            for (const auto* light : litMat->lights) {
                if (light->enabled) { anyLightEnabled = true; break; }
            }
            if (!anyLightEnabled) {
                // If no lights are enabled, skip multipass and draw as unlit
                command.material->setup();
                command.material->shader->set("transform", VP * command.localToWorld);
                command.mesh->draw();
                continue;
            }

            // Single combined pass for transparent lit objects
            const glm::mat4 Mmodel = command.localToWorld;
            const glm::mat4 MVP = VP * Mmodel;
            const glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(Mmodel)));

            if (litMat->directionalShader) {
                litMat->pipelineState.setup();
                ShaderProgram* ls = litMat->directionalShader;
                ls->use();
                ls->set("transform", MVP);
                ls->set("model", Mmodel);
                ls->set("normalMatrix", normalMat);
                ls->set("cameraPos", cameraPos);
                uploadBoneMatrices(ls, command.animationComponent, deltaTime);

                for (const auto* light : litMat->lights) {
                    if (light->type != LightType::DIRECTIONAL) continue;
                    ls->set("directional_light.ambient", light->ambient);
                    ls->set("directional_light.diffuse", light->diffuse);
                    ls->set("directional_light.specular", light->specular);
                    ls->set("directional_light.direction", glm::normalize(light->direction));
                    break;
                }

                for (const auto& batch : command.mesh->getDrawBatches()) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, batch.diffuseTexture);
                    if (litMat->sampler) litMat->sampler->bind(0);
                    ls->set("diffuseMap", 0);

                    glActiveTexture(GL_TEXTURE1); // ADD: Bind specular texture
                    glBindTexture(GL_TEXTURE_2D, batch.specularTexture);
                    if (litMat->sampler) litMat->sampler->bind(1);
                    ls->set("specularMap", 1);

                    ls->set("materialAmbient", batch.ambient);
                    ls->set("materialDiffuse", batch.diffuse);
                    ls->set("materialSpecular", batch.specular);
                    ls->set("materialShininess", batch.shininess);
                    command.mesh->drawBatch(batch);
                }
            }
        }

        // POST PROCESS
        if (postprocessMaterial) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            postprocessMaterial->setup();

            // If player is hit, tint the screen red
            if (player && player->damageIndicatorTimer > 0.0f) {
                // tint heavily red when the timer is close to 0.5, fade back to white as it approaches 0
                float factor = player->damageIndicatorTimer / 0.5f; // assume max 0.5f
                postprocessMaterial->shader->set("tint", glm::vec4(1.0f, 1.0f - factor, 1.0f - factor, 1.0f));
            } else {
                postprocessMaterial->shader->set("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    } 
}