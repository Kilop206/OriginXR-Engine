#include "originxr/editor/EditorModel.hpp"
#include "originxr/SceneIO.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
using namespace originxr;
using namespace originxr::editor;
#define CHECK(x) do {if(!(x)){std::cerr<<"FAIL "<<__FILE__<<':'<<__LINE__<<" "<<#x<<'\n';std::exit(1);}}while(0)
bool closeTo(float a,float b,float e=.0001f) {return std::abs(a-b)<e;}
int main() {
    Scene scene;
    auto& parent=scene.createEntity("Parent");
    const EntityId pid=parent.id;
    parent.transform.position={2,0,-3};
    parent.transform.scale={2,2,2};
    auto& child=scene.createEntity("Child");
    const EntityId cid=child.id;
    child.transform.position={1,0,0};
    CHECK(scene.setParent(cid,pid));
    CHECK(scene.find(cid)->parent==pid);
    CHECK(closeTo(scene.worldMatrix(cid).transformPoint({0,0,0}).x,4.f));
    CHECK(!scene.setParent(pid,cid)); // prevent cycles
    CHECK(!scene.setParent(pid,pid));
    CHECK(!scene.setParent(pid,99999));
    CHECK(scene.validHierarchy());
    UndoHistory history(3);
    history.checkpoint(scene);
    scene.find(cid)->transform.position.x=10;
    CHECK(history.undo(scene));
    CHECK(closeTo(scene.find(cid)->transform.position.x,1.f));
    CHECK(history.redo(scene));
    CHECK(closeTo(scene.find(cid)->transform.position.x,10.f));
    history.checkpoint(scene);
    scene.find(cid)->transform.position.x=5;
    CHECK(history.undo(scene));
    history.checkpoint(scene);
    scene.find(cid)->transform.position.x=7;
    CHECK(!history.redo(scene)); // changes invalidate redo
    const std::string file="originxr_editor_scene_v3.oxscene";
    std::string error;
    CHECK(saveScene(file,scene,error));
    Scene loaded;
    CHECK(loadScene(file,loaded,error));
    CHECK(loaded.find(pid) && loaded.find(cid));
    CHECK(loaded.find(cid)->parent==pid);
    CHECK(closeTo(loaded.find(cid)->transform.position.x,7.f));
    std::remove(file.c_str());
    // Reject missing parent and cycles transactionally.
    const std::string invalid="originxr_editor_invalid.oxscene";
    { std::ofstream out(invalid);out<<"originxr_scene 3\nentity 1 2 \"A\" 0 0 0 0 0 0 1 1 1 1 1 1 0 0 0 1 \"builtin:cube\" \"\" 0\n"; }
    CHECK(!loadScene(invalid,loaded,error));
    CHECK(loaded.find(cid)!=nullptr);
    std::remove(invalid.c_str());
    // Preserve v1/v2 files from the v0.2 release.
    Scene old;
    CHECK(loadScene(std::string(ORIGINXR_ASSET_DIR)+"/scenes/legacy-v1.oxscene",old,error));
    CHECK(!old.entities().empty());
    CHECK(loadScene(std::string(ORIGINXR_ASSET_DIR)+"/scenes/demo.oxscene",old,error));
    CHECK(old.entities().size()>=5);
    CHECK(old.validHierarchy());
    Scene sample;
    CHECK(loadScene(std::string(ORIGINXR_ASSET_DIR)+"/scenes/hierarchy-demo.oxscene",sample,error));
    CHECK(sample.entities().size()==4);
    CHECK(sample.find(2)->parent==1 && sample.find(3)->parent==1);
    // Picking: camera looking down -Z should select closer cube, not farther one.
    Scene picking;
    Entity& nearBox=picking.createEntity("near");
    nearBox.transform.position={0,1.65f,-2.f};
    const EntityId nearer=nearBox.id;
    Entity& farBox=picking.createEntity("far");
    farBox.transform.position={0,1.65f,-5.f};
    Camera camera;
    camera.position={0,1.65f,0};
    Ray ray=cameraRay(camera,400,300,800,600);
    CHECK(closeTo(ray.direction.z,-1.f));
    CHECK(pickEntity(picking,ray)==nearer);
    CHECK(pickEntity(picking,cameraRay(camera,0,0,800,600))==0);
    // A rotated entity should be pickable via inverse-affine ray casting.
    Scene rotated;
    Entity& box=rotated.createEntity("rotated");
    box.transform.position={0,1.65f,-3};
    box.transform.rotation.y=45.f;
    CHECK(pickEntity(rotated,ray)==box.id);
    CHECK(scene.remove(pid));
    CHECK(scene.find(cid)->parent==0); // deleting parent detaches children
    std::cout<<"OriginXR editor model tests passed.\n";
}
