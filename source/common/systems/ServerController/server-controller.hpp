#pragma once
#include "../../ecs/world.hpp"
#include "../jolt-physics-system.hpp"
#include "../../components/ServerComponents/server-component.hpp"
#include "../../components/EnemyComponents/opus-boss-component.hpp"
#include "../../audio/audio-player.hpp"

namespace our {

    // This system handles the server logic, specifically checking health and removing destroyed servers.
    class ServerControllerSystem {
        JoltPhysicsSystem* physics = nullptr;
    public:
        void enter(JoltPhysicsSystem* physics) {
            this->physics = physics;
        }

        void update(World* world, float deltaTime) {
            if(!world) return;
            
            // Collect entities to remove to avoid modifying the set while iterating (though markForRemoval handles this)
            int serverCount = 0;
            for(auto entity : world->getEntities()) {
                auto* server = entity->getComponent<ServerComponent>();
                if(server) {
                    // If the server health reaches zero, mark it as destroyed and remove it from the world
                    if(server->getHealth() <= 0.0f && !server->getIsDestroyed()) {
                        server->setIsDestroyed(true);

                        // Play server destroyed sound
                        static AudioPlayer serverCrashAudioPlayer;
                        serverCrashAudioPlayer.play("assets/audio/game/server_crash_sound.mp3", 0.8f);

                        if(physics) physics->removeBody(entity);
                        world->markForRemoval(entity);
                    } else {
                        serverCount++;
                    }
                }
            }

            // If all servers are gone, lower Opus's shield
            if(serverCount == 0) {
                for(auto entity : world->getEntities()) {
                    if(auto* opus = entity->getComponent<OpusBossComponent>()) {
                        if(opus->getIsSheildActive()) {
                            opus->setIsSheildActive(false);
                            std::cout << "OPUS SHIELD DEACTIVATED! All servers destroyed." << std::endl;
                        }
                    }
                }
            }
        }
    };

}
