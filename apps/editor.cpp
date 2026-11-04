#include "originxr/Assets.hpp"
#include "originxr/Camera.hpp"
#include "originxr/Input.hpp"
#include "originxr/Renderer.hpp"
#include "originxr/SceneIO.hpp"
#include "originxr/editor/EditorModel.hpp"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>

using namespace originxr;
namespace {
struct Rect {ImVec2 min{},max{};bool valid{false};};
float segmentDistance(ImVec2 p,ImVec2 a,ImVec2 b) {
    const float dx=b.x-a.x,dy=b.y-a.y;
    const float len2=dx*dx+dy*dy;
    const float t=len2>0.001f?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/len2,0.f,1.f):0.f;
    const float x=p.x-(a.x+t*dx),y=p.y-(a.y+t*dy);
    return std::sqrt(x*x+y*y);
}
Vec3 axisVector(int axis) {
    return axis==0?Vec3{1,0,0}:(axis==1?Vec3{0,1,0}:Vec3{0,0,1});
}
void copyText(char* dest,std::size_t capacity,const std::string& source) {
    std::snprintf(dest,capacity,"%s",source.c_str());
}
class EditorApp {
public:
    explicit EditorApp(std::string sceneFile):sceneFile_(std::move(sceneFile)) {
        copyText(scenePath_.data(),scenePath_.size(),sceneFile_);
        assets_.setRoot(std::filesystem::path(sceneFile_).parent_path().string());
        std::string error;
        if(!loadScene(sceneFile_,scene_,error))message_="Scene not loaded: "+error;
        else message_="Loaded "+sceneFile_;
    }
    int run() {
        if(!glfwInit()) {std::cerr<<"GLFW initialization failed\n";return 1;}
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
#endif
        GLFWwindow* window=glfwCreateWindow(1500,900,"OriginXR Editor v0.3",nullptr,nullptr);
        if(!window) {std::cerr<<"OpenGL 3.3 window creation failed\n";glfwTerminate();return 2;}
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        Renderer renderer;
        std::string error;
        if(!renderer.initialize(error)) {
            std::cerr<<"Renderer initialization failed: "<<error<<'\n';
            renderer.shutdown();glfwDestroyWindow(window);glfwTerminate();return 3;
        }
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
        ImGui_ImplGlfw_InitForOpenGL(window,true);
        ImGui_ImplOpenGL3_Init("#version 330");
        double last=glfwGetTime();
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            const double now=glfwGetTime();
            const float dt=static_cast<float>(std::clamp(now-last,0.0,0.05));last=now;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            if(play_)scene_.update(dt);
            drawMenu(window);
            drawPanels();
            doShortcuts();
            updateCamera(window,dt);
            int fw=0,fh=0,ww=0,wh=0;
            glfwGetFramebufferSize(window,&fw,&fh);
            glfwGetWindowSize(window,&ww,&wh);
            ImGui::Render();
            if(fw>0 && fh>0) {
                // Clear entire default framebuffer, then render 3D only into the viewport.
                glDisable(GL_SCISSOR_TEST);
                glViewport(0,0,fw,fh);
                glClearColor(0.045f,0.05f,0.065f,1.f);
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                glEnable(GL_SCISSOR_TEST);
                if(viewport_.valid && ww>0 && wh>0) {
                    const float sx=static_cast<float>(fw)/static_cast<float>(ww);
                    const float sy=static_cast<float>(fh)/static_cast<float>(wh);
                    const int x=static_cast<int>(std::lround(viewport_.min.x*sx));
                    const int y=fh-static_cast<int>(std::lround(viewport_.max.y*sy));
                    const int width=static_cast<int>(std::lround((viewport_.max.x-viewport_.min.x)*sx));
                    const int height=static_cast<int>(std::lround((viewport_.max.y-viewport_.min.y)*sy));
                    if(width>0 && height>0) {
                        const float aspect=static_cast<float>(width)/static_cast<float>(height);
                        RenderView view{camera_.projection(aspect)*camera_.view(),x,y,width,height};
                        renderer.renderViews(scene_,{view},assets_,wireframe_);
                    }
                }
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);
            }
            stats_=renderer.stats();
        }
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        renderer.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
private:
    void refreshSelectedFields() {
        const Entity* e=scene_.find(selected_);
        copyText(nameBuffer_.data(),nameBuffer_.size(),e?e->name:"");
        copyText(meshBuffer_.data(),meshBuffer_.size(),e?e->mesh.asset:"");
        copyText(textureBuffer_.data(),textureBuffer_.size(),e?e->material.texture:"");
    }
    void setSelected(EntityId id) {
        if(selected_==id)return;
        selected_=id;
        refreshSelectedFields();
    }
    void validateSelected(){
        if(selected_ && !scene_.find(selected_))selected_=0;
        refreshSelectedFields();
    }
    void addCube() {
        history_.checkpoint(scene_);
        Entity& e=scene_.createEntity("Cube "+std::to_string(scene_.entities().size()+1));
        e.transform.position=camera_.position+camera_.forward()*3.f;
        setSelected(e.id);
        message_="Created cube #"+std::to_string(e.id);
    }
    void deleteSelection() {
        if(!selected_ || play_)return;
        history_.checkpoint(scene_);
        scene_.remove(selected_);setSelected(0);
        message_="Deleted selected entity";
    }
    void save() {
        if(play_){message_="Stop playback before saving";return;}
        std::string path(scenePath_.data());
        std::string error;
        if(saveScene(path,scene_,error)) {
            sceneFile_=path;
            assets_.setRoot(std::filesystem::path(path).parent_path().string());
            message_="Saved "+path;
        } else message_="Save error: "+error;
    }
    void open() {
        if(play_){message_="Stop playback before loading";return;}
        std::string path(scenePath_.data()),error;
        Scene loaded;
        if(!loadScene(path,loaded,error)){message_="Load error: "+error;return;}
        scene_=std::move(loaded);sceneFile_=path;assets_.setRoot(std::filesystem::path(path).parent_path().string());
        history_.clear();setSelected(0);message_="Loaded "+path;
    }
    void undo() {
        if(play_)return;
        if(history_.undo(scene_)){validateSelected();message_="Undo";}
    }
    void redo() {
        if(play_)return;
        if(history_.redo(scene_)){validateSelected();message_="Redo";}
    }
    void togglePlay() {
        if(!play_) {sceneBeforePlay_=scene_;play_=true;message_="Simulation running (changes temporary)";}
        else {play_=false;scene_=sceneBeforePlay_;validateSelected();message_="Simulation stopped; edit scene restored";}
    }
    void drawMenu(GLFWwindow* window) {
        if(ImGui::BeginMainMenuBar()) {
            if(ImGui::BeginMenu("File")) {
                if(ImGui::MenuItem("Open scene..."))requestOpenPopup_=true;
                if(ImGui::MenuItem("Save", "Ctrl+S",false,!play_))save();
                if(ImGui::MenuItem("Save As...",nullptr,false,!play_))requestSavePopup_=true;
                ImGui::Separator();
                if(ImGui::MenuItem("Quit"))glfwSetWindowShouldClose(window,GLFW_TRUE);
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Edit")) {
                if(ImGui::MenuItem("Undo", "Ctrl+Z",false,!play_&&history_.undoCount()>0))undo();
                if(ImGui::MenuItem("Redo", "Ctrl+Y",false,!play_&&history_.redoCount()>0))redo();
                if(ImGui::MenuItem("Delete selected", "Delete",false,!play_&&selected_))deleteSelection();
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Wireframe",nullptr,&wireframe_);
                if(ImGui::MenuItem("Reset camera"))camera_.reset();
                ImGui::EndMenu();
            }
            ImGui::TextDisabled("  OriginXR 0.3  |  Editor (desktop)");
            ImGui::EndMainMenuBar();
        }
    }
    void drawFilePopups() {
        if(requestOpenPopup_) {ImGui::OpenPopup("Open Scene");requestOpenPopup_=false;}
        if(requestSavePopup_) {ImGui::OpenPopup("Save As");requestSavePopup_=false;}
        if(ImGui::BeginPopupModal("Open Scene",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Path",scenePath_.data(),scenePath_.size());
            if(ImGui::Button("Open")) {open();ImGui::CloseCurrentPopup();}
            ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if(ImGui::BeginPopupModal("Save As",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Path",scenePath_.data(),scenePath_.size());
            if(ImGui::Button("Save")) {save();ImGui::CloseCurrentPopup();}
            ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    void beginPanel(const char* name,ImVec2 pos,ImVec2 size) {
        ImGui::SetNextWindowPos(pos,ImGuiCond_Always);
        ImGui::SetNextWindowSize(size,ImGuiCond_Always);
        ImGui::Begin(name,nullptr,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize|
                                ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    }
    void drawPanels() {
        const ImVec2 display=ImGui::GetIO().DisplaySize;
        const float top=ImGui::GetFrameHeight();
        const float footer=28.f;
        const float left=std::min(270.f,std::max(190.f,display.x*0.22f));
        const float right=std::min(350.f,std::max(210.f,display.x*0.25f));
        const float bottom=std::min(175.f,std::max(110.f,display.y*0.22f));
        const float middle=std::max(120.f,display.y-top-bottom-footer);
        beginPanel("Hierarchy",{0,top},{left,middle});drawHierarchy();ImGui::End();
        beginPanel("Inspector",{display.x-right,top},{right,middle});drawInspector();ImGui::End();
        beginPanel("Assets / Diagnostics",{0,top+middle},{display.x,bottom});drawDiagnostics();ImGui::End();
        beginPanel("Viewport",{left,top},{std::max(160.f,display.x-left-right),middle});drawViewport();ImGui::End();
        beginPanel("Status",{0,display.y-footer},{display.x,footer});
        ImGui::TextUnformatted(message_.c_str());ImGui::End();
    }
    void drawHierarchy() {
        if(ImGui::Button("+ Cube",ImVec2(-1,0)) && !play_) addCube();
        if(ImGui::Button("Scene Root  (drop to unparent)",ImVec2(-1,0)))setSelected(0);
        if(ImGui::BeginDragDropTarget()) {
            if(const ImGuiPayload* p=ImGui::AcceptDragDropPayload("ORIGINXR_ENTITY")) {
                const EntityId moved=*static_cast<const EntityId*>(p->Data);
                Scene test=scene_;
                if(test.setParent(moved,0)) {history_.checkpoint(scene_);scene_.setParent(moved,0);}
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::Separator();
        std::function<void(const Entity&)> draw=[&](const Entity& e) {
            bool children=false;
            for(const auto& candidate:scene_.entities())if(candidate.parent==e.id){children=true;break;}
            ImGuiTreeNodeFlags flags=ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_SpanAvailWidth;
            if(!children)flags|=ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if(selected_==e.id)flags|=ImGuiTreeNodeFlags_Selected;
            const bool opened=ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(e.id)),flags,
                                                "%s  [%u]",e.name.c_str(),e.id);
            if(ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())setSelected(e.id);
            if(!play_ && ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("ORIGINXR_ENTITY",&e.id,sizeof(e.id));
                ImGui::Text("Move %s",e.name.c_str());ImGui::EndDragDropSource();
            }
            if(!play_ && ImGui::BeginDragDropTarget()) {
                if(const ImGuiPayload* p=ImGui::AcceptDragDropPayload("ORIGINXR_ENTITY")) {
                    const EntityId moved=*static_cast<const EntityId*>(p->Data);
                    Scene test=scene_;
                    if(test.setParent(moved,e.id)) {history_.checkpoint(scene_);scene_.setParent(moved,e.id);}
                    else message_="Reparent rejected: would create a cycle";
                }
                ImGui::EndDragDropTarget();
            }
            if(children && opened) {
                for(const auto& child:scene_.entities())if(child.parent==e.id)draw(child);
                ImGui::TreePop();
            }
        };
        for(const auto& e:scene_.entities())if(e.parent==0)draw(e);
        ImGui::Separator();ImGui::Text("%zu entities",scene_.entities().size());
    }
    bool dragVector(const char* label,Vec3& value,float speed=0.05f) {
        const Scene before=scene_;
        const bool changed=ImGui::DragFloat3(label,&value.x,speed);
        if(ImGui::IsItemActivated())history_.checkpoint(before);
        return changed;
    }
    void drawInspector() {
        Entity* e=scene_.find(selected_);
        if(!e){ImGui::TextDisabled("Select an entity in the hierarchy or viewport.");return;}
        ImGui::BeginDisabled(play_);
        ImGui::Text("Entity #%u",e->id);
        if(ImGui::InputText("Name",nameBuffer_.data(),nameBuffer_.size(),ImGuiInputTextFlags_EnterReturnsTrue)) {
            history_.checkpoint(scene_);e->name=nameBuffer_.data();
        }
        ImGui::SeparatorText("Local transform");
        dragVector("Position",e->transform.position);
        dragVector("Rotation",e->transform.rotation,0.5f);
        dragVector("Scale",e->transform.scale);
        ImGui::SeparatorText("Mesh");
        if(ImGui::InputText("Asset",meshBuffer_.data(),meshBuffer_.size(),ImGuiInputTextFlags_EnterReturnsTrue)) {
            if(meshBuffer_[0]) {history_.checkpoint(scene_);e->mesh.asset=meshBuffer_.data();}
        }
        {
            Scene before=scene_;
            if(ImGui::Checkbox("Visible",&e->mesh.visible))history_.checkpoint(before);
        }
        ImGui::SeparatorText("Material");
        {
            Scene before=scene_;
            ImGui::ColorEdit3("Tint",&e->material.tint.x);
            if(ImGui::IsItemActivated())history_.checkpoint(before);
        }
        if(ImGui::InputText("Texture",textureBuffer_.data(),textureBuffer_.size(),ImGuiInputTextFlags_EnterReturnsTrue)) {
            history_.checkpoint(scene_);e->material.texture=textureBuffer_.data();
        }
        if(ImGui::Button("Clear texture")) {history_.checkpoint(scene_);e->material.texture.clear();textureBuffer_[0]='\0';}
        {
            Scene before=scene_;
            if(ImGui::Checkbox("Unlit",&e->material.unlit))history_.checkpoint(before);
        }
        ImGui::SeparatorText("Simulation");
        dragVector("Spin deg/s",e->spin.degreesPerSecond,0.5f);
        ImGui::Separator();
        if(ImGui::Button("Duplicate")) {
            history_.checkpoint(scene_);
            const Entity copy=*e;
            Entity& duplicate=scene_.createEntity(copy.name+" Copy");
            const EntityId newId=duplicate.id;
            duplicate=copy;duplicate.id=newId;
            setSelected(newId);
        }
        ImGui::SameLine();if(ImGui::Button("Delete"))deleteSelection();
        ImGui::EndDisabled();
    }
    void drawDiagnostics() {
        ImGui::Text("Scene: %s",sceneFile_.c_str());
        ImGui::Text("Draw calls: %u   Instances: %u   Triangles: %u   CPU render: %.3f ms",
                    stats_.drawCalls,stats_.instances,stats_.triangles,stats_.cpuMilliseconds);
        ImGui::Text("Assets loaded: %zu meshes, %zu textures",assets_.meshCount(),assets_.imageCount());
        ImGui::Text("History: %zu undo / %zu redo",history_.undoCount(),history_.redoCount());
        ImGui::Separator();
        ImGui::TextWrapped("Import OBJ: enter a path like ../meshes/pyramid.obj in the Mesh field (press Enter). Texture: PPM P3/P6.");
        ImGui::TextWrapped("WASD + RMB: fly camera | Scroll: dolly | Left click: pick | Drag colored axes: translate | Ctrl+S/Z/Y | Delete");
    }
    bool project(const Vec3& p,ImVec2& pixel) const {
        if(!viewport_.valid)return false;
        if(dot(p-camera_.position,camera_.forward())<=camera_.nearPlane)return false;
        const float w=viewport_.max.x-viewport_.min.x,h=viewport_.max.y-viewport_.min.y;
        if(w<=0.f || h<=0.f)return false;
        const Mat4 vp=camera_.projection(w/h)*camera_.view();
        const Vec3 ndc=vp.transformPoint(p);
        pixel={viewport_.min.x+(ndc.x+1.f)*.5f*w,viewport_.min.y+(1.f-ndc.y)*.5f*h};
        return true;
    }
    void drawGizmo(ImDrawList* list) {
        Entity* e=scene_.find(selected_);
        if(!e || play_)return;
        ImVec2 center;
        const Mat4 parent=e->parent?scene_.worldMatrix(e->parent):Mat4::identity();
        const Vec3 position=parent.transformPoint(e->transform.position);
        if(!project(position,center))return;
        const ImU32 colors[3]={IM_COL32(245,95,95,255),IM_COL32(110,225,120,255),IM_COL32(90,160,255,255)};
        const ImVec2 mouse=ImGui::GetIO().MousePos;
        int hot=-1;float nearest=11.f;
        for(int i=0;i<3;++i) {
            ImVec2 endpoint;
            if(!project(parent.transformPoint(e->transform.position+axisVector(i)*0.8f),endpoint))continue;
            const float dx=endpoint.x-center.x,dy=endpoint.y-center.y;
            const float len=std::sqrt(dx*dx+dy*dy);
            if(len<8.f)continue;
            list->AddLine(center,endpoint,colors[i],dragAxis_==i?5.f:3.f);
            list->AddCircleFilled(endpoint,5.f,colors[i]);
            const float d=segmentDistance(mouse,center,endpoint);
            if(d<nearest){nearest=d;hot=i;}
        }
        list->AddCircle(center,7.f,IM_COL32(255,220,80,255),0,2.f);
        if(dragAxis_<0 && viewportHovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hot>=0) {
            dragAxis_=hot;
            dragMouse_=mouse;
            dragFrom_=e->transform.position;
            dragCenter_=center;
            ImVec2 tip;
            if(project(parent.transformPoint(dragFrom_+axisVector(hot)),tip))
                dragPerUnit_={tip.x-center.x,tip.y-center.y};
            else dragPerUnit_={0.f,0.f};
            history_.checkpoint(scene_);
        }
        if(dragAxis_>=0) {
            if(!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {dragAxis_=-1;return;}
            const float denom=dragPerUnit_.x*dragPerUnit_.x+dragPerUnit_.y*dragPerUnit_.y;
            if(denom>16.f) {
                const float delta=((mouse.x-dragMouse_.x)*dragPerUnit_.x+
                                   (mouse.y-dragMouse_.y)*dragPerUnit_.y)/denom;
                e->transform.position=dragFrom_+axisVector(dragAxis_)*delta;
            }
        }
    }
    void drawViewport() {
        drawFilePopups();
        ImGui::BeginDisabled(play_);
        if(ImGui::Button("Select"))dragAxis_=-1;
        ImGui::SameLine();if(ImGui::Button("+ Cube"))addCube();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if(ImGui::Button(play_?"Stop":"Play"))togglePlay();
        ImGui::SameLine();ImGui::Checkbox("Wireframe",&wireframe_);
        ImGui::Separator();
        const ImVec2 available=ImGui::GetContentRegionAvail();
        viewport_.valid=available.x>16.f && available.y>16.f;
        if(!viewport_.valid)return;
        viewport_.min=ImGui::GetCursorScreenPos();
        viewport_.max={viewport_.min.x+available.x,viewport_.min.y+available.y};
        ImGui::InvisibleButton("##originxr_viewport",available,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);
        viewportHovered_=ImGui::IsItemHovered();
        auto* list=ImGui::GetWindowDrawList();
        list->AddRect(viewport_.min,viewport_.max,IM_COL32(85,95,112,255));
        drawGizmo(list);
        if(viewportHovered_ && !play_ && dragAxis_<0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 m=ImGui::GetIO().MousePos;
            const editor::Ray ray=editor::cameraRay(camera_,m.x-viewport_.min.x,m.y-viewport_.min.y,
                             available.x,available.y);
            setSelected(editor::pickEntity(scene_,ray));
        }
        list->AddText({viewport_.min.x+12.f,viewport_.min.y+12.f},IM_COL32(220,225,230,255),
                      "OriginXR viewport | RMB + WASD to navigate");
    }
    void updateCamera(GLFWwindow* window,float dt) {
        ImGuiIO& io=ImGui::GetIO();
        if(!viewportHovered_ || !ImGui::IsMouseDown(ImGuiMouseButton_Right) || io.WantTextInput)return;
        camera_.yaw+=io.MouseDelta.x*.12f;
        camera_.pitch=std::clamp(camera_.pitch-io.MouseDelta.y*.12f,-85.f,85.f);
        InputState input;
        input.set(InputAction::Forward,glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS);
        input.set(InputAction::Backward,glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS);
        input.set(InputAction::Left,glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS);
        input.set(InputAction::Right,glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS);
        input.set(InputAction::Up,glfwGetKey(window,GLFW_KEY_E)==GLFW_PRESS);
        input.set(InputAction::Down,glfwGetKey(window,GLFW_KEY_Q)==GLFW_PRESS);
        input.set(InputAction::Sprint,glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS);
        moveFreeCamera(camera_,input,dt);
    }
    void doShortcuts() {
        const ImGuiIO& io=ImGui::GetIO();
        if(io.WantTextInput)return;
        if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S,false))save();
        if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z,false))undo();
        if(io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y,false))redo();
        if(ImGui::IsKeyPressed(ImGuiKey_Delete,false) && !io.WantCaptureKeyboard)deleteSelection();
    }
    std::string sceneFile_,message_;
    Scene scene_,sceneBeforePlay_;
    AssetManager assets_;
    Camera camera_;
    editor::UndoHistory history_;
    EntityId selected_{0};
    Rect viewport_;
    bool viewportHovered_{false},play_{false},wireframe_{false};
    bool requestOpenPopup_{false},requestSavePopup_{false};
    int dragAxis_{-1};
    ImVec2 dragMouse_{},dragCenter_{},dragPerUnit_{};
    Vec3 dragFrom_{};
    RenderStats stats_{};
    std::array<char,1024> scenePath_{};
    std::array<char,256> nameBuffer_{};
    std::array<char,512> meshBuffer_{},textureBuffer_{};
};
}
int main(int argc,char** argv) {
    std::string scene=(std::filesystem::absolute(argv[0]).parent_path()/"scenes"/"demo.oxscene").string();
    if(argc>1)scene=argv[1];
    EditorApp app(scene);
    return app.run();
}
