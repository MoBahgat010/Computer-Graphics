#include <iostream>
#include <fstream>
#include <flags/flags.h>
#include <json/json.hpp>

#include <application.hpp>

#include "states/menu-state.hpp"
#include "states/start-screen-state.hpp"
#include "states/settings-state.hpp"
#include "states/game-over-state.hpp"
#include "states/story-beginning-state.hpp"
#include "states/play-state.hpp"
#include "states/level1-victory-state.hpp"
#include "states/level2-cutscene-state.hpp"
#include "states/level2-ending-cutscene-state.hpp"
#include "states/credits-state.hpp"
#include "states/shader-test-state.hpp"
#include "states/mesh-test-state.hpp"
#include "states/transform-test-state.hpp"
#include "states/pipeline-test-state.hpp"
#include "states/texture-test-state.hpp"
#include "states/sampler-test-state.hpp"
#include "states/material-test-state.hpp"
#include "states/entity-test-state.hpp"
#include "states/renderer-test-state.hpp"
#include "states/controller-test-state.hpp"

int main(int argc, char** argv) {
    
    flags::args args(argc, argv); // Parse the command line arguments
    // config_path is the path to the json file containing the application configuration
    // Default: "config/app.json"
    std::string config_path = args.get<std::string>("c", "config/app.jsonc");
    // run_for_frames is how many frames to run the application before automatically closing
    // This is useful for testing multiple configurations in a batch
    // Default: 0 where the application runs indefinitely until manually closed
    int run_for_frames = args.get<int>("f", 0);

    // Open the config file and exit if failed
    std::ifstream file_in(config_path);
    if(!file_in){
        std::cerr << "Couldn't open file: " << config_path << std::endl;
        return -1;
    }
    // Read the file into a json object then close the file
    nlohmann::json app_config = nlohmann::json::parse(file_in, nullptr, true, true);
    file_in.close();

    // Create the application
    our::Application app(app_config);
    
    // Register all the states of the project in the application
    app.registerState<Menustate>("menu");
    app.registerState<StartScreenState>("start-screen");
    app.registerState<SettingsState>("settings");
    app.registerState<GameOverState>("game-over");
    app.registerState<StoryBeginningState>("story-beginning");
    app.registerState<Playstate>("play");
    app.registerState<PlaystateLevel2>("play-level2");
    app.registerState<Level1VictoryState>("level1-victory");
    app.registerState<Level2CutsceneState>("level2-cutscene");
    app.registerState<Level2EndingCutsceneState>("level2-ending-cutscene");
    app.registerState<CreditsState>("credits");
    app.registerState<ShaderTestState>("shader-test");
    app.registerState<MeshTestState>("mesh-test");
    app.registerState<TransformTestState>("transform-test");
    app.registerState<PipelineTestState>("pipeline-test");
    app.registerState<TextureTestState>("texture-test");
    app.registerState<SamplerTestState>("sampler-test");
    app.registerState<MaterialTestState>("material-test");
    app.registerState<EntityTestState>("entity-test");
    app.registerState<RendererTestState>("renderer-test");
    app.registerState<ControllerTestState>("controller-test");
    // Then choose the state to run based on the option "start-scene" in the config
    if(app_config.contains(std::string{"start-scene"})){
        app.changeState(app_config["start-scene"].get<std::string>());
    }

    // Finally run the application
    // Here, the application loop will run till the terminatio condition is statisfied
    return app.run(run_for_frames);
}