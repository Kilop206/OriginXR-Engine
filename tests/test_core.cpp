#include "originxr/Camera.hpp"
#include "originxr/Scene.hpp"
#include "originxr/SceneIO.hpp"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>

using namespace originxr;
#define CHECK(expr) do { if(!(expr)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " : " #expr << '\n'; std::exit(1); } } while(0)
static bool near(float a,float b) {return std::fabs(a-b)<1e-4f;}
int main() {
    CHECK(near(dot({1,2,3},{4,5,6}),32));
    Vec3 r=cross({1,0,0},{0,1,0});
    CHECK(near(r.z,1));
    Vec3 p=(Mat4::translation({2,1,-3})*Mat4::scaling({2,2,2})).transformPoint({1,1,1});
    CHECK(near(p.x,4)&&near(p.y,3)&&near(p.z,-1));
    Camera c;
    CHECK(near(c.forward().z,-1));
    CHECK(near(c.right().x,1));
    auto proj=c.projection(16.f/9.f,0.f);
    CHECK(proj.v[0]>0 && proj.v[11]==-1.f);
    CHECK(!near(c.projection(1.f,-0.032f).v[8],c.projection(1.f,0.032f).v[8]));
    Scene scene;
    Entity& e=scene.createEntity("Demo object");
    e.transform.position={3,2,-5};e.spin.degreesPerSecond={0,45,0};
    EntityId id=e.id;
    scene.update(0.02f);
    CHECK(near(scene.find(id)->transform.rotation.y,0.9f));
    CHECK(scene.find(9999)==nullptr);
    const std::string file="originxr_test_scene.oxscene";
    std::string error;
    CHECK(saveScene(file,scene,error));
    Scene loaded;
    CHECK(loadScene(file,loaded,error));
    CHECK(loaded.entities().size()==1);
    CHECK(loaded.entities().front().name=="Demo object");
    CHECK(near(loaded.entities().front().transform.position.x,3));
    CHECK(!loadScene("originxr_missing.oxscene",loaded,error));
    CHECK(loaded.entities().size()==1); // transactional failure
    CHECK(loaded.remove(loaded.entities().front().id));
    CHECK(loaded.entities().empty());
    std::remove(file.c_str());
    // Reject malformed scene without wiping the user's current scene.
    const std::string bad="originxr_invalid_scene.oxscene";
    {std::ofstream out(bad);out<<"originxr_scene 1\nentity \"Broken\" 0 1\n";}
    CHECK(!loadScene(bad,scene,error));
    CHECK(scene.entities().size()==1);
    std::remove(bad.c_str());
    // Verify the scene supplied with the sandbox also parses correctly.
    Scene supplied;
    const std::string demoPath=std::string(ORIGINXR_ASSET_DIR)+"/scenes/demo.oxscene";
    CHECK(loadScene(demoPath,supplied,error));
    CHECK(supplied.entities().size()>=5);
    std::cout<<"All OriginXR core tests passed.\n";
}
