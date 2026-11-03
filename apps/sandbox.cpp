#include "originxr/Engine.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <cmath>

int main(int argc,char** argv) {
    originxr::EngineConfig cfg;
    // Default scene is copied alongside the executable by CMake.
    cfg.scenePath=(std::filesystem::absolute(argv[0]).parent_path()/"scenes"/"demo.oxscene").string();
    for(int i=1;i<argc;++i) {
        if(std::string(argv[i])=="--stereo") cfg.stereoPreview=true;
        else cfg.scenePath=argv[i];
    }
    std::cout<<"OriginXR 0.2 - desktop renderer / stereo preview (not headset XR)\n";
    originxr::Engine engine(cfg);
    // Example of a user-defined system, completely outside the engine core.
    engine.onUpdate([elapsed=0.0f](originxr::Scene& scene,float dt) mutable {
        elapsed+=dt;
        for(auto& e:scene.entities()) {
            if(e.name=="Floating Satellite") {
                e.transform.position.y=2.6f+0.3f*std::sin(elapsed*1.5f);
            }
        }
    });
    return engine.run();
}
