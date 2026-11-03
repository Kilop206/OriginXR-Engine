#include "originxr/SceneIO.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace originxr {
namespace {
bool readVec(std::istream& in, Vec3& v) { return static_cast<bool>(in >> v.x >> v.y >> v.z); }
void writeVec(std::ostream& out, const Vec3& v) { out << ' ' << v.x << ' ' << v.y << ' ' << v.z; }
}
bool loadScene(const std::string& path, Scene& output, std::string& error) {
    std::ifstream file(path);
    if (!file) { error="Cannot open scene: "+path; return false; }
    Scene parsed;
    std::string line;
    std::size_t row=0;
    int version=0;
    while (std::getline(file,line)) {
        ++row;
        const auto first=line.find_first_not_of(" \t\r");
        if(first==std::string::npos || line[first]=='#')continue;
        std::istringstream in(line);
        std::string marker;in>>marker;
        if(!version) {
            if(marker!="originxr_scene" || !(in>>version) || (version!=1&&version!=2)) {
                error="Unsupported scene header at line "+std::to_string(row);return false;
            }
            std::string extra;
            if(in>>extra) {error="Extra header fields at line "+std::to_string(row);return false;}
            continue;
        }
        if(marker!="entity") {error="Unknown token at line "+std::to_string(row);return false;}
        std::string name,meshId="builtin:cube",textureId;
        Vec3 pos,rot,scale,color,spin;
        int visible=1,unlit=0;
        if(!(in>>std::quoted(name)) || !readVec(in,pos) || !readVec(in,rot) ||
           !readVec(in,scale) || !readVec(in,color) || !readVec(in,spin) || !(in>>visible)
           || (visible!=0 && visible!=1)) {
            error="Invalid entity at line "+std::to_string(row);return false;
        }
        if(version>=2 && (!(in>>std::quoted(meshId)>>std::quoted(textureId)>>unlit) ||
                          (unlit!=0&&unlit!=1) || meshId.empty())) {
            error="Invalid v2 materials at line "+std::to_string(row);return false;
        }
        std::string extra;
        if(in>>extra) {error="Extra fields at line "+std::to_string(row);return false;}
        Entity& e=parsed.createEntity(name);
        e.transform.position=pos;e.transform.rotation=rot;e.transform.scale=scale;
        e.material.tint=color;e.spin.degreesPerSecond=spin;e.mesh.visible=visible!=0;
        e.mesh.asset=meshId;e.material.texture=textureId;e.material.unlit=unlit!=0;
    }
    if(!version) {error="Scene has no header";return false;}
    output=std::move(parsed);error.clear();return true;
}
bool saveScene(const std::string& path, const Scene& scene, std::string& error) {
    std::ofstream file(path);
    if(!file) {error="Cannot save scene: "+path;return false;}
    file<<"# OriginXR scene, units in meters, angles in degrees\noriginxr_scene 2\n";
    file<<std::setprecision(9);
    for(const Entity& e:scene.entities()) {
        file<<"entity "<<std::quoted(e.name);
        writeVec(file,e.transform.position);writeVec(file,e.transform.rotation);
        writeVec(file,e.transform.scale);writeVec(file,e.material.tint);
        writeVec(file,e.spin.degreesPerSecond);
        file<<' '<<(e.mesh.visible?1:0)<<' '<<std::quoted(e.mesh.asset)<<' '
            <<std::quoted(e.material.texture)<<' '<<(e.material.unlit?1:0)<<'\n';
    }
    if(!file) {error="Write error: "+path;return false;}
    error.clear();return true;
}
} // namespace originxr
