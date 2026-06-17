#pragma once
//
// scene_underwater.hpp - starter scene for "Reef of the Lost Diver".
//
// MRZ-01 baseline + MRZ-02 quaternion camera: a window with an underwater
// atmosphere, the gimbal-lock-free quaternion camera, a cubemap skybox and a
// couple of lit test objects. Proves the framework (Shader_Loader,
// Render_Utils, Texture, Camera, SOIL, Assimp, ImGui) is wired up correctly.
//
// Team: replace/extend the placeholders below with your own modules:
//   * Mroz    -> Light system B13 (MRZ-05), particles (MRZ-07)
//   * Nedzynski -> Spline + PTF (NED-01/02), fish swimming shader (NED-03), skybox polish
//   * Olejnik -> PBR + normal + shadow mapping shaders (OLE-01..06)
//
#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"   // ImGui_ImplGlfw_*Callback – muszą być chainowane
#include "backends/imgui_impl_opengl3.h"

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"
#include "Camera.h"               // Core::createPerspectiveMatrix
#include "QuaternionCamera.h"     // MRZ-02 quaternion camera
#include "ParticleSystem.h"       // MRZ-07 babelki
#include "Spline.h"               // NED-01 splajn Catmull-Rom
#include "FishAnimation.h"        // NED-04 ryby po splajnie z PTF

#include <vector>

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <fstream>

// ---------------------------------------------------------------------------
// Skybox cube geometry (positions only)
// ---------------------------------------------------------------------------
static const float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
};

// ---------------------------------------------------------------------------
// Shader programs / GPU resources
// ---------------------------------------------------------------------------
// OLE-02: zestaw map PBR (ambientCG naming: Prefix_Color, _Metalness, _Roughness, _AmbientOcclusion).
struct PBRTextureSet
{
    GLuint albedoMap    = 0;
    GLuint metallicMap  = 0;
    GLuint roughnessMap = 0;
    GLuint aoMap        = 0;
    GLuint normalMap    = 0;          // OLE-03: normal map w przestrzeni stycznej
    bool useAlbedoMap    = false;
    bool useMetallicMap  = false;
    bool useRoughnessMap = false;
    bool useAoMap        = false;
    bool useNormalMap    = false;     // OLE-03: flaga wlaczajaca normal mapping
};

struct PBRMaterial
{
    glm::vec3 albedo    = glm::vec3(0.8f);
    float     metallic  = 0.0f;
    float     roughness = 0.5f;
    PBRTextureSet tex;
    glm::vec2   uvScale = glm::vec2(1.0f);
};

// NED-03 (A10): parametry deformacji plywania, wysylane do fish.vert.
struct FishParams
{
    float waveAmplitude = 0.12f; // amplituda fali bocznej (lokalne jednostki)
    float waveFrequency = 6.0f;  // ile fal na dlugosc ciala
    float waveSpeed     = 4.0f;  // predkosc machania ogonem
    float fishLength    = 1.0f;  // dlugosc ryby w lokalnym +Z (fish.obj: glowa z=0, ogon z=1)
    float finAmplitude  = 0.08f; // amplituda ruchu pletw (piersiowe wystaja w X)
};

// NED-06: parametry pulsowania meduzy, wysylane do jellyfish.vert.
struct JellyParams
{
    float pulseAmplitude    = 0.18f; // jak mocno pulsuje dzwon
    float pulseSpeed        = 2.0f;  // czestotliwosc pulsu
    float tentacleAmplitude = 0.10f; // jak mocno koysza sie czulki
    float tentacleSpeed     = 1.6f;
    float tentacleLength    = 0.95f; // zgodne z generatorem modelu
    float bobAmplitude      = 0.25f; // pionowy ruch (puls = napped do gory)
};

namespace {
    Core::Shader_Loader shaderLoader;

    GLuint programPBR    = 0;
    GLuint programSkybox = 0;
    GLuint programDebugLine = 0; // NED-01 podglad splajnu
    GLuint programFish   = 0;    // NED-03 pływanie ryb (A10)
    GLuint programJelly  = 0;    // NED-06 pulsujace meduzy
    GLuint programWaterOverlay = 0; // pelnoekranowy efekt wody
    GLuint programShadowDepth  = 0; // OLE-04 przebieg cieni
    GLuint programPostprocess  = 0; // OLE-07 post-processing podwodny

    // --- OLE-07: Post-processing FBO (kolor + glebia) -------------------------
    GLuint sceneFBO = 0;
    GLuint sceneColorTex = 0;
    GLuint sceneDepthTex = 0;
    int    sceneFBOWidth = 0, sceneFBOHeight = 0;
    bool   usePostprocess = true;
    // Parametry post-processingu
    glm::vec3 ppTint         = glm::vec3(0.6f, 0.85f, 0.95f); // niebiesko-zielony tint
    float     ppTintStrength = 0.35f;
    float     ppDepthFogDensity = 0.08f;
    float     ppDepthFogStart   = 1.0f;
    glm::vec3 ppDepthFogColor   = glm::vec3(0.04f, 0.18f, 0.28f);
    float     ppChromaticStrength = 0.003f;
    float     ppVignetteStrength  = 0.4f;

    // --- OLE-04: Shadow mapping (metoda obowiazkowa M4) -----------------------
    const int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    GLuint shadowFBO = 0;              // framebuffer only-depth
    GLuint shadowDepthTex = 0;         // tekstura GL_DEPTH_COMPONENT
    float  shadowOrthoSize = 25.0f;    // polowa boku frustum ortho swiatla
    float  shadowNear = -30.0f;        // near/far planes ortho
    float  shadowFar  =  30.0f;
    bool   useShadows = true;
    // OLE-06: strojenie cieni
    float  shadowBiasMin = 0.001f;     // minimalny bias
    float  shadowBiasMax = 0.01f;      // maks bias przy ostrym kacie
    bool   usePCF5x5 = false;          // PCF 5x5 zamiast 3x3

    Core::RenderContext sphereContext;
    Core::RenderContext cubeContext;
    Core::RenderContext groundContext; // re-uses the cube model, scaled flat
    Core::RenderContext fishContext;   // NED-03/ALL-01 model ryby (models/fish.obj)
    Core::RenderContext jellyContext;  // NED-06 model meduzy (models/jellyfish.obj)

    GLuint skyboxVAO = 0, skyboxVBO = 0;
    GLuint skyboxCubemap = 0;

    // --- Splajn (NED-01) -----------------------------------------------------
    Spline debugSpline;                 // sciezka A (ryby + podglad)
    GLuint splineVAO = 0, splineVBO = 0;
    int    splineVertexCount = 0;
    bool   showSpline = true;

    // --- Druga sciezka dla ryb (NED-04) --------------------------------------
    Spline fishSplineB;                 // sciezka B (druga grupa ryb)
    GLuint splineBVAO = 0, splineBVBO = 0;
    int    splineBVertexCount = 0;

    // --- PTF debug (NED-02) --------------------------------------------------
    // Kolorowe osie (T=czerwona, N=zielona, B=niebieska) w co ktorej ramce PTF.
    GLuint ptfAxesVAO = 0, ptfAxesVBO = 0;
    int    ptfAxesVertexCount = 0;
    bool   showPTF = true;

    // --- Pelnoekranowy efekt wody --------------------------------------------
    GLuint waterQuadVAO = 0, waterQuadVBO = 0;
    bool   showWaterOverlay = true;
    float  waterOverlayStrength = 0.7f;

    // --- MRZ-07: babelki -----------------------------------------------------
    GLuint       programParticle = 0;
    BubbleSystem bubbles;
    bool         showBubbles = true;
    glm::vec3    bubbleColor = glm::vec3(0.75f, 0.88f, 0.95f);

    float aspectRatio = 1.0f;

    // --- Quaternion camera (MRZ-02) ------------------------------------------
    Camera camera = Camera(glm::vec3(0.0f, 1.0f, 6.0f));
    float mouseSensitivity = 0.1f;
    float rollSpeed = 60.0f; // stopnie/s dla Q/E
    bool  firstMouse = true;
    float lastX = 500.0f, lastY = 500.0f;
    bool  mouseLook = true;

    // --- Lighting / atmosphere (tweakable in the ImGui panel) ----------------
    glm::vec3 sunDir     = glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f)); // light from above
    glm::vec3 sunColor   = glm::vec3(0.85f, 0.95f, 1.0f);
    glm::vec3 waterColor = glm::vec3(0.05f, 0.22f, 0.30f);
    float     fogDensity = 0.035f;

    // OLE-01: tweakable PBR test materials (ImGui)
    PBRMaterial sphereMaterial = { glm::vec3(0.9f, 0.5f, 0.4f), 0.0f, 0.35f };
    PBRMaterial metalMaterial  = { glm::vec3(0.78f, 0.78f, 0.80f), 1.0f, 0.25f };
    PBRMaterial groundMaterial = { glm::vec3(0.55f, 0.48f, 0.35f), 0.0f, 0.95f };
    bool groundUseTextures = true;
    bool groundUseNormalMap = true;  // OLE-03: osobny toggle na normal mapy
    bool metalUseTextures  = true;
    bool metalUseNormalMap = true;   // OLE-03

    // --- OLE-05: Wiele swiatel -----------------------------------------------
    // Struktury zgodne z uniformami w pbr.frag
    struct PointLightCPU {
        glm::vec3 position  = glm::vec3(0.0f);
        glm::vec3 color     = glm::vec3(1.0f);
        float     intensity = 1.0f;
        float     constant  = 1.0f;
        float     linear    = 0.14f;
        float     quadratic = 0.07f;
    };
    struct SpotLightCPU {
        glm::vec3 position   = glm::vec3(0.0f);
        glm::vec3 direction  = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 color      = glm::vec3(1.0f);
        float     intensity  = 1.0f;
        float     constant   = 1.0f;
        float     linear     = 0.09f;
        float     quadratic  = 0.032f;
        float     innerCutoff = glm::cos(glm::radians(12.5f));
        float     outerCutoff = glm::cos(glm::radians(17.5f));
    };

    static const int MAX_POINT_LIGHTS = 8;
    static const int MAX_SPOT_LIGHTS  = 4;
    PointLightCPU pointLights[MAX_POINT_LIGHTS];
    int numPointLights = 0;
    SpotLightCPU spotLights[MAX_SPOT_LIGHTS];
    int numSpotLights = 0;

    // Bioluminescencja: 3 dryfujace swiatla punktowe
    bool showBioLights = true;
    glm::vec3 bioBasePos[3] = {
        glm::vec3(-3.0f, 1.5f, 2.0f),
        glm::vec3( 4.0f, 2.0f, -1.5f),
        glm::vec3( 0.0f, 0.5f, -4.0f),
    };
    glm::vec3 bioColors[3] = {
        glm::vec3(0.2f, 0.5f, 1.0f),    // niebieski
        glm::vec3(0.1f, 0.9f, 0.4f),    // zielony
        glm::vec3(0.6f, 0.2f, 0.9f),    // fioletowy
    };
    float bioIntensity = 3.0f;
    float bioPulseSpeed = 1.5f;

    // --- MRZ-05 (B13): latarka nurka -----------------------------------------
    // Reflektor (spotlight) przyczepiony do kamery: pozycja = pozycja kamery,
    // kierunek = front kamery. Wlaczana F, kolor cyklowany C, jasnosc +/- / scroll.
    bool  headlampOn = true;
    int   headlampColorIdx = 0;
    glm::vec3 headlampColors[3] = {
        glm::vec3(1.00f, 1.00f, 1.00f),   // bialy
        glm::vec3(1.00f, 0.82f, 0.55f),   // cieply (zolty)
        glm::vec3(0.55f, 0.78f, 1.00f),   // chlodny (niebieski)
    };
    const char* headlampColorNames[3] = { "bialy", "cieply", "chlodny" };
    float headlampIntensity = 9.0f;

    // NED-03 (A10): animacja ryb (domyslne dostrojone do modelu models/fish.obj)
    FishParams  fishParams;
    PBRMaterial fishMaterial = { glm::vec3(0.30f, 0.55f, 0.75f), 0.1f, 0.45f }; // srebrzysto-niebieska
    bool        showFish = true;

    // NED-04: instancje ryb jadacych po splajnach (+ kolory rownolegle)
    std::vector<FishAnimation> fishes;
    std::vector<glm::vec3>     fishColors;

    // --- MRZ-06: straszenie ryb (lewy klik) ----------------------------------
    bool      scareActive    = false;
    float     scareTimer     = 0.0f;
    float     scareDuration  = 3.0f;   // jak dlugo ryby uciekaja (s)
    float     scareRadius    = 9.0f;   // promien od miejsca strachu
    float     scareSpeedMult = 4.5f;   // ile razy szybciej ryby smigaja
    glm::vec3 scareOrigin    = glm::vec3(0.0f);

    // NED-06: meduzy (pulsowanie)
    JellyParams jellyParams;
    PBRMaterial jellyMaterial = { glm::vec3(0.55f, 0.45f, 0.85f), 0.0f, 0.30f }; // fioletowo-niebieska, gladka
    bool        showJelly = true;
}

// ---------------------------------------------------------------------------
// Camera / projection helpers
// ---------------------------------------------------------------------------
inline glm::mat4 createCameraMatrix()
{
    return camera.viewMatrix();
}

inline glm::mat4 createPerspectiveMatrix()
{
    return Core::createPerspectiveMatrix(0.05f, 200.0f, glm::min(aspectRatio, 1.0f)) *
           glm::scale(glm::vec3(glm::min(1.0f / aspectRatio, 1.0f), 1.0f, 1.0f));
}

// ---------------------------------------------------------------------------
// OLE-02: ladowanie i bindowanie map PBR
// ---------------------------------------------------------------------------
inline bool fileExists(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

inline void loadPBRTextureSet(const std::string& dir, const std::string& prefix, PBRTextureSet& tex)
{
    auto tryLoad = [&](const char* suffix, GLuint& id, bool& useFlag) {
        std::string path = dir + "/" + prefix + suffix;
        if (!fileExists(path)) return;
        id = Core::LoadTexture(path.c_str());
        useFlag = true;
        std::cout << "Loaded PBR map: " << path << std::endl;
    };

    tryLoad("_Color.jpg",              tex.albedoMap,    tex.useAlbedoMap);
    tryLoad("_Metalness.jpg",          tex.metallicMap,  tex.useMetallicMap);
    tryLoad("_Roughness.jpg",          tex.roughnessMap, tex.useRoughnessMap);
    tryLoad("_AmbientOcclusion.jpg",   tex.aoMap,        tex.useAoMap);
    tryLoad("_NormalGL.jpg",           tex.normalMap,    tex.useNormalMap);  // OLE-03
}

inline void setTextureFlags(PBRTextureSet& tex, bool enabled)
{
    if (tex.albedoMap)    tex.useAlbedoMap    = enabled;
    if (tex.metallicMap)  tex.useMetallicMap  = enabled;
    if (tex.roughnessMap) tex.useRoughnessMap = enabled;
    if (tex.aoMap)        tex.useAoMap        = enabled;
    if (tex.normalMap)    tex.useNormalMap    = enabled;  // OLE-03
}

// Wspolne uniformy + mapy dla pbr.frag (uzywane tez przez fish.vert + pbr.frag).
inline void bindPBRMaterial(GLuint program, const PBRMaterial& material)
{
    const PBRTextureSet& t = material.tex;

    glUniform3fv(glGetUniformLocation(program, "albedo"), 1, (float*)&material.albedo);
    glUniform1f(glGetUniformLocation(program, "metallic"),  material.metallic);
    glUniform1f(glGetUniformLocation(program, "roughness"), material.roughness);
    glUniform2fv(glGetUniformLocation(program, "uvScale"), 1, (float*)&material.uvScale);

    glUniform1i(glGetUniformLocation(program, "useAlbedoMap"),    t.useAlbedoMap ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "useMetallicMap"),  t.useMetallicMap ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "useRoughnessMap"), t.useRoughnessMap ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "useAoMap"),        t.useAoMap ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "useNormalMap"),    t.useNormalMap ? 1 : 0);  // OLE-03

    if (t.useAlbedoMap)
        Core::SetActiveTexture(t.albedoMap, "albedoMap", program, 0);
    if (t.useMetallicMap)
        Core::SetActiveTexture(t.metallicMap, "metallicMap", program, 1);
    if (t.useRoughnessMap)
        Core::SetActiveTexture(t.roughnessMap, "roughnessMap", program, 2);
    if (t.useAoMap)
        Core::SetActiveTexture(t.aoMap, "aoMap", program, 3);
    if (t.useNormalMap)                                                                      // OLE-03
        Core::SetActiveTexture(t.normalMap, "normalMap", program, 4);                        // OLE-03
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------
// OLE-04: wysyla uniformy cieni do aktywnego programu (lightSpaceMatrix, shadowMap, useShadows).
inline void bindShadowUniforms(GLuint program, const glm::mat4& lightSpaceMat)
{
    glUniformMatrix4fv(glGetUniformLocation(program, "lightSpaceMatrix"), 1, GL_FALSE, (float*)&lightSpaceMat);
    glUniform1i(glGetUniformLocation(program, "useShadows"), useShadows ? 1 : 0);
    glUniform1f(glGetUniformLocation(program, "shadowBiasMin"), shadowBiasMin);   // OLE-06
    glUniform1f(glGetUniformLocation(program, "shadowBiasMax"), shadowBiasMax);   // OLE-06
    glUniform1i(glGetUniformLocation(program, "usePCF5x5"), usePCF5x5 ? 1 : 0);  // OLE-06
    // Shadow map na texture unit 5 (0-4 zajete przez PBR mapy)
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glUniform1i(glGetUniformLocation(program, "shadowMap"), 5);
}

// OLE-05: wysyla tablice swiatel punktowych i reflektorow do aktywnego programu.
inline void bindLightUniforms(GLuint program)
{
    glUniform1i(glGetUniformLocation(program, "numPointLights"), numPointLights);
    for (int i = 0; i < numPointLights; ++i)
    {
        std::string base = "pointLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, (base + ".position").c_str()),  1, (float*)&pointLights[i].position);
        glUniform3fv(glGetUniformLocation(program, (base + ".color").c_str()),     1, (float*)&pointLights[i].color);
        glUniform1f(glGetUniformLocation(program,  (base + ".intensity").c_str()),  pointLights[i].intensity);
        glUniform1f(glGetUniformLocation(program,  (base + ".constant").c_str()),   pointLights[i].constant);
        glUniform1f(glGetUniformLocation(program,  (base + ".linear").c_str()),     pointLights[i].linear);
        glUniform1f(glGetUniformLocation(program,  (base + ".quadratic").c_str()),  pointLights[i].quadratic);
    }

    glUniform1i(glGetUniformLocation(program, "numSpotLights"), numSpotLights);
    for (int i = 0; i < numSpotLights; ++i)
    {
        std::string base = "spotLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, (base + ".position").c_str()),   1, (float*)&spotLights[i].position);
        glUniform3fv(glGetUniformLocation(program, (base + ".direction").c_str()),  1, (float*)&spotLights[i].direction);
        glUniform3fv(glGetUniformLocation(program, (base + ".color").c_str()),      1, (float*)&spotLights[i].color);
        glUniform1f(glGetUniformLocation(program,  (base + ".intensity").c_str()),   spotLights[i].intensity);
        glUniform1f(glGetUniformLocation(program,  (base + ".constant").c_str()),    spotLights[i].constant);
        glUniform1f(glGetUniformLocation(program,  (base + ".linear").c_str()),      spotLights[i].linear);
        glUniform1f(glGetUniformLocation(program,  (base + ".quadratic").c_str()),   spotLights[i].quadratic);
        glUniform1f(glGetUniformLocation(program,  (base + ".innerCutoff").c_str()), spotLights[i].innerCutoff);
        glUniform1f(glGetUniformLocation(program,  (base + ".outerCutoff").c_str()), spotLights[i].outerCutoff);
    }
}

// OLE-04: oblicza macierz lightSpace (ortho * view) dla swiatla kierunkowego.
inline glm::mat4 computeLightSpaceMatrix()
{
    // View z perspektywy swiatla: patrzy w kierunku -sunDir z centrum sceny
    glm::vec3 lightPos = sunDir * 15.0f; // odsuniecie od centrum
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(
        -shadowOrthoSize, shadowOrthoSize,
        -shadowOrthoSize, shadowOrthoSize,
        shadowNear, shadowFar);
    return lightProjection * lightView;
}

inline void drawPBRObject(Core::RenderContext& context, glm::mat4 modelMatrix,
                          const PBRMaterial& material, const glm::mat4& lightSpaceMat)
{
    glUseProgram(programPBR);

    glm::mat4 view = createCameraMatrix();
    glm::mat4 projection = createPerspectiveMatrix();

    glUniformMatrix4fv(glGetUniformLocation(programPBR, "model"),      1, GL_FALSE, (float*)&modelMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programPBR, "view"),       1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(programPBR, "projection"), 1, GL_FALSE, (float*)&projection);

    glUniform3fv(glGetUniformLocation(programPBR, "cameraPos"),  1, (float*)&camera.position);
    glUniform3fv(glGetUniformLocation(programPBR, "lightDir"),   1, (float*)&sunDir);
    glUniform3fv(glGetUniformLocation(programPBR, "lightColor"), 1, (float*)&sunColor);

    bindPBRMaterial(programPBR, material);
    bindShadowUniforms(programPBR, lightSpaceMat);
    bindLightUniforms(programPBR);  // OLE-05

    glUniform3fv(glGetUniformLocation(programPBR, "fogColor"),  1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programPBR, "fogDensity"), fogDensity);

    Core::DrawContext(context);
    glUseProgram(0);
}

// NED-03 (A10): rysuje rybe z deformacja plywania w fish.vert.
// phaseOffset pozwala desynchronizowac kilka ryb (rozne fazy machania ogonem).
inline void drawFish(Core::RenderContext& context, glm::mat4 modelMatrix,
                     const PBRMaterial& material, float time, float phaseOffset,
                     const glm::mat4& lightSpaceMat)
{
    glUseProgram(programFish);

    glm::mat4 view = createCameraMatrix();
    glm::mat4 projection = createPerspectiveMatrix();

    glUniformMatrix4fv(glGetUniformLocation(programFish, "model"),      1, GL_FALSE, (float*)&modelMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programFish, "view"),       1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(programFish, "projection"), 1, GL_FALSE, (float*)&projection);

    glUniform3fv(glGetUniformLocation(programFish, "cameraPos"),  1, (float*)&camera.position);
    glUniform3fv(glGetUniformLocation(programFish, "lightDir"),   1, (float*)&sunDir);
    glUniform3fv(glGetUniformLocation(programFish, "lightColor"), 1, (float*)&sunColor);

    bindPBRMaterial(programFish, material);
    bindShadowUniforms(programFish, lightSpaceMat);
    bindLightUniforms(programFish);  // OLE-05

    glUniform3fv(glGetUniformLocation(programFish, "fogColor"),  1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programFish, "fogDensity"), fogDensity);

    // Parametry animacji A10 (phaseOffset wchodzi w czas, wiec rozne ryby macha inaczej)
    glUniform1f(glGetUniformLocation(programFish, "time"),          time + phaseOffset);
    glUniform1f(glGetUniformLocation(programFish, "waveAmplitude"), fishParams.waveAmplitude);
    glUniform1f(glGetUniformLocation(programFish, "waveFrequency"), fishParams.waveFrequency);
    glUniform1f(glGetUniformLocation(programFish, "waveSpeed"),     fishParams.waveSpeed);
    glUniform1f(glGetUniformLocation(programFish, "fishLength"),    fishParams.fishLength);
    glUniform1f(glGetUniformLocation(programFish, "finAmplitude"),  fishParams.finAmplitude);

    Core::DrawContext(context);
    glUseProgram(0);
}

// NED-06: rysuje pulsujaca meduze (deformacja w jellyfish.vert).
// phaseOffset desynchronizuje puls miedzy meduzami.
inline void drawJellyfish(Core::RenderContext& context, glm::mat4 modelMatrix,
                          const PBRMaterial& material, float time, float phaseOffset,
                          const glm::mat4& lightSpaceMat)
{
    glUseProgram(programJelly);

    glm::mat4 view = createCameraMatrix();
    glm::mat4 projection = createPerspectiveMatrix();

    glUniformMatrix4fv(glGetUniformLocation(programJelly, "model"),      1, GL_FALSE, (float*)&modelMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programJelly, "view"),       1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(programJelly, "projection"), 1, GL_FALSE, (float*)&projection);

    glUniform3fv(glGetUniformLocation(programJelly, "cameraPos"),  1, (float*)&camera.position);
    glUniform3fv(glGetUniformLocation(programJelly, "lightDir"),   1, (float*)&sunDir);
    glUniform3fv(glGetUniformLocation(programJelly, "lightColor"), 1, (float*)&sunColor);

    bindPBRMaterial(programJelly, material);
    bindShadowUniforms(programJelly, lightSpaceMat);
    bindLightUniforms(programJelly);  // OLE-05

    glUniform3fv(glGetUniformLocation(programJelly, "fogColor"),  1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programJelly, "fogDensity"), fogDensity);

    glUniform1f(glGetUniformLocation(programJelly, "time"),              time + phaseOffset);
    glUniform1f(glGetUniformLocation(programJelly, "pulseAmplitude"),    jellyParams.pulseAmplitude);
    glUniform1f(glGetUniformLocation(programJelly, "pulseSpeed"),        jellyParams.pulseSpeed);
    glUniform1f(glGetUniformLocation(programJelly, "tentacleAmplitude"), jellyParams.tentacleAmplitude);
    glUniform1f(glGetUniformLocation(programJelly, "tentacleSpeed"),     jellyParams.tentacleSpeed);
    glUniform1f(glGetUniformLocation(programJelly, "tentacleLength"),    jellyParams.tentacleLength);

    Core::DrawContext(context);
    glUseProgram(0);
}

// Podglad splajnu jako linia (NED-01) - sluzy do wizualnej kontroli gladkosci.
inline void drawSpline()
{
    if (!showSpline) return;

    glUseProgram(programDebugLine);
    glm::mat4 viewProjection = createPerspectiveMatrix() * createCameraMatrix();
    glUniformMatrix4fv(glGetUniformLocation(programDebugLine, "viewProjection"), 1, GL_FALSE, (float*)&viewProjection);

    // Sciezka A - zolta
    if (splineVertexCount >= 2)
    {
        glm::vec3 colorA = glm::vec3(1.0f, 0.85f, 0.2f);
        glUniform3fv(glGetUniformLocation(programDebugLine, "lineColor"), 1, (float*)&colorA);
        glBindVertexArray(splineVAO);
        glDrawArrays(GL_LINE_STRIP, 0, splineVertexCount);
    }
    // Sciezka B (NED-04) - turkusowa
    if (splineBVertexCount >= 2)
    {
        glm::vec3 colorB = glm::vec3(0.2f, 0.9f, 0.85f);
        glUniform3fv(glGetUniformLocation(programDebugLine, "lineColor"), 1, (float*)&colorB);
        glBindVertexArray(splineBVAO);
        glDrawArrays(GL_LINE_STRIP, 0, splineBVertexCount);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

// Osie PTF (NED-02): T=czerwona, N=zielona, B=niebieska, rysowane jako GL_LINES.
// Kolor jest zakodowany w wierzcholku (dodatkowy atrybut loc=1).
// Shader debug_line uzywa uniformu lineColor - tutaj rysujemy kazda os osobno z wlasnym kolorem.
inline void drawPTFAxes()
{
    if (!showPTF || ptfAxesVertexCount < 2) return;

    glUseProgram(programDebugLine);
    glm::mat4 vp = createPerspectiveMatrix() * createCameraMatrix();
    glUniformMatrix4fv(glGetUniformLocation(programDebugLine, "viewProjection"), 1, GL_FALSE, (float*)&vp);

    glBindVertexArray(ptfAxesVAO);

    // Osie sa upakowane: [poczatek T, koniec T,  poczatek N, koniec N,  poczatek B, koniec B] powtarza sie co ramke.
    // Rysujemy je trojkami odcinkow (6 wierzcholkow na ramke), kazda os innym kolorem.
    int framesStored = ptfAxesVertexCount / 6; // 6 wierzcholkow na ramke (3 osie x 2 punkty)
    glm::vec3 colT(1.0f, 0.2f, 0.2f); // czerwona - tangent
    glm::vec3 colN(0.2f, 1.0f, 0.3f); // zielona  - normalna
    glm::vec3 colB(0.3f, 0.5f, 1.0f); // niebieska - binormalna

    for (int i = 0; i < framesStored; ++i)
    {
        int base = i * 6;
        glUniform3fv(glGetUniformLocation(programDebugLine, "lineColor"), 1, (float*)&colT);
        glDrawArrays(GL_LINES, base + 0, 2);
        glUniform3fv(glGetUniformLocation(programDebugLine, "lineColor"), 1, (float*)&colN);
        glDrawArrays(GL_LINES, base + 2, 2);
        glUniform3fv(glGetUniformLocation(programDebugLine, "lineColor"), 1, (float*)&colB);
        glDrawArrays(GL_LINES, base + 4, 2);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

// Skybox podwodny (NED-05, metoda obowiazkowa M6).
// glDepthMask(GL_FALSE) + GL_LEQUAL + trick pos.xyww w shaderze:
//   skybox zawsze za geometria, nie nadpisuje bufora glebokosci uzyteczna wartoscia.
inline void drawSkybox(float time)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE); // wylaczony zapis glebi (wymaganie NED-05)
    glUseProgram(programSkybox);

    // Uciecie translacji z macierzy widoku - skybox obraca sie z kamera, ale nie jedzie za nia.
    glm::mat4 view = glm::mat4(glm::mat3(createCameraMatrix()));
    glm::mat4 viewProjection = createPerspectiveMatrix() * view;
    glUniformMatrix4fv(glGetUniformLocation(programSkybox, "viewProjection"), 1, GL_FALSE, (float*)&viewProjection);
    glUniform3fv(glGetUniformLocation(programSkybox, "waterTint"), 1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programSkybox, "time"), time); // animowane kaustyki

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glUniform1i(glGetUniformLocation(programSkybox, "skybox"), 0);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glUseProgram(0);
    glDepthMask(GL_TRUE);  // przywroc zapis glebi dla reszty sceny
    glDepthFunc(GL_LESS);
}

// Pelnoekranowy efekt wody - rysowany jako OSTATNI, na wierzchu calej sceny.
// Alpha blending + wylaczony test/zapis glebi (overlay nie nalezy do geometrii 3D).
inline void drawWaterOverlay(float time)
{
    if (!showWaterOverlay || waterOverlayStrength <= 0.0f) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(programWaterOverlay);
    glUniform3fv(glGetUniformLocation(programWaterOverlay, "waterColor"), 1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programWaterOverlay, "time"), time);
    glUniform1f(glGetUniformLocation(programWaterOverlay, "overlayStrength"), waterOverlayStrength);

    glBindVertexArray(waterQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// MRZ-07: rysuje babelki (point sprites) z blendingiem, bez zapisu glebi.
inline void drawBubbles(GLFWwindow* window, float time)
{
    if (!showBubbles) return;

    static float lastTime = time;
    float dt = time - lastTime;
    lastTime = time;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;
    bubbles.update(dt);

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // babelki nie zapisuja glebi (nie zaslaniaja sie twardo)

    glUseProgram(programParticle);
    glm::mat4 view = createCameraMatrix();
    glm::mat4 proj = createPerspectiveMatrix();
    glUniformMatrix4fv(glGetUniformLocation(programParticle, "view"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(programParticle, "projection"), 1, GL_FALSE, (float*)&proj);
    glUniform1f(glGetUniformLocation(programParticle, "sizeScale"), (float)fbH * 0.02f);
    glUniform3fv(glGetUniformLocation(programParticle, "bubbleColor"), 1, (float*)&bubbleColor);

    bubbles.draw();

    glUseProgram(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

// OLE-04: rysuje jeden obiekt do shadow depth FBO (tylko pozycja + model).
inline void drawShadowDepth(Core::RenderContext& context, const glm::mat4& modelMatrix,
                            const glm::mat4& lightSpaceMat)
{
    glUniformMatrix4fv(glGetUniformLocation(programShadowDepth, "model"), 1, GL_FALSE, (float*)&modelMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programShadowDepth, "lightSpaceMatrix"), 1, GL_FALSE, (float*)&lightSpaceMat);
    Core::DrawContext(context);
}

inline void renderScene(GLFWwindow* window)
{
    float time = (float)glfwGetTime();

    // --- OLE-05: aktualizacja swiatel punktowych (bioluminescencja) -----------
    numPointLights = 0;
    numSpotLights  = 0;
    if (showBioLights)
    {
        for (int i = 0; i < 3 && numPointLights < MAX_POINT_LIGHTS; ++i)
        {
            PointLightCPU& pl = pointLights[numPointLights];
            // Dryfujaca pozycja (wolny sin/cos z roznymi fazami)
            float phase = time * 0.3f + i * 2.1f;
            pl.position = bioBasePos[i] + glm::vec3(
                std::sin(phase) * 1.5f,
                std::sin(phase * 0.7f + 1.0f) * 0.8f,
                std::cos(phase * 0.5f) * 1.2f
            );
            pl.color     = bioColors[i];
            // Pulsujaca jasnosc (bioluminescencja miga)
            float pulse  = 0.5f + 0.5f * std::sin(time * bioPulseSpeed + i * 1.3f);
            pl.intensity = bioIntensity * pulse;
            pl.constant  = 1.0f;
            pl.linear    = 0.22f;
            pl.quadratic = 0.20f;
            ++numPointLights;
        }
    }

    // --- MRZ-05 (B13): latarka nurka - spotlight jadacy z kamera --------------
    if (headlampOn && numSpotLights < MAX_SPOT_LIGHTS)
    {
        SpotLightCPU& sl = spotLights[numSpotLights];
        sl.position    = camera.position;
        sl.direction   = camera.front();
        sl.color       = headlampColors[headlampColorIdx];
        sl.intensity   = headlampIntensity;
        sl.constant    = 1.0f;
        sl.linear      = 0.09f;
        sl.quadratic   = 0.032f;
        sl.innerCutoff = glm::cos(glm::radians(15.0f));
        sl.outerCutoff = glm::cos(glm::radians(23.0f));
        ++numSpotLights;
    }

    // Precompute model matrices (uzywane zarowno w shadow pass jak i main pass)
    glm::mat4 groundModel = glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)) * glm::scale(glm::vec3(40.0f, 0.2f, 40.0f));
    glm::mat4 sphereModel = glm::translate(glm::vec3(-1.5f, 1.0f + 0.15f * std::sin(time), 0.0f)) * glm::scale(glm::vec3(1.0f));
    const float metalCubeScale = 0.1f;
    glm::mat4 cubeModel = glm::translate(glm::vec3(2.0f, 1.0f + 0.1f * std::sin(time * 0.7f), 0.0f)) *
        glm::rotate((float)time * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(metalCubeScale));

    // OLE-04: macierz lightSpace (ortho + view z perspektywy swiatla)
    glm::mat4 lightSpaceMat = computeLightSpaceMatrix();

    // ========================================================================
    // OLE-04: PRZEBIEG CIENI - renderowanie geometrii do FBO glebi
    // ========================================================================
    if (useShadows)
    {
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Opcjonalnie: front-face culling w przebiegu cieni (zmniejsza peter-panning)
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        glUseProgram(programShadowDepth);

        // Rysujemy cala geometrie sceny (te same obiekty co w main pass)
        drawShadowDepth(groundContext, groundModel, lightSpaceMat);
        drawShadowDepth(sphereContext, sphereModel, lightSpaceMat);
        drawShadowDepth(cubeContext, cubeModel, lightSpaceMat);

        // Ryby i meduzy tez rzucaja cienie (uzywa shadow_depth.vert z ich pozycjami)
        if (showFish)
        {
            Core::RenderContext& fishMesh = (fishContext.vertexArray != 0) ? fishContext : sphereContext;
            for (size_t i = 0; i < fishes.size(); ++i)
                drawShadowDepth(fishMesh, fishes[i].modelMatrix(), lightSpaceMat);
        }
        if (showJelly && jellyContext.vertexArray != 0)
        {
            glm::vec3 jellyBase[3] = {
                glm::vec3(-4.5f, 0.5f, -2.0f), glm::vec3(2.5f, 1.0f, 3.5f), glm::vec3(5.0f, 0.0f, -3.5f)
            };
            float jellyPhase[3] = { 0.0f, 2.1f, 4.0f };
            float jellyScale[3] = { 1.6f, 1.2f, 2.0f };
            const float seabedTopY = 0.0f, margin = 0.15f;
            for (int i = 0; i < 3; ++i)
            {
                float ph = time * jellyParams.pulseSpeed + jellyPhase[i];
                float bob = jellyParams.bobAmplitude * std::sin(ph);
                float tentacleReach = jellyParams.tentacleLength * jellyScale[i];
                float minCenterY = seabedTopY + margin + tentacleReach + jellyParams.bobAmplitude;
                float centerY = glm::max(jellyBase[i].y, minCenterY) + bob;
                glm::mat4 m = glm::translate(glm::vec3(jellyBase[i].x, centerY, jellyBase[i].z))
                            * glm::scale(glm::vec3(jellyScale[i]));
                drawShadowDepth(jellyContext, m, lightSpaceMat);
            }
        }

        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK); // przywroc domyslne

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Przywroc viewport do rozmiaru okna
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
    }

    // ========================================================================
    // PRZEBIEG GLOWNY - renderowanie sceny do FBO posredniego (OLE-07)
    // ========================================================================
    if (usePostprocess && sceneFBO != 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    }

    // ========================================================================
    // PRZEBIEG GLOWNY - renderowanie sceny z PBR + cieniami
    // ========================================================================
    glClearColor(waterColor.r * 0.6f, waterColor.g * 0.7f, waterColor.b * 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sandy seabed (flat scaled cube, matte dielectric)
    drawPBRObject(groundContext, groundModel, groundMaterial, lightSpaceMat);

    // Dielectric test sphere
    drawPBRObject(sphereContext, sphereModel, sphereMaterial, lightSpaceMat);

    // Metallic test cube
    drawPBRObject(cubeContext, cubeModel, metalMaterial, lightSpaceMat);

    // --- Ryby (NED-03, A10) ---
    if (showFish)
    {
        Core::RenderContext& fishMesh = (fishContext.vertexArray != 0) ? fishContext : sphereContext;

        static float fishLastTime = time;
        float dt = time - fishLastTime;
        fishLastTime = time;
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.1f) dt = 0.1f;

        // MRZ-06: odliczanie strachu
        if (scareActive) { scareTimer -= dt; if (scareTimer <= 0.0f) scareActive = false; }
        float baseWave = fishParams.waveSpeed;

        for (size_t i = 0; i < fishes.size(); ++i)
        {
            // Ryby w promieniu od miejsca klikniecia smigaja szybciej i machaja ogonem jak szalone.
            bool scared = scareActive && glm::distance(fishes[i].position(), scareOrigin) < scareRadius;
            fishes[i].update(dt * (scared ? scareSpeedMult : 1.0f));

            PBRMaterial mat = fishMaterial;
            mat.albedo = (i < fishColors.size()) ? fishColors[i] : fishMaterial.albedo;

            fishParams.waveSpeed = scared ? baseWave * 2.5f : baseWave; // frenzy ogona (per-ryba)
            drawFish(fishMesh, fishes[i].modelMatrix(), mat, time, fishes[i].swimPhase(), lightSpaceMat);
        }
        fishParams.waveSpeed = baseWave; // przywroc (panel ImGui czyta oryginal)
    }

    // --- Meduzy (NED-06) ---
    if (showJelly && jellyContext.vertexArray != 0)
    {
        glm::vec3 jellyBase[3] = {
            glm::vec3(-4.5f, 0.5f, -2.0f),
            glm::vec3( 2.5f, 1.0f,  3.5f),
            glm::vec3( 5.0f, 0.0f, -3.5f),
        };
        float jellyPhase[3] = { 0.0f, 2.1f, 4.0f };
        float jellyScale[3] = { 1.6f, 1.2f, 2.0f };
        const float seabedTopY = 0.0f;
        const float margin     = 0.15f;

        for (int i = 0; i < 3; ++i)
        {
            float ph  = time * jellyParams.pulseSpeed + jellyPhase[i];
            float bob = jellyParams.bobAmplitude * std::sin(ph);

            float tentacleReach = jellyParams.tentacleLength * jellyScale[i];
            float minCenterY = seabedTopY + margin + tentacleReach + jellyParams.bobAmplitude;
            float centerY = glm::max(jellyBase[i].y, minCenterY) + bob;

            glm::mat4 m = glm::translate(glm::vec3(jellyBase[i].x, centerY, jellyBase[i].z))
                        * glm::scale(glm::vec3(jellyScale[i]));
            drawJellyfish(jellyContext, m, jellyMaterial, time, jellyPhase[i], lightSpaceMat);
        }
    }

    // Splajn (NED-01) - podglad sciezki dla ryb
    drawSpline();

    // Osie PTF (NED-02) - weryfikacja stabilnosci ramek
    drawPTFAxes();

    // Skybox last (depth trick keeps it behind everything)
    drawSkybox(time);

    // MRZ-07: babelki (po skyboxie, przed overlayem wody)
    drawBubbles(window, time);

    // Pelnoekranowy efekt wody na samym wierzchu (po skyboxie i geometrii)
    drawWaterOverlay(time);

    // ========================================================================
    // OLE-07: PRZEBIEG POST-PROCESSINGU - efekty podwodne
    // ========================================================================
    if (usePostprocess && sceneFBO != 0)
    {
        // Przywroc domyslny framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(programPostprocess);

        // Tekstury sceny
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColorTex);
        glUniform1i(glGetUniformLocation(programPostprocess, "screenTexture"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneDepthTex);
        glUniform1i(glGetUniformLocation(programPostprocess, "depthTexture"), 1);

        // Parametry
        glUniform3fv(glGetUniformLocation(programPostprocess, "underwaterTint"), 1, (float*)&ppTint);
        glUniform1f(glGetUniformLocation(programPostprocess, "tintStrength"), ppTintStrength);
        glUniform1f(glGetUniformLocation(programPostprocess, "depthFogDensity"), ppDepthFogDensity);
        glUniform1f(glGetUniformLocation(programPostprocess, "depthFogStart"), ppDepthFogStart);
        glUniform3fv(glGetUniformLocation(programPostprocess, "depthFogColor"), 1, (float*)&ppDepthFogColor);
        glUniform1f(glGetUniformLocation(programPostprocess, "nearPlane"), 0.05f);
        glUniform1f(glGetUniformLocation(programPostprocess, "farPlane"), 200.0f);
        glUniform1f(glGetUniformLocation(programPostprocess, "chromaticStrength"), ppChromaticStrength);
        glUniform1f(glGetUniformLocation(programPostprocess, "vignetteStrength"), ppVignetteStrength);
        glUniform1f(glGetUniformLocation(programPostprocess, "time"), time);

        // Rysuj fullscreen quad (uzywa tego samego VAO co water overlay)
        glBindVertexArray(waterQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glUseProgram(0);
        glEnable(GL_DEPTH_TEST);
    }
}

// ---------------------------------------------------------------------------
// Model loading
// ---------------------------------------------------------------------------
inline bool loadModelToContext(const std::string& path, Core::RenderContext& context)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << path << " -> " << import.GetErrorString() << std::endl;
        return false;
    }
    context.initFromAssimpMesh(scene->mMeshes[0]);
    std::cout << "Loaded model: " << path << " (" << context.size / 3 << " tris)" << std::endl;
    return true;
}

// ---------------------------------------------------------------------------
// OLE-07: tworzenie/resize FBO sceny (kolor + glebia)
// ---------------------------------------------------------------------------
inline void createOrResizeSceneFBO(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    if (width == sceneFBOWidth && height == sceneFBOHeight && sceneFBO != 0) return;

    // Usun stare zasoby
    if (sceneFBO != 0)
    {
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteTextures(1, &sceneColorTex);
        glDeleteTextures(1, &sceneDepthTex);
    }

    sceneFBOWidth = width;
    sceneFBOHeight = height;

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    // Tekstura koloru (RGB16F dla HDR)
    glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex, 0);

    // Tekstura glebi
    glGenTextures(1, &sceneDepthTex);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::OLE-07: Scene FBO nie jest kompletny!" << std::endl;
    else
        std::cout << "[OLE-07] Scene FBO " << width << "x" << height << " OK" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ---------------------------------------------------------------------------
// Input callbacks
// ---------------------------------------------------------------------------
inline void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    aspectRatio = height > 0 ? width / (float)height : 1.0f;
    glViewport(0, 0, width, height);
    // OLE-07: resize FBO sceny
    createOrResizeSceneFBO(width, height);
}

inline void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // init() nadpisuje callbacki GLFW – bez tego ImGui nie dostaje pozycji myszy i suwaki nie działają.
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    // Gdy kursor nad panelem ImGui, nie obracaj kamery (WantCaptureMouse = poprzednia klatka).
    if (!mouseLook || ImGui::GetIO().WantCaptureMouse)
    {
        firstMouse = true;
        return;
    }

    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }

    float dx = ((float)xpos - lastX) * mouseSensitivity; // prawo dodatnie
    float dy = (lastY - (float)ypos) * mouseSensitivity; // gora dodatnia
    lastX = (float)xpos; lastY = (float)ypos;

    camera.addYawPitch(-dx, dy);
}

inline void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    // Caps Lock: ręczne przełączenie trybu rozglądania (opcjonalne – panel działa też bez tego).
    if (key == GLFW_KEY_CAPS_LOCK && action == GLFW_PRESS)
        mouseLook = !mouseLook;

    // Interakcje (MRZ-06) - tylko gdy nie piszemy w polu ImGui.
    if (action == GLFW_PRESS && !ImGui::GetIO().WantCaptureKeyboard)
    {
        if (key == GLFW_KEY_F) headlampOn = !headlampOn;                       // latarka on/off
        if (key == GLFW_KEY_C) headlampColorIdx = (headlampColorIdx + 1) % 3;  // kolor latarki
        if (key == GLFW_KEY_B) showBioLights = !showBioLights;                 // bioluminescencja
    }
}

inline void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) return;
    // MRZ-05: scroll = jasnosc latarki (alternatywa dla +/-)
    headlampIntensity = glm::clamp(headlampIntensity + (float)yoffset * 0.6f, 0.0f, 30.0f);
}

inline void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    // MRZ-06: lewy klik straszy ryby w poblizu kamery
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && !ImGui::GetIO().WantCaptureMouse)
    {
        scareActive = true;
        scareTimer  = scareDuration;
        scareOrigin = camera.position;
    }
}

inline void updateCursorMode(GLFWwindow* window)
{
    // Nad panelem ImGui pokaż kursor; w widoku 3D schowaj go do rozglądania.
    if (ImGui::GetIO().WantCaptureMouse || !mouseLook)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

inline void processInput(GLFWwindow* window)
{
    static float lastTime = (float)glfwGetTime();
    float now = (float)glfwGetTime();
    float dt  = now - lastTime;
    lastTime  = now;

    float speed = 4.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f;
    float roll = rollSpeed * dt;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    // ruch wzdluz lokalnych osi kamery
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(speed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.moveForward(-speed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.moveRight(-speed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(speed);
    // gora/dol wzgledem swiata (ku powierzchni / w glab)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)        camera.moveWorldUp(speed);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.moveWorldUp(-speed);
    // przechyl (roll)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.addRoll(roll);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.addRoll(-roll);

    // MRZ-05: jasnosc latarki na +/- (alternatywa: scroll)
    float lampStep = 12.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
        headlampIntensity += lampStep;
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
        headlampIntensity -= lampStep;
    headlampIntensity = glm::clamp(headlampIntensity, 0.0f, 30.0f);
}

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------
inline void init(GLFWwindow* window)
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    // --- OLE-04: Shadow FBO (2048x2048, only depth) --------------------------
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Fragmenty poza mapa cieni nie sa w cieniu (border = 1.0)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
    glDrawBuffer(GL_NONE); // brak bufora koloru
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::OLE-04: Shadow FBO nie jest kompletny!" << std::endl;
    else
        std::cout << "[OLE-04] Shadow FBO " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << " OK" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    programPBR = shaderLoader.CreateProgram(
        (char*)"shaders/pbr.vert", (char*)"shaders/pbr.frag");
    programSkybox = shaderLoader.CreateProgram(
        (char*)"shaders/skybox.vert", (char*)"shaders/skybox.frag");
    programDebugLine = shaderLoader.CreateProgram(
        (char*)"shaders/debug_line.vert", (char*)"shaders/debug_line.frag");
    // NED-03: fish.vert deformuje, oswietlenie wspolne z pbr.frag (spojnosc z OLE-01)
    programFish = shaderLoader.CreateProgram(
        (char*)"shaders/fish.vert", (char*)"shaders/pbr.frag");
    // NED-06: meduza - osobny vertex shader (pulsowanie), wspolny pbr.frag
    programJelly = shaderLoader.CreateProgram(
        (char*)"shaders/jellyfish.vert", (char*)"shaders/pbr.frag");
    programWaterOverlay = shaderLoader.CreateProgram(
        (char*)"shaders/water_overlay.vert", (char*)"shaders/water_overlay.frag");
    programShadowDepth = shaderLoader.CreateProgram(
        (char*)"shaders/shadow_depth.vert", (char*)"shaders/shadow_depth.frag");
    programPostprocess = shaderLoader.CreateProgram(
        (char*)"shaders/postprocess.vert", (char*)"shaders/postprocess.frag");
    programParticle = shaderLoader.CreateProgram(
        (char*)"shaders/particle.vert", (char*)"shaders/particle.frag");

    // MRZ-07: babelki - emitery przy dnie (kominy/wybicia z piasku)
    {
        std::vector<glm::vec3> bubbleEmitters = {
            glm::vec3(-5.0f, -1.8f, -3.0f),
            glm::vec3( 3.0f, -1.8f,  2.0f),
            glm::vec3( 0.0f, -1.8f, -6.0f),
            glm::vec3( 6.0f, -1.8f,  4.0f),
        };
        bubbles.init(160, bubbleEmitters);
    }

    if (!loadModelToContext("./models/sphere.obj", sphereContext))
        std::cout << "Brak models/sphere.obj – dodaj model kuli do folderu models/" << std::endl;
    if (!loadModelToContext("./models/cube.obj", cubeContext))
        std::cout << "Brak models/cube.obj – metal cube nie bedzie widoczny" << std::endl;
    if (!loadModelToContext("./models/cube.obj", groundContext))
        std::cout << "Brak models/cube.obj – dno nie bedzie widoczne" << std::endl;
    if (!loadModelToContext("./models/fish.obj", fishContext))
        std::cout << "Brak models/fish.obj – ryby beda uzywac kuli jako placeholdera" << std::endl;
    if (!loadModelToContext("./models/jellyfish.obj", jellyContext))
        std::cout << "Brak models/jellyfish.obj – meduzy nie beda widoczne" << std::endl;

    // OLE-02: mapy PBR (piasek na dno, zardzewialy metal na kostke)
    {
        PBRTextureSet sandTex, rustTex;
        loadPBRTextureSet("./textures/pbr/sand", "sand", sandTex);
        loadPBRTextureSet("./textures/pbr/rusty_metal", "rusty_metal", rustTex);

        groundMaterial.tex = sandTex;
        groundMaterial.uvScale = glm::vec2(16.0f); // powtarzanie UV na duzej plaszczyznie

        metalMaterial.tex = rustTex;
        metalMaterial.metallic  = 1.0f;  // fallback gdy mapy wylaczone
        metalMaterial.roughness = 0.35f;
    }

    // Skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Fullscreen quad (2 trojkaty) dla overlayu wody - pozycje w NDC
    static const float waterQuadVerts[] = {
        -1.0f, -1.0f,   1.0f, -1.0f,   1.0f,  1.0f,
        -1.0f, -1.0f,   1.0f,  1.0f,  -1.0f,  1.0f
    };
    glGenVertexArrays(1, &waterQuadVAO);
    glGenBuffers(1, &waterQuadVBO);
    glBindVertexArray(waterQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(waterQuadVerts), waterQuadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Cubemap faces: +X, -X, +Y, -Y, +Z, -Z
    const char* faces[6] = {
        "textures/skybox/_px.jpg", "textures/skybox/_nx.jpg",
        "textures/skybox/_py.jpg", "textures/skybox/_ny.jpg",
        "textures/skybox/_pz.jpg", "textures/skybox/_nz.jpg"
    };
    skyboxCubemap = Core::LoadCubemap(faces);

    // --- Splajn (NED-01): zapetlona sciezka testowa nad dnem ----------------
    debugSpline.addControlPoint(glm::vec3( 6.0f, 1.0f,  0.0f));
    debugSpline.addControlPoint(glm::vec3( 3.0f, 2.5f,  5.0f));
    debugSpline.addControlPoint(glm::vec3(-3.0f, 3.0f,  4.0f));
    debugSpline.addControlPoint(glm::vec3(-6.0f, 1.0f, -1.0f));
    debugSpline.addControlPoint(glm::vec3(-2.0f, 0.0f, -5.0f));
    debugSpline.addControlPoint(glm::vec3( 4.0f, 2.0f, -4.0f));

    std::vector<glm::vec3> splinePts = debugSpline.sampleLine(32);
    splineVertexCount = (int)splinePts.size();

    glGenVertexArrays(1, &splineVAO);
    glGenBuffers(1, &splineVBO);
    glBindVertexArray(splineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, splineVBO);
    glBufferData(GL_ARRAY_BUFFER, splinePts.size() * sizeof(glm::vec3), splinePts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    // Szybki test poprawnosci w konsoli (NED-01 "done when").
    glm::vec3 p0 = debugSpline.evaluate(0.0f);
    glm::vec3 first = debugSpline.points()[0];
    std::cout << "[NED-01] evaluate(0) = (" << p0.x << ", " << p0.y << ", " << p0.z
              << "), pierwszy punkt = (" << first.x << ", " << first.y << ", " << first.z << ")\n";
    std::cout << "[NED-01] |tangent(0.0)| = " << glm::length(debugSpline.evaluateTangent(0.0f))
              << ", |tangent(0.5)| = " << glm::length(debugSpline.evaluateTangent(0.5f)) << "\n";

    // --- PTF (NED-02): buduj ramki i wgraj osie do GPU -----------------------
    debugSpline.buildFrames(128);

    // Co ktora ramke pokazujemy jako osie (zeby nie bylo za gesto).
    const int ptfStride = 4;
    const float axisLen = 0.35f;
    const auto& allFrames = debugSpline.frames();
    std::vector<glm::vec3> axisVerts;
    axisVerts.reserve(allFrames.size() / (size_t)ptfStride * 6);
    for (size_t fi = 0; fi < allFrames.size(); fi += (size_t)ptfStride)
    {
        const SplineFrame& f = allFrames[fi];
        axisVerts.push_back(f.position);
        axisVerts.push_back(f.position + f.T * axisLen);
        axisVerts.push_back(f.position);
        axisVerts.push_back(f.position + f.N * axisLen);
        axisVerts.push_back(f.position);
        axisVerts.push_back(f.position + f.B * axisLen);
    }
    ptfAxesVertexCount = (int)axisVerts.size();

    glGenVertexArrays(1, &ptfAxesVAO);
    glGenBuffers(1, &ptfAxesVBO);
    glBindVertexArray(ptfAxesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ptfAxesVBO);
    glBufferData(GL_ARRAY_BUFFER, axisVerts.size() * sizeof(glm::vec3), axisVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    // Test PTF w konsoli (NED-02 "done when").
    SplineFrame f0 = debugSpline.getFrame(0.0f);
    SplineFrame fh = debugSpline.getFrame(0.5f);
    std::cout << "[NED-02] frame(0): T=(" << f0.T.x << "," << f0.T.y << "," << f0.T.z
              << ") N=(" << f0.N.x << "," << f0.N.y << "," << f0.N.z << ")\n";
    std::cout << "[NED-02] |T|=" << glm::length(f0.T) << " |N|=" << glm::length(f0.N)
              << " dot(T,N)=" << glm::dot(f0.T, f0.N)
              << " (powinna byc bliska 0, T i N prostopadle)\n";
    std::cout << "[NED-02] frame(0.5): dot(T,N)=" << glm::dot(fh.T, fh.N) << "\n";

    // --- NED-04: druga sciezka dla ryb + ramki PTF --------------------------
    fishSplineB.addControlPoint(glm::vec3(-7.0f, 0.5f,  3.0f));
    fishSplineB.addControlPoint(glm::vec3(-2.0f, 1.0f,  7.0f));
    fishSplineB.addControlPoint(glm::vec3( 5.0f, 1.5f,  6.0f));
    fishSplineB.addControlPoint(glm::vec3( 7.0f, 0.8f, -2.0f));
    fishSplineB.addControlPoint(glm::vec3( 1.0f, 2.0f, -6.0f));
    fishSplineB.addControlPoint(glm::vec3(-5.0f, 1.2f, -4.0f));
    fishSplineB.buildFrames(128);

    std::vector<glm::vec3> splineBPts = fishSplineB.sampleLine(32);
    splineBVertexCount = (int)splineBPts.size();
    glGenVertexArrays(1, &splineBVAO);
    glGenBuffers(1, &splineBVBO);
    glBindVertexArray(splineBVAO);
    glBindBuffer(GL_ARRAY_BUFFER, splineBVBO);
    glBufferData(GL_ARRAY_BUFFER, splineBPts.size() * sizeof(glm::vec3), splineBPts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    // --- NED-04: instancje ryb na obu sciezkach -----------------------------
    // FishAnimation(path, speed [petla/s], tOffset, scale, swimPhase)
    fishes.clear(); fishColors.clear();
    fishes.emplace_back(&debugSpline, 0.060f, 0.00f, 1.5f, 0.0f);
    fishColors.push_back(glm::vec3(0.95f, 0.45f, 0.10f)); // pomaranczowa
    fishes.emplace_back(&debugSpline, 0.060f, 0.50f, 1.2f, 2.0f);
    fishColors.push_back(glm::vec3(0.95f, 0.80f, 0.18f)); // zolta
    fishes.emplace_back(&fishSplineB, 0.045f, 0.25f, 1.6f, 1.0f);
    fishColors.push_back(glm::vec3(0.85f, 0.22f, 0.40f)); // czerwono-rozowa
    fishes.emplace_back(&fishSplineB, 0.075f, 0.70f, 1.1f, 3.0f);
    fishColors.push_back(glm::vec3(0.25f, 0.75f, 0.55f)); // zielono-morska
    std::cout << "[NED-04] ryb na splajnach: " << fishes.size() << " (2 sciezki)\n";

    int w, h; glfwGetFramebufferSize(window, &w, &h);
    framebuffer_size_callback(window, w, h);
}

inline void shutdown(GLFWwindow* window)
{
    shaderLoader.DeleteProgram(programPBR);
    shaderLoader.DeleteProgram(programSkybox);
    shaderLoader.DeleteProgram(programDebugLine);
    shaderLoader.DeleteProgram(programFish);
    shaderLoader.DeleteProgram(programJelly);
    shaderLoader.DeleteProgram(programWaterOverlay);
    shaderLoader.DeleteProgram(programShadowDepth);  // OLE-04
    shaderLoader.DeleteProgram(programPostprocess);   // OLE-07
    glDeleteFramebuffers(1, &shadowFBO);              // OLE-04
    glDeleteTextures(1, &shadowDepthTex);             // OLE-04
    // OLE-07: scene FBO
    if (sceneFBO != 0)
    {
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteTextures(1, &sceneColorTex);
        glDeleteTextures(1, &sceneDepthTex);
    }
    glDeleteVertexArrays(1, &splineVAO);
    glDeleteBuffers(1, &splineVBO);
    glDeleteVertexArrays(1, &splineBVAO);
    glDeleteBuffers(1, &splineBVBO);
    glDeleteVertexArrays(1, &ptfAxesVAO);
    glDeleteBuffers(1, &ptfAxesVBO);
    glDeleteVertexArrays(1, &waterQuadVAO);
    glDeleteBuffers(1, &waterQuadVBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
inline void renderLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("WSAD ruch, Q/E przechyl, Spacja/Ctrl gora-dol");
        ImGui::Text("Suwaki: najedz myszka na panel (Caps Lock = lock kamery)");
        if (ImGui::GetIO().WantCaptureMouse)
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "Panel aktywny - suwaki dzialaja");
        ImGui::Separator();
        ImGui::Text("Kamera: %6.1f %6.1f %6.1f", camera.position.x, camera.position.y, camera.position.z);
        ImGui::PushItemWidth(160.0f);
        ImGui::SliderFloat("Czulosc myszy", &mouseSensitivity, 0.02f, 0.4f);
        ImGui::Separator();
        ImGui::Checkbox("Pokaz splajn (NED-01)", &showSpline);
        ImGui::Checkbox("Pokaz ramki PTF (NED-02)", &showPTF);
        ImGui::Separator();
        ImGui::Text("NED-03 Ryby (A10)");
        ImGui::Checkbox("Pokaz ryby", &showFish);
        ImGui::SliderFloat("Wave amplitude", &fishParams.waveAmplitude, 0.0f, 1.0f);
        ImGui::SliderFloat("Wave frequency", &fishParams.waveFrequency, 0.5f, 15.0f);
        ImGui::SliderFloat("Wave speed",     &fishParams.waveSpeed,     0.0f, 15.0f);
        ImGui::SliderFloat("Fin amplitude",  &fishParams.finAmplitude,  0.0f, 0.5f);
        ImGui::TextDisabled("Lewy klik = strasz ryby w poblizu");
        ImGui::SliderFloat("Strach: promien", &scareRadius, 1.0f, 20.0f);
        ImGui::SliderFloat("Strach: mnoznik", &scareSpeedMult, 1.0f, 10.0f);
        ImGui::Separator();
        ImGui::Text("NED-06 Meduzy");
        ImGui::Checkbox("Pokaz meduzy", &showJelly);
        ImGui::SliderFloat("Puls amplituda", &jellyParams.pulseAmplitude,    0.0f, 0.5f);
        ImGui::SliderFloat("Puls predkosc",  &jellyParams.pulseSpeed,        0.0f, 6.0f);
        ImGui::SliderFloat("Czulki amplit.", &jellyParams.tentacleAmplitude, 0.0f, 0.3f);
        ImGui::SliderFloat("Bob pionowy",    &jellyParams.bobAmplitude,      0.0f, 0.6f);
        ImGui::Separator();
        ImGui::Checkbox("Efekt wody (overlay)", &showWaterOverlay);
        ImGui::SliderFloat("Sila efektu wody", &waterOverlayStrength, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::SliderFloat("Fog density", &fogDensity, 0.0f, 0.15f);
        ImGui::ColorEdit3("Water color", (float*)&waterColor);
        ImGui::ColorEdit3("Sun color",   (float*)&sunColor);
        ImGui::SliderFloat3("Sun dir",   (float*)&sunDir, -1.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("OLE-04 Shadow mapping (M4)");
        ImGui::Checkbox("Cienie wlaczone", &useShadows);
        ImGui::SliderFloat("Shadow ortho size", &shadowOrthoSize, 5.0f, 60.0f);
        ImGui::SliderFloat("Bias min", &shadowBiasMin, 0.0f, 0.01f, "%.4f");
        ImGui::SliderFloat("Bias max", &shadowBiasMax, 0.0f, 0.1f, "%.3f");
        ImGui::Checkbox("PCF 5x5 (mieksze krawedzie)", &usePCF5x5);
        ImGui::Separator();
        ImGui::Text("OLE-01 PBR (kula - uniformy)");
        ImGui::ColorEdit3("Albedo", (float*)&sphereMaterial.albedo);
        ImGui::SliderFloat("Metallic",  &sphereMaterial.metallic,  0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &sphereMaterial.roughness, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("OLE-02 PBR tekstury");
        if (ImGui::Checkbox("Mapy piasku (dno)", &groundUseTextures))
            setTextureFlags(groundMaterial.tex, groundUseTextures);
        if (ImGui::Checkbox("Mapy zardzewialego metalu (kostka)", &metalUseTextures))
            setTextureFlags(metalMaterial.tex, metalUseTextures);
        ImGui::SliderFloat("Metal roughness (fallback)", &metalMaterial.roughness, 0.0f, 1.0f);
        ImGui::TextDisabled("Kula i ryby: tylko uniformy");
        ImGui::Separator();
        ImGui::Text("OLE-03 Normal mapping (M1)");
        if (ImGui::Checkbox("Normal map: piasek (dno)", &groundUseNormalMap))
        {
            if (groundMaterial.tex.normalMap)
                groundMaterial.tex.useNormalMap = groundUseNormalMap;
        }
        if (ImGui::Checkbox("Normal map: metal (kostka)", &metalUseNormalMap))
        {
            if (metalMaterial.tex.normalMap)
                metalMaterial.tex.useNormalMap = metalUseNormalMap;
        }
        ImGui::TextDisabled("Wlacz/wylacz aby porownac plaski vs wyboje");
        ImGui::Separator();
        ImGui::Text("MRZ-05 Latarka nurka (B13)");
        ImGui::Checkbox("Latarka [F]", &headlampOn);
        if (ImGui::Button("Kolor [C]")) headlampColorIdx = (headlampColorIdx + 1) % 3;
        ImGui::SameLine();
        ImGui::Text("%s", headlampColorNames[headlampColorIdx]);
        ImGui::SliderFloat("Jasnosc [+/- / scroll]", &headlampIntensity, 0.0f, 30.0f);
        ImGui::Separator();
        ImGui::Text("OLE-05 Wiele swiatel");
        ImGui::Checkbox("Bioluminescencja [B] (3 point lights)", &showBioLights);
        ImGui::SliderFloat("Bio intensywnosc", &bioIntensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Bio pulsowanie", &bioPulseSpeed, 0.0f, 5.0f);
        ImGui::ColorEdit3("Bio 1 (niebieski)", (float*)&bioColors[0]);
        ImGui::ColorEdit3("Bio 2 (zielony)",   (float*)&bioColors[1]);
        ImGui::ColorEdit3("Bio 3 (fioletowy)", (float*)&bioColors[2]);
        ImGui::Text("Aktywne: %d point, %d spot", numPointLights, numSpotLights);
        ImGui::Separator();
        ImGui::Text("MRZ-07 Babelki");
        ImGui::Checkbox("Babelki", &showBubbles);
        ImGui::ColorEdit3("Kolor babelkow", (float*)&bubbleColor);
        ImGui::Separator();
        ImGui::Text("OLE-07 Post-processing podwodny");
        ImGui::Checkbox("Post-processing wlaczony", &usePostprocess);
        ImGui::ColorEdit3("Tint podwodny", (float*)&ppTint);
        ImGui::SliderFloat("Sila tintu", &ppTintStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Mgla glebi - gestosc", &ppDepthFogDensity, 0.0f, 0.5f);
        ImGui::SliderFloat("Mgla glebi - start", &ppDepthFogStart, 0.0f, 20.0f);
        ImGui::ColorEdit3("Kolor mgly glebi", (float*)&ppDepthFogColor);
        ImGui::SliderFloat("Aberracja chromatyczna", &ppChromaticStrength, 0.0f, 0.02f, "%.4f");
        ImGui::SliderFloat("Vignette", &ppVignetteStrength, 0.0f, 1.5f);
        ImGui::PopItemWidth();
        ImGui::End();

        updateCursorMode(window);

        renderScene(window);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
