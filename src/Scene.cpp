#include "originxr/Scene.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace originxr {
Mat4 Transform::matrix() const {
    return Mat4::translation(position) * Mat4::rotationY(rotation.y)
        * Mat4::rotationX(rotation.x) * Mat4::rotationZ(rotation.z)
        * Mat4::scaling(scale);
}
Entity& Scene::createEntity(std::string name) {
    if (nextId_ == 0) throw std::overflow_error("Entity ID overflow");
    Entity e;
    e.id=nextId_++;
    e.name=std::move(name);
    entities_.push_back(e);
    return entities_.back();
}
Entity* Scene::restoreEntity(EntityId id, std::string name) {
    if (id==0 || id==UINT32_MAX || find(id)) return nullptr;
    Entity e; e.id=id; e.name=std::move(name);
    entities_.push_back(std::move(e));
    if (nextId_<=id) nextId_=id+1;
    return &entities_.back();
}
bool Scene::setParent(EntityId child, EntityId parent) {
    Entity* target=find(child);
    if (!target || child==parent || (parent && !find(parent))) return false;
    EntityId walk=parent;
    for (std::size_t i=0;walk && i<entities_.size();++i) {
        if (walk==child) return false;
        const Entity* p=find(walk);
        if (!p) return false;
        walk=p->parent;
    }
    if(walk) return false; // prior corrupted parent cycle
    target->parent=parent;
    return true;
}
bool Scene::validHierarchy() const {
    for (const auto& e:entities_) {
        EntityId walk=e.parent;
        for(std::size_t i=0;walk && i<entities_.size();++i) {
            if(walk==e.id) return false;
            const Entity* p=find(walk);
            if(!p) return false;
            walk=p->parent;
        }
        if(walk) return false;
    }
    return true;
}
Mat4 Scene::worldMatrix(EntityId id) const {
    const Entity* e=find(id);
    if(!e) return Mat4::identity();
    Mat4 world=e->transform.matrix();
    EntityId walk=e->parent;
    for(std::size_t i=0;walk && i<entities_.size();++i) {
        const Entity* p=find(walk);
        if(!p) break;
        world=p->transform.matrix()*world;
        walk=p->parent;
    }
    return world;
}
Entity* Scene::find(EntityId id) {
    auto it=std::find_if(entities_.begin(),entities_.end(),[id](const Entity& e){return e.id==id;});
    return it==entities_.end()?nullptr:&*it;
}
const Entity* Scene::find(EntityId id) const {
    auto it=std::find_if(entities_.begin(),entities_.end(),[id](const Entity& e){return e.id==id;});
    return it==entities_.end()?nullptr:&*it;
}
bool Scene::remove(EntityId id) {
    auto it=std::remove_if(entities_.begin(),entities_.end(),[id](const Entity& e){return e.id==id;});
    if(it==entities_.end()) return false;
    entities_.erase(it, entities_.end());
    for (auto& e:entities_) if(e.parent==id) e.parent=0; // detach direct children
    return true;
}
void Scene::clear() { entities_.clear(); nextId_=1; }
void Scene::update(float dt) {
    dt=std::clamp(dt,0.f,0.05f);
    for(auto& e:entities_) e.transform.rotation += e.spin.degreesPerSecond*dt;
}
}
