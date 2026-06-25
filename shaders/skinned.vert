#version 410 core
//
// skinned.vert - GPU skinning (animacja szkieletowa nurka, Mixamo/glTF).
//
// Kazdy wierzcholek ma do 4 kosci (boneIDs) z wagami. Macierz skinningu to suma
// wazona macierzy kosci finalBones[] (liczonych na CPU w AnimatedModel). Wynik
// oswietlamy wspolnym pbr.frag, wiec out-y musza byc jak w pbr.vert.

layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aNormal;
layout(location = 2) in vec2  aUV;
layout(location = 3) in ivec4 aBoneIDs;
layout(location = 4) in vec4  aWeights;

const int MAX_BONES = 100;
uniform mat4 finalBones[MAX_BONES];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    // Macierz skinningu = suma wazona macierzy kosci.
    mat4 skin = mat4(0.0);
    float wsum = 0.0;
    for (int i = 0; i < 4; ++i) {
        int id = aBoneIDs[i];
        if (id < 0) continue;
        skin += aWeights[i] * finalBones[id];
        wsum += aWeights[i];
    }
    if (wsum < 0.0001) skin = mat4(1.0);   // wierzcholek bez wag -> bez deformacji

    vec4 localPos = skin * vec4(aPos, 1.0);
    worldPos = vec3(model * localPos);

    mat3 nrmMat = mat3(transpose(inverse(model * skin)));
    worldNormal = normalize(nrmMat * aNormal);

    texCoord = aUV;
    TBN = mat3(1.0); // nurek nie uzywa normal mapy; pbr.frag i tak czyta TBN tylko gdy useNormalMap

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
