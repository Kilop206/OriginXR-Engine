#include "originxr/Assets.hpp"
#include "originxr/SceneIO.hpp"
#include "originxr/XRMath.hpp"
#include "originxr/RenderView.hpp"
#include "originxr/Input.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace originxr;
#define CHECK(expr) do {if(!(expr)) {std::cerr<<"FAIL "<<__LINE__<<": " #expr <<'\n';return 1;}}while(0)
static bool near(float a,float b){return std::fabs(a-b)<1e-4f;}
int main() {
    std::string error;
    AssetManager assets;
    CHECK(assets.meshCount()==2);
    auto* cube=assets.mesh("builtin:cube",error);
    CHECK(cube && cube->valid() && cube->indices.size()==36);
    auto* plane=assets.mesh("builtin:plane",error);
    CHECK(plane && plane->valid() && plane->indices.size()==6);
    const std::string root=ORIGINXR_ASSET_DIR;
    assets.setRoot(root+"/scenes");
    auto* pyramid=assets.mesh("../meshes/pyramid.obj",error);
    CHECK(pyramid && pyramid->valid() && pyramid->indices.size()==18);
    CHECK(assets.mesh("../meshes/pyramid.obj",error)==pyramid); // cache
    auto* texture=assets.image("../textures/checker.ppm",error);
    CHECK(texture && texture->valid() && texture->width==8 && texture->height==8);
    CHECK(assets.image("../textures/checker.ppm",error)==texture);
    CHECK(!assets.mesh("missing.obj",error) && !error.empty());
    CHECK(!assets.image("missing.ppm",error) && !error.empty());
    CHECK(!assets.registerMesh("invalid",{},error));
    CHECK(!assets.registerMesh("builtin:cube",createCubeMesh(),error));
    MeshData failedMesh=createCubeMesh();failedMesh.indices[0]=90000;
    CHECK(!assets.registerMesh("broken",failedMesh,error));
    const std::string badObj="originxr_bad.obj";
    {std::ofstream o(badObj);o<<"v 0 0 0\nf 1 2 3\n";}
    MeshData mesh;
    CHECK(!loadObj(badObj,mesh,error));std::remove(badObj.c_str());
    const std::string badPpm="originxr_bad.ppm";
    {std::ofstream o(badPpm);o<<"P3\n2 2\n255\n23\n";}
    ImageData pixels;
    CHECK(!loadPpm(badPpm,pixels,error));std::remove(badPpm.c_str());
    Scene scene;
    auto& e=scene.createEntity("Mesh instance");
    e.mesh.asset="../meshes/pyramid.obj";
    e.material.texture="../textures/checker.ppm";
    e.material.tint={.2f,.3f,.4f};e.material.unlit=true;
    const std::string saved="originxr_v2_test.oxscene";
    CHECK(saveScene(saved,scene,error));
    Scene restored;CHECK(loadScene(saved,restored,error));
    std::remove(saved.c_str());
    CHECK(restored.entities().size()==1);
    CHECK(restored.entities().front().mesh.asset==e.mesh.asset);
    CHECK(restored.entities().front().material.texture==e.material.texture);
    CHECK(restored.entities().front().material.unlit);
    CHECK(near(restored.entities().front().material.tint.y,.3f));
    // Upgrade old v1 files in memory without breaking user scenes.
    Scene old;CHECK(loadScene(root+"/scenes/legacy-v1.oxscene",old,error));
    CHECK(old.entities().size()==1);
    CHECK(old.entities()[0].mesh.asset=="builtin:cube");
    CHECK(near(old.entities()[0].material.tint.x,.6f));
    XRPose pose;pose.position={2,3,4};
    auto view=makeXRView(pose);
    CHECK(near(view.v[12],-2.f)&&near(view.v[13],-3.f)&&near(view.v[14],-4.f));
    pose.qy=std::sin(kPi/4);pose.qw=std::cos(kPi/4);
    view=makeXRView(pose);
    auto origin=view.transformPoint(pose.position);
    CHECK(near(origin.x,0)&&near(origin.y,0)&&near(origin.z,0));
    auto projection=makeXRProjection({-.8f,.8f,.7f,-.7f});
    CHECK(projection.v[0]>0 && projection.v[5]>0);
    bool invalid=false;
    try {makeXRView({{},0,0,0,0});}catch(const std::invalid_argument&){invalid=true;}
    CHECK(invalid);
    RenderView rv;CHECK(rv.viewProjection.v[0]==1 && rv.width==0);
    Camera camera;
    InputState input;input.set(InputAction::Forward,true);
    const auto start=camera.position;
    moveFreeCamera(camera,input,.05f);
    CHECK(near(camera.position.z,start.z-.15f));
    input.set(InputAction::Forward,false);
    input.set(InputAction::Up,true);input.set(InputAction::Sprint,true);
    moveFreeCamera(camera,input,.05f);
    CHECK(near(camera.position.y,start.y+.35f));
    std::cout<<"OriginXR assets, scenes and XR math tests passed.\n";
}
