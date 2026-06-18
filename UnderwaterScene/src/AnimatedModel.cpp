//
// AnimatedModel.cpp - implementacja GPU skinningu (NED, nurek Mixamo/glTF).
//
#include "AnimatedModel.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "gtc/matrix_transform.hpp"

#include <cstddef>
#include <cmath>
#include <iostream>
#include <algorithm>

// --- konwersje Assimp -> glm ------------------------------------------------
static glm::mat4 aiToGlm(const aiMatrix4x4& m)
{
    // aiMatrix4x4 jest row-major, glm::mat4 column-major -> transpozycja.
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);
}
static glm::vec3 aiToGlm(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
static glm::quat aiToGlm(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }

void SkinnedVertex::addBone(int id, float w)
{
    if (w <= 0.0f) return;
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        if (boneIDs[i] < 0) { boneIDs[i] = id; weights[i] = w; return; }
    // wszystkie 4 sloty zajete -> podmien najslabszy, jesli nowy mocniejszy
    int weakest = 0;
    for (int i = 1; i < MAX_BONE_INFLUENCE; ++i)
        if (weights[i] < weights[weakest]) weakest = i;
    if (w > weights[weakest]) { boneIDs[weakest] = id; weights[weakest] = w; }
}

// --- interpolacja kanalu -----------------------------------------------------
template <typename T>
static int keyIndex(const std::vector<std::pair<float, T>>& keys, float t)
{
    for (int i = 0; i + 1 < (int)keys.size(); ++i)
        if (t < keys[i + 1].first) return i;
    return std::max(0, (int)keys.size() - 2);
}

glm::mat4 BoneChannel::localAt(float t) const
{
    glm::vec3 pos(0.0f), scl(1.0f);
    glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);

    if (positions.size() == 1) pos = positions[0].second;
    else if (positions.size() > 1) {
        int i = keyIndex(positions, t);
        float t0 = positions[i].first, t1 = positions[i + 1].first;
        float a = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
        pos = glm::mix(positions[i].second, positions[i + 1].second, glm::clamp(a, 0.0f, 1.0f));
    }

    if (rotations.size() == 1) rot = rotations[0].second;
    else if (rotations.size() > 1) {
        int i = keyIndex(rotations, t);
        float t0 = rotations[i].first, t1 = rotations[i + 1].first;
        float a = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
        rot = glm::normalize(glm::slerp(rotations[i].second, rotations[i + 1].second, glm::clamp(a, 0.0f, 1.0f)));
    }

    if (scales.size() == 1) scl = scales[0].second;
    else if (scales.size() > 1) {
        int i = keyIndex(scales, t);
        float t0 = scales[i].first, t1 = scales[i + 1].first;
        float a = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
        scl = glm::mix(scales[i].second, scales[i + 1].second, glm::clamp(a, 0.0f, 1.0f));
    }

    return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scl);
}

// --- ladowanie ---------------------------------------------------------------
void AnimatedModel::readHierarchy(NodeData& dst, const aiNode* src)
{
    dst.name = src->mName.C_Str();
    dst.transform = aiToGlm(src->mTransformation);
    dst.children.resize(src->mNumChildren);
    for (unsigned i = 0; i < src->mNumChildren; ++i)
        readHierarchy(dst.children[i], src->mChildren[i]);
}

void AnimatedModel::readAnimation(const aiAnimation* anim)
{
    AnimClip clip;
    clip.name = anim->mName.C_Str();
    clip.duration = (float)anim->mDuration;
    clip.ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? (float)anim->mTicksPerSecond : 25.0f;

    for (unsigned c = 0; c < anim->mNumChannels; ++c)
    {
        const aiNodeAnim* ch = anim->mChannels[c];
        BoneChannel bc;
        bc.positions.reserve(ch->mNumPositionKeys);
        for (unsigned k = 0; k < ch->mNumPositionKeys; ++k)
            bc.positions.emplace_back((float)ch->mPositionKeys[k].mTime, aiToGlm(ch->mPositionKeys[k].mValue));
        bc.rotations.reserve(ch->mNumRotationKeys);
        for (unsigned k = 0; k < ch->mNumRotationKeys; ++k)
            bc.rotations.emplace_back((float)ch->mRotationKeys[k].mTime, aiToGlm(ch->mRotationKeys[k].mValue));
        bc.scales.reserve(ch->mNumScalingKeys);
        for (unsigned k = 0; k < ch->mNumScalingKeys; ++k)
            bc.scales.emplace_back((float)ch->mScalingKeys[k].mTime, aiToGlm(ch->mScalingKeys[k].mValue));
        clip.channels[ch->mNodeName.C_Str()] = std::move(bc);
    }
    clips.push_back(std::move(clip));
    clipNamesVec.push_back(clips.back().name);
}

bool AnimatedModel::load(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights | aiProcess_CalcTangentSpace);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cout << "ERROR::AnimatedModel::" << path << " -> " << importer.GetErrorString() << std::endl;
        return false;
    }

    globalInverse = glm::inverse(aiToGlm(scene->mRootNode->mTransformation));

    glm::vec3 mn( 1e30f), mx(-1e30f);

    for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi)
    {
        const aiMesh* mesh = scene->mMeshes[mi];
        std::vector<SkinnedVertex> verts(mesh->mNumVertices);
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            verts[v].pos    = aiToGlm(mesh->mVertices[v]);
            verts[v].normal = mesh->mNormals ? aiToGlm(mesh->mNormals[v]) : glm::vec3(0, 1, 0);
            if (mesh->mTextureCoords[0])
                verts[v].uv = glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            mn = glm::min(mn, verts[v].pos);
            mx = glm::max(mx, verts[v].pos);
        }

        // Kosci + wagi
        for (unsigned b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            std::string name = bone->mName.C_Str();
            auto it = boneInfoMap.find(name);
            int id;
            if (it == boneInfoMap.end()) {
                id = boneCount++;
                boneInfoMap[name] = { id, aiToGlm(bone->mOffsetMatrix) };
            } else id = it->second.id;
            for (unsigned w = 0; w < bone->mNumWeights; ++w)
                verts[bone->mWeights[w].mVertexId].addBone(id, bone->mWeights[w].mWeight);
        }

        std::vector<unsigned> indices;
        indices.reserve(mesh->mNumFaces * 3);
        for (unsigned f = 0; f < mesh->mNumFaces; ++f)
            for (unsigned j = 0; j < mesh->mFaces[f].mNumIndices; ++j)
                indices.push_back(mesh->mFaces[f].mIndices[j]);

        SkinnedMesh sm;
        sm.indexCount = (int)indices.size();

        // Material: baseColorFactor / metallic / roughness (glTF, brak tekstur w tym modelu)
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiColor4D base(0.8f, 0.8f, 0.8f, 1.0f);
            if (aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &base) == AI_SUCCESS ||
                aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &base) == AI_SUCCESS)
                sm.albedo = glm::vec3(base.r, base.g, base.b);
            float f;
            if (aiGetMaterialFloat(mat, AI_MATKEY_METALLIC_FACTOR, &f) == AI_SUCCESS)  sm.metallic = f;
            if (aiGetMaterialFloat(mat, AI_MATKEY_ROUGHNESS_FACTOR, &f) == AI_SUCCESS) sm.roughness = f;
        }

        glGenVertexArrays(1, &sm.vao);
        glBindVertexArray(sm.vao);
        glGenBuffers(1, &sm.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, sm.vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(SkinnedVertex), verts.data(), GL_STATIC_DRAW);
        glGenBuffers(1, &sm.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_INT, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, boneIDs));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, weights));
        glBindVertexArray(0);

        skinnedMeshes.push_back(sm);
    }

    readHierarchy(root, scene->mRootNode);
    for (unsigned a = 0; a < scene->mNumAnimations; ++a)
        readAnimation(scene->mAnimations[a]);

    aabbMin = mn; aabbMax = mx;
    loaded = true;
    std::cout << "[DIVER] Zaladowano " << path << ": " << skinnedMeshes.size() << " meshy, "
              << boneCount << " kosci, " << clips.size() << " animacji. AABB Y=["
              << mn.y << ", " << mx.y << "]" << std::endl;
    if (boneCount > MAX_BONES_UNIFORM)
        std::cout << "  UWAGA: " << boneCount << " kosci > limit " << MAX_BONES_UNIFORM
                  << " w shaderze!" << std::endl;
    return true;
}

// --- liczenie pozy -----------------------------------------------------------
void AnimatedModel::computeGlobal(const NodeData& node, const glm::mat4& parentGlobal,
                                  const AnimClip& clip, float timeTicks,
                                  std::vector<glm::mat4>& out) const
{
    auto it = clip.channels.find(node.name);
    glm::mat4 local = (it != clip.channels.end()) ? it->second.localAt(timeTicks) : node.transform;
    glm::mat4 global = parentGlobal * local;

    auto bit = boneInfoMap.find(node.name);
    if (bit != boneInfoMap.end())
        out[bit->second.id] = globalInverse * global * bit->second.offset;

    for (const auto& child : node.children)
        computeGlobal(child, global, clip, timeTicks, out);
}

void AnimatedModel::computePose(int clip, float timeSeconds, float animSpeed,
                                std::vector<glm::mat4>& out) const
{
    out.assign(std::max(1, boneCount), glm::mat4(1.0f));
    if (!loaded || clips.empty()) return;
    clip = glm::clamp(clip, 0, (int)clips.size() - 1);
    const AnimClip& c = clips[clip];

    float timeTicks = 0.0f;
    if (c.duration > 0.0f) {
        float ticks = timeSeconds * c.ticksPerSecond * animSpeed;
        timeTicks = std::fmod(ticks, c.duration);
        if (timeTicks < 0.0f) timeTicks += c.duration;
    }
    computeGlobal(root, glm::mat4(1.0f), c, timeTicks, out);
}
