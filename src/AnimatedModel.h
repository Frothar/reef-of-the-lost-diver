#pragma once
//
// AnimatedModel.h - animacja szkieletowa (GPU skinning) dla nurka z Mixamo/glTF.
//
// Laduje model glTF z koscmi (JOINTS_0/WEIGHTS_0) i animacjami, liczy poze w czasie
// (hierarchia wezlow + interpolacja klatek kluczowych) i zwraca tablice macierzy
// kosci do uniformu finalBones[] w shaderze skinned.vert.
//
// Celowo NIE uzywa aiProcess_PreTransformVertices (to skasowaloby szkielet).
//
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include "glew.h"
#include <string>
#include <vector>
#include <map>

struct aiNode;       // fwd (zeby naglowek nie ciagnal calego assimpa)
struct aiScene;
struct aiAnimation;

static const int MAX_BONE_INFLUENCE = 4;
static const int MAX_BONES_UNIFORM  = 100; // musi byc zgodne ze skinned.vert

// Pojedynczy wierzcholek skinowany (interleaved w VBO).
struct SkinnedVertex {
    glm::vec3 pos    = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec2 uv     = glm::vec2(0.0f);
    int   boneIDs[MAX_BONE_INFLUENCE] = { -1, -1, -1, -1 };
    float weights[MAX_BONE_INFLUENCE] = {  0.0f, 0.0f, 0.0f, 0.0f };
    void addBone(int id, float w);
};

struct SkinnedMesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    int    indexCount = 0;
    glm::vec3 albedo   = glm::vec3(0.8f);  // glTF baseColorFactor
    float     metallic = 0.0f;
    float     roughness = 0.7f;
};

struct BoneInfo { int id = 0; glm::mat4 offset = glm::mat4(1.0f); }; // inverse-bind

// Kanal animacji jednego wezla (klatki kluczowe T/R/S).
struct BoneChannel {
    std::vector<std::pair<float, glm::vec3>> positions; // (czas w tickach, wartosc)
    std::vector<std::pair<float, glm::quat>> rotations;
    std::vector<std::pair<float, glm::vec3>> scales;
    glm::mat4 localAt(float timeTicks) const;
};

struct AnimClip {
    std::string name;
    float duration       = 0.0f;  // w tickach
    float ticksPerSecond = 25.0f;
    std::map<std::string, BoneChannel> channels; // klucz = nazwa wezla
};

// Kopia hierarchii wezlow (nazwa + transform lokalny + dzieci).
struct NodeData {
    std::string name;
    glm::mat4   transform = glm::mat4(1.0f);
    std::vector<NodeData> children;
};

class AnimatedModel {
public:
    bool load(const std::string& path);

    // Liczy poze klipu w danym czasie (sekundy) -> macierze kosci (rozmiar = liczba kosci).
    // animSpeed skaluje tempo. Zwraca przez 'out' (uzupelnia/zmienia rozmiar).
    void computePose(int clip, float timeSeconds, float animSpeed, std::vector<glm::mat4>& out) const;

    // Zwalnia zasoby GPU wszystkich meshy (VAO/VBO/EBO). Wolane przy zamknieciu.
    void destroy();

    const std::vector<SkinnedMesh>& meshes() const { return skinnedMeshes; }
    int  numClips() const { return (int)clips.size(); }
    int  numBones() const { return (int)boneCount; }
    const std::vector<std::string>& clipNames() const { return clipNamesVec; }
    bool valid() const { return loaded; }

    glm::vec3 aabbMin = glm::vec3(0.0f); // bind-pose (do skalowania/sadzania)
    glm::vec3 aabbMax = glm::vec3(0.0f);

private:
    std::vector<SkinnedMesh> skinnedMeshes;
    std::map<std::string, BoneInfo> boneInfoMap;
    int boneCount = 0;
    NodeData root;
    glm::mat4 globalInverse = glm::mat4(1.0f);
    std::vector<AnimClip> clips;
    std::vector<std::string> clipNamesVec;
    bool loaded = false;

    void readHierarchy(NodeData& dst, const aiNode* src);
    void readAnimation(const aiAnimation* anim);
    void computeGlobal(const NodeData& node, const glm::mat4& parentGlobal,
                       const AnimClip& clip, float timeTicks,
                       std::vector<glm::mat4>& out) const;
};
