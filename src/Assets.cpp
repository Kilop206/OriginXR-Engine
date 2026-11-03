#include "originxr/Assets.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace originxr {
bool MeshData::valid() const {
    if(vertices.empty() || indices.empty() || indices.size()%3) return false;
    for(auto i:indices) if(i>=vertices.size()) return false;
    return true;
}
bool ImageData::valid() const {
    return width>0 && height>0 && rgb.size()==static_cast<std::size_t>(width)*static_cast<std::size_t>(height)*3;
}
MeshData createCubeMesh() {
    MeshData mesh;
    constexpr float h=.5f;
    struct Face { Vec3 n; std::array<Vec3,4> v; };
    const std::array<Face,6> faces{{
        {{0,0,1}, {{{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}}}},
        {{0,0,-1},{{{h,-h,-h},{-h,-h,-h},{-h,h,-h},{h,h,-h}}}},
        {{1,0,0}, {{{h,-h,h},{h,-h,-h},{h,h,-h},{h,h,h}}}},
        {{-1,0,0},{{{-h,-h,-h},{-h,-h,h},{-h,h,h},{-h,h,-h}}}},
        {{0,1,0}, {{{-h,h,h},{h,h,h},{h,h,-h},{-h,h,-h}}}},
        {{0,-1,0},{{{-h,-h,-h},{h,-h,-h},{h,-h,h},{-h,-h,h}}}}
    }};
    const std::array<std::array<float,2>,4> uv{{{{0,0}},{{1,0}},{{1,1}},{{0,1}}}};
    for(const auto& f:faces) {
        auto start=static_cast<std::uint32_t>(mesh.vertices.size());
        for(std::size_t k=0;k<4;++k) mesh.vertices.push_back({f.v[k],f.n,uv[k][0],uv[k][1]});
        for(auto i:{0u,1u,2u,0u,2u,3u}) mesh.indices.push_back(start+i);
    }
    return mesh;
}
MeshData createPlaneMesh() {
    MeshData m;
    m.vertices={{{-.5f,0,-.5f},{0,1,0},0,0},{{-.5f,0,.5f},{0,1,0},0,1},{{.5f,0,.5f},{0,1,0},1,1},{{.5f,0,-.5f},{0,1,0},1,0}};
    m.indices={0,1,2,0,2,3};
    return m;
}
AssetManager::AssetManager() {
    std::string error;
    registerMesh("builtin:cube",createCubeMesh(),error);
    registerMesh("builtin:plane",createPlaneMesh(),error);
}
bool AssetManager::registerMesh(std::string id,MeshData data,std::string& error) {
    if(id.empty() || !data.valid()) {error="Invalid mesh ID or geometry";return false;}
    if(meshes_.count(id)) {error="Mesh key already exists: "+id;return false;}
    meshes_.emplace(std::move(id),std::make_shared<const MeshData>(std::move(data)));
    error.clear();return true;
}
bool AssetManager::registerImage(std::string id,ImageData data,std::string& error) {
    if(id.empty() || !data.valid()) {error="Invalid image ID or pixels";return false;}
    if(images_.count(id)) {error="Image key already exists: "+id;return false;}
    images_.emplace(std::move(id),std::make_shared<const ImageData>(std::move(data)));
    error.clear();return true;
}
const MeshData* AssetManager::mesh(const std::string& id,std::string& error) {
    auto found=meshes_.find(id);
    if(found!=meshes_.end()) {error.clear();return found->second.get();}
    MeshData loaded;
    const auto resolved=std::filesystem::path(id).is_absolute() || root_.empty() ? id : (std::filesystem::path(root_)/id).string();
    if(!loadObj(resolved,loaded,error))return nullptr;
    registerMesh(id,std::move(loaded),error);
    return meshes_.at(id).get();
}
const ImageData* AssetManager::image(const std::string& id,std::string& error) {
    auto found=images_.find(id);
    if(found!=images_.end()) {error.clear();return found->second.get();}
    ImageData loaded;
    const auto resolved=std::filesystem::path(id).is_absolute() || root_.empty() ? id : (std::filesystem::path(root_)/id).string();
    if(!loadPpm(resolved,loaded,error))return nullptr;
    registerImage(id,std::move(loaded),error);
    return images_.at(id).get();
}
namespace {
struct Ref {int p=0,t=0,n=0;};
bool parseRef(const std::string& token,Ref& out) {
    try {
        auto a=token.find('/'),b=token.find('/',a==std::string::npos?0:a+1);
        out.p=std::stoi(token.substr(0,a));
        if(a!=std::string::npos && b!=a+1) {
            auto mid=token.substr(a+1,b==std::string::npos?b:b-a-1);
            if(!mid.empty()) out.t=std::stoi(mid);
        }
        if(b!=std::string::npos && b+1<token.size()) out.n=std::stoi(token.substr(b+1));
        return out.p!=0;
    } catch(...) {return false;}
}
int resolve(int objIndex,std::size_t count) {
    if(objIndex==0)return -1;
    const long long index=objIndex>0?static_cast<long long>(objIndex)-1:static_cast<long long>(count)+objIndex;
    return index>=0 && index<static_cast<long long>(count)?static_cast<int>(index):-1;
}
std::string nextToken(std::istream& in) {
    std::string token;
    while(in>>token) {
        if(token[0]=='#') {in.ignore(std::numeric_limits<std::streamsize>::max(),'\n');continue;}
        return token;
    }
    return {};
}
} // namespace
bool loadObj(const std::string& path,MeshData& result,std::string& error) {
    std::ifstream in(path);
    if(!in) {error="Cannot open OBJ: "+path;return false;}
    std::vector<Vec3> positions,normals;
    std::vector<std::array<float,2>> uvs;
    MeshData parsed;
    std::string line;
    int lineNumber=0;
    while(std::getline(in,line)) {
        ++lineNumber;
        std::istringstream tokens(line);
        std::string tag;tokens>>tag;
        if(tag.empty()||tag[0]=='#')continue;
        if(tag=="v") {Vec3 p; if(!(tokens>>p.x>>p.y>>p.z))goto malformed;positions.push_back(p);}
        else if(tag=="vn") {Vec3 n;if(!(tokens>>n.x>>n.y>>n.z))goto malformed;normals.push_back(normalized(n));}
        else if(tag=="vt") {std::array<float,2> uv{};if(!(tokens>>uv[0]>>uv[1]))goto malformed;uvs.push_back(uv);}
        else if(tag=="f") {
            std::vector<Vertex> polygon;
            std::string index;
            while(tokens>>index) {
                if(index[0]=='#')break;
                Ref ref;
                if(!parseRef(index,ref))goto malformed;
                int p=resolve(ref.p,positions.size());
                int t=ref.t?resolve(ref.t,uvs.size()):-1;
                int n=ref.n?resolve(ref.n,normals.size()):-1;
                if(p<0||(ref.t&&t<0)||(ref.n&&n<0))goto malformed;
                Vertex vertex;
                vertex.position=positions[static_cast<std::size_t>(p)];
                if(t>=0) {vertex.u=uvs[static_cast<std::size_t>(t)][0];vertex.v=1.f-uvs[static_cast<std::size_t>(t)][1];}
                if(n>=0) vertex.normal=normals[static_cast<std::size_t>(n)];
                else vertex.normal={0,0,0};
                polygon.push_back(vertex);
            }
            if(polygon.size()<3)goto malformed;
            for(std::size_t i=1;i+1<polygon.size();++i) {
                const auto face=normalized(cross(polygon[i].position-polygon[0].position,polygon[i+1].position-polygon[0].position));
                for(std::size_t j:{std::size_t(0),i,i+1}) {
                    Vertex v=polygon[j];
                    if(dot(v.normal,v.normal)<1e-8f)v.normal=face;
                    parsed.indices.push_back(static_cast<std::uint32_t>(parsed.vertices.size()));
                    parsed.vertices.push_back(v);
                }
            }
        }
        continue;
        malformed: error="Malformed OBJ at line "+std::to_string(lineNumber)+": "+path;return false;
    }
    if(!parsed.valid()) {error="OBJ has no triangles: "+path;return false;}
    result=std::move(parsed);error.clear();return true;
}
bool loadPpm(const std::string& path,ImageData& result,std::string& error) {
    std::ifstream in(path,std::ios::binary);
    if(!in) {error="Cannot open PPM: "+path;return false;}
    ImageData img;
    try {
        const std::string magic=nextToken(in);
        if(magic!="P3" && magic!="P6")throw std::runtime_error("Expected P3/P6 PPM");
        img.width=std::stoi(nextToken(in));img.height=std::stoi(nextToken(in));
        const int maxv=std::stoi(nextToken(in));
        if(img.width<=0||img.height<=0||img.width>8192||img.height>8192||maxv!=255)
            throw std::runtime_error("Invalid dimensions/maxval (must be 255)");
        img.rgb.resize(static_cast<std::size_t>(img.width)*static_cast<std::size_t>(img.height)*3);
        if(magic=="P3") {
            for(auto& value:img.rgb) {int n=std::stoi(nextToken(in));if(n<0||n>255)throw std::runtime_error("Invalid RGB value");value=static_cast<std::uint8_t>(n);}
        } else {
            const int whitespace=in.get();
            if(whitespace==EOF || !std::isspace(static_cast<unsigned char>(whitespace))) throw std::runtime_error("Invalid P6 delimiter");
            if(whitespace=='\r' && in.peek()=='\n') in.get();
            in.read(reinterpret_cast<char*>(img.rgb.data()),static_cast<std::streamsize>(img.rgb.size()));
            if(in.gcount()!=static_cast<std::streamsize>(img.rgb.size()))throw std::runtime_error("Truncated P6 image");
        }
    } catch(const std::exception& e) {error=std::string("Invalid PPM: ")+e.what();return false;}
    result=std::move(img);error.clear();return true;
}
} // namespace originxr
