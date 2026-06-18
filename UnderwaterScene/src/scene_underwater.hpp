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
#include "SOIL/SOIL.h"           // dekodowanie osadzonych tekstur GLB z pamieci
#include "Camera.h"               // Core::createPerspectiveMatrix
#include "QuaternionCamera.h"     // MRZ-02 quaternion camera
#include "ParticleSystem.h"       // MRZ-07 babelki
#include "Spline.h"               // NED-01 splajn Catmull-Rom
#include "FishAnimation.h"        // NED-04 ryby po splajnie z PTF
#include "AnimatedModel.h"        // NED nurek - animacja szkieletowa (GPU skinning)

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
    GLuint programSketchFish = 0; // ta sama deformacja dla multi-mesh ryb Sketchfab
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

    // --- Modele Sketchfab (multi-mesh GLB) -----------------------------------
    using MultiMeshModel = std::vector<Core::RenderContext>;

    struct SceneProp {
        MultiMeshModel* model;
        glm::mat4 transform;
        PBRMaterial material;
    };

    MultiMeshModel coralModel1;     // coral.glb
    MultiMeshModel coralModel2;     // Coral 3D Model.glb
    MultiMeshModel coralPiece;      // Coral Piece.glb
    MultiMeshModel crescentCoral;   // Crescent Moon Coral.glb
    MultiMeshModel coralFish;       // coral_fish.glb
    MultiMeshModel deepSeaFish;     // Deep Sea Fish 3D.glb
    MultiMeshModel guppyFish;       // Guppy Fish.glb
    MultiMeshModel shinyFish;       // Shiny Fish.glb
    MultiMeshModel porscheModel;    // Porsche 911 Turbo 1975.glb

    std::vector<SceneProp> sceneProps;
    bool showProps = true;

    GLuint skyboxVAO = 0, skyboxVBO = 0;
    GLuint skyboxCubemap = 0;

    // --- Splajn (NED-01) -----------------------------------------------------
    Spline debugSpline;                 // sciezka A (ryby + podglad)
    GLuint splineVAO = 0, splineVBO = 0;
    int    splineVertexCount = 0;
    bool   showSpline = true;

    // --- Druga sciezka dla ryb (NED-04) --------------------------------------
    Spline fishSplineB;                 // sciezka B (druga grupa ryb)
    Spline fishSplineC;                 // sciezka C (ryby Sketchfab)
    GLuint splineBVAO = 0, splineBVBO = 0;
    int    splineBVertexCount = 0;

    // Ryby Sketchfab na splajnach (rysowane drawMultiMeshPBR)
    struct SketchFishInstance {
        FishAnimation anim;
        MultiMeshModel* model;
        PBRMaterial material;
        glm::mat4 localFix = glm::mat4(1.0f); // skala+centrowanie+wyrownanie ciala (fishLocalFix)
        float bodyLen = 1.0f;                 // dlugosc ciala po normalizacji (= targetSize)
    };
    std::vector<SketchFishInstance> sketchFish;

    // --- Nurek (NED): animacja szkieletowa, GPU skinning ---------------------
    AnimatedModel          diver;
    GLuint                 programSkinned = 0;
    std::vector<glm::mat4> diverBones;            // macierze kosci na biezaca klatke
    bool                   showDiver      = true;
    int                    diverClip      = 0;    // ktory z 5 klipow Mixamo
    int                    diverClipSwim  = 0;    // klip plywania (ustawiany w init)
    int                    diverClipIdle  = 0;    // klip idle
    float                  diverAnimSpeed = 1.0f; // tempo animacji
    // Pozycja i orientacja nurka (3-cia osoba)
    glm::vec3 diverPos       = glm::vec3(0.0f, 2.5f, 0.0f);
    glm::quat diverOrient    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // kwaternion orientacji
    glm::vec3 diverRotDeg    = glm::vec3(0.0f, 180.0f, 0.0f); // pitch(X), yaw(Y), roll(Z) - Euler fallback
    float     diverScaleMul  = 0.6f;        // mnoznik do auto-skali
    float     diverBaseScale = 1.0f;        // auto-skala z AABB (liczona przy load)
    glm::vec3 diverCenter    = glm::vec3(0.0f); // srodek bind-pose (do centrowania)
    bool      diverMoving    = false;       // czy nurek sie rusza w tej klatce

    // Tryb trzeciej osoby
    bool      thirdPerson       = true;     // V = toggle
    float     tpCamDistance     = 3.0f;     // odleglosc kamery za nurkiem
    float     tpCamHeight       = 1.2f;     // wysokosc kamery nad nurkiem
    float     tpCamSmoothFactor = 8.0f;     // gladkosc sledzenia (wieksze = szybsze)
    glm::vec3 tpCamCurrentPos;              // aktualna pozycja kamery (gladzenie)
    bool      tpCamInitialized  = false;    // czy ustawiono startowa pozycje

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
    bool  showPanel = true;   // H: pokaz/ukryj panel ImGui (czysty widok na demo / screeny)

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
    if (thirdPerson && diver.valid())
    {
        glm::vec3 lookAt = diverPos + glm::vec3(0.0f, 0.5f, 0.0f);
        return glm::lookAt(camera.position, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    }
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

// Rysuje multi-mesh rybe Sketchfab z deformacja plywania (sketchfish.vert).
// preTransform = localFix (normalizacja modelu), modelMatrix = ramka PTF splajnu.
// bodyLen = dlugosc ciala po normalizacji (= targetSize). Per sub-mesh bindujemy
// jego wlasna teksture base-color (jak w drawMultiMeshPBR).
inline void drawSketchFish(MultiMeshModel& model, const glm::mat4& modelMatrix,
                           const glm::mat4& preTransform, const PBRMaterial& material,
                           float time, float phaseOffset, float bodyLen,
                           const glm::mat4& lightSpaceMat)
{
    glUseProgram(programSketchFish);

    glm::mat4 view = createCameraMatrix();
    glm::mat4 projection = createPerspectiveMatrix();

    glUniformMatrix4fv(glGetUniformLocation(programSketchFish, "model"),        1, GL_FALSE, (float*)&modelMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programSketchFish, "preTransform"), 1, GL_FALSE, (float*)&preTransform);
    glUniformMatrix4fv(glGetUniformLocation(programSketchFish, "view"),         1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(programSketchFish, "projection"),   1, GL_FALSE, (float*)&projection);

    glUniform3fv(glGetUniformLocation(programSketchFish, "cameraPos"),  1, (float*)&camera.position);
    glUniform3fv(glGetUniformLocation(programSketchFish, "lightDir"),   1, (float*)&sunDir);
    glUniform3fv(glGetUniformLocation(programSketchFish, "lightColor"), 1, (float*)&sunColor);

    bindShadowUniforms(programSketchFish, lightSpaceMat);
    bindLightUniforms(programSketchFish);

    glUniform3fv(glGetUniformLocation(programSketchFish, "fogColor"),  1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programSketchFish, "fogDensity"), fogDensity);

    glUniform1f(glGetUniformLocation(programSketchFish, "time"),          time + phaseOffset);
    glUniform1f(glGetUniformLocation(programSketchFish, "waveAmplitude"), fishParams.waveAmplitude);
    glUniform1f(glGetUniformLocation(programSketchFish, "waveFrequency"), fishParams.waveFrequency);
    glUniform1f(glGetUniformLocation(programSketchFish, "waveSpeed"),     fishParams.waveSpeed);
    glUniform1f(glGetUniformLocation(programSketchFish, "bodyLength"),    bodyLen);
    glUniform1f(glGetUniformLocation(programSketchFish, "finAmplitude"),  fishParams.finAmplitude);

    for (auto& ctx : model)
    {
        PBRMaterial m = material;
        if (ctx.albedoTex) { m.tex.albedoMap = ctx.albedoTex; m.tex.useAlbedoMap = true; }
        bindPBRMaterial(programSketchFish, m);
        Core::DrawContext(ctx);
    }
    glUseProgram(0);
}

// Buduje macierz umiejscowienia nurka.
// Model Mixamo ma przod w +Z, a my ruszamy sie w -Z -> dodajemy obrot 180 stopni.
inline glm::mat4 buildDiverMatrix()
{
    // Kompensacja: model Mixamo patrzy w +Z, my ruszamy sie w -Z
    static const glm::mat4 modelFlip = glm::rotate(glm::radians(180.0f), glm::vec3(0,1,0));

    if (thirdPerson)
    {
        glm::mat4 rot = glm::mat4_cast(diverOrient);
        return glm::translate(diverPos)
             * rot
             * modelFlip
             * glm::scale(glm::vec3(diverBaseScale * diverScaleMul))
             * glm::translate(-diverCenter);
    }
    else
    {
        return glm::translate(diverPos)
             * glm::rotate(glm::radians(diverRotDeg.y), glm::vec3(0,1,0))
             * glm::rotate(glm::radians(diverRotDeg.x), glm::vec3(1,0,0))
             * glm::rotate(glm::radians(diverRotDeg.z), glm::vec3(0,0,1))
             * glm::scale(glm::vec3(diverBaseScale * diverScaleMul))
             * glm::translate(-diverCenter);
    }
}

// Wektory bazowe nurka z kwaternionu orientacji.
inline glm::vec3 diverFront() { return diverOrient * glm::vec3(0.0f, 0.0f, -1.0f); }
inline glm::vec3 diverRight() { return diverOrient * glm::vec3(1.0f, 0.0f,  0.0f); }
inline glm::vec3 diverUp()    { return diverOrient * glm::vec3(0.0f, 1.0f,  0.0f); }

// Aktualizuje kamere w trybie 3-ciej osoby — podaza za nurkiem z gladkim lag-em.
inline void updateThirdPersonCamera(float dt)
{
    // Punkt docelowy: za nurkiem + nad nurkiem
    glm::vec3 targetCamPos = diverPos
                           - diverFront() * tpCamDistance
                           + glm::vec3(0.0f, tpCamHeight, 0.0f);

    if (!tpCamInitialized)
    {
        tpCamCurrentPos = targetCamPos;
        tpCamInitialized = true;
    }
    else
    {
        // Exponential smooth (lerp)
        float t = glm::clamp(tpCamSmoothFactor * dt, 0.0f, 1.0f);
        tpCamCurrentPos = glm::mix(tpCamCurrentPos, targetCamPos, t);
    }

    // Clamp kamera nad podloga
    if (tpCamCurrentPos.y < 0.6f) tpCamCurrentPos.y = 0.6f;

    // Kamera patrzy na nurka (troche powyzej srodka ciala)
    glm::vec3 lookAt = diverPos + glm::vec3(0.0f, 0.5f, 0.0f);
    camera.position = tpCamCurrentPos;
    // Ustawiamy orientacje kamery tak, by patrzyla na nurka
    // Uzywamy glm::lookAt do wyliczenia macierzy widoku wprost
}

// Rysuje nurka z animacja szkieletowa (GPU skinning, skinned.vert + pbr.frag).
// Poza liczona z klipu diverClip w czasie; umiejscowienie z buildDiverMatrix().
inline void drawDiver(float time, const glm::mat4& lightSpaceMat)
{
    if (!showDiver || !diver.valid()) return;

    glm::mat4 modelMatrix = buildDiverMatrix();
    diver.computePose(diverClip, time, diverAnimSpeed, diverBones);

    glUseProgram(programSkinned);

    glm::mat4 view = createCameraMatrix();
    glm::mat4 projection = createPerspectiveMatrix();
    glUniformMatrix4fv(glGetUniformLocation(programSkinned, "model"),      1, GL_FALSE, (float*)&modelMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programSkinned, "view"),       1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(programSkinned, "projection"), 1, GL_FALSE, (float*)&projection);

    // Macierze kosci -> uniform finalBones[]
    if (!diverBones.empty())
        glUniformMatrix4fv(glGetUniformLocation(programSkinned, "finalBones"),
                           (GLsizei)diverBones.size(), GL_FALSE, (float*)diverBones.data());

    glUniform3fv(glGetUniformLocation(programSkinned, "cameraPos"),  1, (float*)&camera.position);
    glUniform3fv(glGetUniformLocation(programSkinned, "lightDir"),   1, (float*)&sunDir);
    glUniform3fv(glGetUniformLocation(programSkinned, "lightColor"), 1, (float*)&sunColor);

    bindShadowUniforms(programSkinned, lightSpaceMat);
    bindLightUniforms(programSkinned);
    glUniform3fv(glGetUniformLocation(programSkinned, "fogColor"),  1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programSkinned, "fogDensity"), fogDensity);

    for (const auto& sm : diver.meshes())
    {
        PBRMaterial m;
        m.albedo    = sm.albedo;
        m.metallic  = sm.metallic;
        m.roughness = glm::clamp(sm.roughness, 0.05f, 1.0f);
        bindPBRMaterial(programSkinned, m);

        glBindVertexArray(sm.vao);
        glDrawElements(GL_TRIANGLES, sm.indexCount, GL_UNSIGNED_INT, (void*)0);
    }
    glBindVertexArray(0);
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

// Rysuje multi-mesh model (GLB ze Sketchfab) z PBR.
inline void drawMultiMeshPBR(MultiMeshModel& model, glm::mat4 modelMatrix,
                              const PBRMaterial& material, const glm::mat4& lightSpaceMat)
{
    for (auto& ctx : model)
    {
        if (ctx.albedoTex)
        {
            // Sub-mesh ma wlasna teksture base-color z GLB -> uzyj jej zamiast plaskiego koloru.
            PBRMaterial m = material;
            m.tex.albedoMap = ctx.albedoTex;
            m.tex.useAlbedoMap = true;
            drawPBRObject(ctx, modelMatrix, m, lightSpaceMat);
        }
        else
        {
            drawPBRObject(ctx, modelMatrix, material, lightSpaceMat);
        }
    }
}

// Rysuje multi-mesh model do shadow depth FBO.
inline void drawMultiMeshShadow(MultiMeshModel& model, const glm::mat4& modelMatrix,
                                 const glm::mat4& lightSpaceMat)
{
    for (auto& ctx : model)
        drawShadowDepth(ctx, modelMatrix, lightSpaceMat);
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

    // --- MRZ-05 (B13): latarka nurka - spotlight jadacy z kamera/nurkiem ------
    if (headlampOn && numSpotLights < MAX_SPOT_LIGHTS)
    {
        SpotLightCPU& sl = spotLights[numSpotLights];
        if (thirdPerson && diver.valid())
        {
            // W 3-ciej osobie latarka swieci z pozycji nurka, w kierunku nurka
            sl.position  = diverPos + glm::vec3(0.0f, 0.8f, 0.0f); // na wysokosci glowy
            sl.direction = diverFront();
        }
        else
        {
            sl.position  = camera.position;
            sl.direction = camera.front();
        }
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

        // Dekoracje Sketchfab
        if (showProps)
        {
            for (auto& prop : sceneProps)
                drawMultiMeshShadow(*prop.model, prop.transform, lightSpaceMat);
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

    // --- Dekoracje Sketchfab (korale, Porsche) ---
    if (showProps)
    {
        for (auto& prop : sceneProps)
            drawMultiMeshPBR(*prop.model, prop.transform, prop.material, lightSpaceMat);
    }

    // --- Ryby Sketchfab na splajnach ---
    if (showProps)
    {
        static float sfLastTime = time;
        float sfDt = time - sfLastTime;
        sfLastTime = time;
        if (sfDt < 0.0f) sfDt = 0.0f;
        if (sfDt > 0.1f) sfDt = 0.1f;

        for (auto& sf : sketchFish)
        {
            sf.anim.update(sfDt);
            // Ta sama deformacja plywania co nasze ryby (sketchfish.vert):
            // frameMatrix() umiejscawia, localFix normalizuje model do przestrzeni ciala.
            drawSketchFish(*sf.model, sf.anim.frameMatrix(), sf.localFix, sf.material,
                           time, sf.anim.swimPhase(), sf.bodyLen, lightSpaceMat);
        }
    }

    // --- Nurek (animacja szkieletowa, GPU skinning) ---
    drawDiver(time, lightSpaceMat);

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

// Laduje osadzona (w GLB) teksture base-color danego mesha do tekstury OpenGL.
// Zwraca 0 gdy material nie ma tekstury albo jest zewnetrzna. To naprawia
// "biale jak z gliny" modele Sketchfab - dostaja swoje prawdziwe kolory.
inline GLuint loadEmbeddedAlbedo(const aiScene* scene, const aiMesh* mesh)
{
    if (mesh->mMaterialIndex >= scene->mNumMaterials) return 0;
    const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    aiString texPath;
    if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) != AI_SUCCESS &&
        mat->GetTexture(aiTextureType_DIFFUSE,    0, &texPath) != AI_SUCCESS)
        return 0;

    const aiTexture* tex = scene->GetEmbeddedTexture(texPath.C_Str());
    if (!tex) return 0; // zewnetrzny plik - GLB zwykle osadza wszystko, wiec pomijamy

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (tex->mHeight == 0)
    {
        // Skompresowana (PNG/JPG) w pamieci: mWidth = liczba bajtow danych.
        int w = 0, h = 0, ch = 0;
        unsigned char* img = SOIL_load_image_from_memory(
            (const unsigned char*)tex->pcData, (int)tex->mWidth, &w, &h, &ch, SOIL_LOAD_RGBA);
        if (!img) { glDeleteTextures(1, &id); return 0; }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
        SOIL_free_image_data(img);
    }
    else
    {
        // Surowe piksele aiTexel (BGRA wg Assimp): mWidth x mHeight.
        int w = (int)tex->mWidth, h = (int)tex->mHeight;
        std::vector<unsigned char> rgba((size_t)w * h * 4);
        for (int i = 0; i < w * h; ++i) {
            rgba[i*4+0] = tex->pcData[i].r;
            rgba[i*4+1] = tex->pcData[i].g;
            rgba[i*4+2] = tex->pcData[i].b;
            rgba[i*4+3] = tex->pcData[i].a;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

inline bool loadMultiMeshModel(const std::string& path, MultiMeshModel& model)
{
    Assimp::Importer import;
    // aiProcess_PreTransformVertices WYPIEKA transformacje wezlow (skala/obrot/translacja
    // z grafu sceny GLB, w tym konwersje Z-up->Y-up) prosto w wierzcholki. Bez tego
    // modele Sketchfab laduja w surowym ukladzie mesha -> lezace zamiast stojace.
    const aiScene* scene = import.ReadFile(path,
        aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenNormals
        | aiProcess_PreTransformVertices);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << path << " -> " << import.GetErrorString() << std::endl;
        return false;
    }
    model.resize(scene->mNumMeshes);
    int totalTris = 0, texCount = 0;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i)
    {
        model[i].initFromAssimpMesh(scene->mMeshes[i]);
        model[i].albedoTex = loadEmbeddedAlbedo(scene, scene->mMeshes[i]);
        if (model[i].albedoTex) ++texCount;
        totalTris += model[i].size / 3;
    }
    std::cout << "Loaded multi-mesh: " << path << " (" << scene->mNumMeshes
              << " meshes, " << totalTris << " tris, " << texCount << " tekstur)" << std::endl;
    return true;
}

// AABB calego wieloczesciowego modelu (suma AABB sub-meshy, po PreTransformVertices).
inline void modelAABB(const MultiMeshModel& model, glm::vec3& mn, glm::vec3& mx)
{
    mn = glm::vec3( 1e30f);
    mx = glm::vec3(-1e30f);
    bool any = false;
    for (const auto& c : model)
    {
        if (c.aabbMin == c.aabbMax) continue; // pusty mesh
        mn = glm::min(mn, c.aabbMin);
        mx = glm::max(mx, c.aabbMax);
        any = true;
    }
    if (!any) { mn = glm::vec3(0.0f); mx = glm::vec3(0.0f); }
}

// Macierz dekoracji: skaluje model tak, by jego najwiekszy wymiar == targetSize,
// centruje go w poziomie nad (pos.x,pos.z) i SADZA dnem dokladnie na wysokosci pos.y.
// Dzieki temu nic nie tonie w piasku niezaleznie od originu/skali modelu GLB.
inline glm::mat4 placeProp(const MultiMeshModel& model, glm::vec3 pos,
                           float targetSize, float yawRad = 0.0f)
{
    glm::vec3 mn, mx; modelAABB(model, mn, mx);
    glm::vec3 sz = mx - mn;
    float maxDim = glm::max(sz.x, glm::max(sz.y, sz.z));
    float s = (maxDim > 1e-6f) ? targetSize / maxDim : 1.0f;
    glm::vec3 c = 0.5f * (mn + mx);
    // translate(pos) * yaw * scale(s) * (przesun dol-srodek modelu do origin)
    return glm::translate(pos)
         * glm::rotate(yawRad, glm::vec3(0.0f, 1.0f, 0.0f))
         * glm::scale(glm::vec3(s))
         * glm::translate(glm::vec3(-c.x, -mn.y, -c.z));
}

// Lokalna korekta modelu ryby Sketchfab dla ruchu po splajnie:
// - skaluje do targetSize, centruje na srodku ciala (zeby nie zataczala luku obok sciezki),
// - wyrownuje najdluzsza pozioma os ciala do lokalnego -Z (kierunek "do przodu" ramki PTF),
//   wiec ryba plynie wzdluz ciala, a nie bokiem.
// headingDeg: dodatkowy obrot wokol pionu na wypadek gdy nos modelu wskazuje
// w przeciwna strone niz kierunek plyniecia (frame patrzy w -Z). 180 = obroc rybe
// "przodem do przodu". Tu strojone per-instancja, bo kazdy model Sketchfab ma inny przod.
inline glm::mat4 fishLocalFix(const MultiMeshModel& model, float targetSize, float headingDeg)
{
    glm::vec3 mn, mx; modelAABB(model, mn, mx);
    glm::vec3 sz = mx - mn;
    float maxDim = glm::max(sz.x, glm::max(sz.y, sz.z));
    float s = (maxDim > 1e-6f) ? targetSize / maxDim : 1.0f;
    glm::vec3 c = 0.5f * (mn + mx);
    // Jesli cialo jest dluzsze w X niz w Z -> obroc o 90st, by dlugosc legla wzdluz Z.
    float bodyYaw = ((sz.x > sz.z) ? glm::radians(90.0f) : 0.0f) + glm::radians(headingDeg);
    return glm::scale(glm::vec3(s))
         * glm::rotate(bodyYaw, glm::vec3(0.0f, 1.0f, 0.0f))
         * glm::translate(-c);
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

    if (thirdPerson)
    {
        // Mysz obraca nurka (yaw wokol swiata-gora, pitch wokol lokalnego prawa)
        glm::quat qYaw   = glm::angleAxis(glm::radians(-dx), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat qPitch = glm::angleAxis(glm::radians(dy),  diverRight());
        diverOrient = glm::normalize(qYaw * qPitch * diverOrient);
    }
    else
    {
        camera.addYawPitch(-dx, dy);
    }
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
        if (key == GLFW_KEY_H) showPanel = !showPanel;                         // pokaz/ukryj panel
        if (key == GLFW_KEY_V)                                                 // 3-cia osoba / free-cam
        {
            thirdPerson = !thirdPerson;
            if (!thirdPerson)
            {
                // Przejscie do 1-szej osoby: kamera w oczach nurka
                camera.position = diverPos + glm::vec3(0.0f, 0.8f, 0.0f);
                // Synchronizuj orientacje kamery z orientacja nurka
                // (Camera uzywa kwaterniony, wiec to proste)
                // Nurek patrzy w diverFront(), kamera tez powinna
            }
            else
            {
                // Przejscie do 3-ciej osoby: nurek = pozycja kamery
                diverPos = camera.position;
                tpCamInitialized = false; // reset smooth followowania
            }
        }
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

    if (thirdPerson && diver.valid())
    {
        // === TRYB 3-CIEJ OSOBY: WSAD rusza nurkiem ===
        diverMoving = false;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { diverPos += diverFront() * speed; diverMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { diverPos -= diverFront() * speed; diverMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { diverPos -= diverRight() * speed; diverMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { diverPos += diverRight() * speed; diverMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)        { diverPos.y += speed; diverMoving = true; }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) { diverPos.y -= speed; diverMoving = true; }
        // Roll nurka
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            diverOrient = glm::normalize(diverOrient * glm::angleAxis(glm::radians(roll), glm::vec3(0.0f, 0.0f, -1.0f)));
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            diverOrient = glm::normalize(diverOrient * glm::angleAxis(glm::radians(-roll), glm::vec3(0.0f, 0.0f, -1.0f)));

        // Clamp nurka nad podloga
        if (diverPos.y < 0.6f) diverPos.y = 0.6f;

        // Kamera podaza za nurkiem
        updateThirdPersonCamera(dt);
    }
    else
    {
        // === TRYB FREE-CAM: WSAD rusza kamera ===
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward(speed);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.moveForward(-speed);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.moveRight(-speed);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight(speed);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)        camera.moveWorldUp(speed);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.moveWorldUp(-speed);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.addRoll(roll);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.addRoll(-roll);

        // Nie pozwol kamerze wplynac pod piasek
        const float minCameraY = 0.4f;
        if (camera.position.y < minCameraY)
            camera.position.y = minCameraY;
    }

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
    // Ta sama deformacja dla multi-mesh ryb Sketchfab (z dodatkowa normalizacja modelu)
    programSketchFish = shaderLoader.CreateProgram(
        (char*)"shaders/sketchfish.vert", (char*)"shaders/pbr.frag");
    // NED nurek: GPU skinning (animacja szkieletowa), oswietlenie wspolne z pbr.frag
    programSkinned = shaderLoader.CreateProgram(
        (char*)"shaders/skinned.vert", (char*)"shaders/pbr.frag");
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
        // Emitery przy dnie (gora piasku na y=0) - babelki wybijaja z piasku i wznosza sie.
        std::vector<glm::vec3> bubbleEmitters = {
            glm::vec3(-5.0f, 0.0f, -3.0f),
            glm::vec3( 3.0f, 0.0f,  2.0f),
            glm::vec3( 0.0f, 0.0f, -6.0f),
            glm::vec3( 6.0f, 0.0f,  4.0f),
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

    // --- Modele Sketchfab (multi-mesh GLB) ---
    loadMultiMeshModel("./models/coral.glb", coralModel1);
    loadMultiMeshModel("./models/Coral 3D Model.glb", coralModel2);
    loadMultiMeshModel("./models/Coral Piece.glb", coralPiece);
    loadMultiMeshModel("./models/Crescent Moon Coral.glb", crescentCoral);
    loadMultiMeshModel("./models/coral_fish.glb", coralFish);
    loadMultiMeshModel("./models/Deep Sea Fish 3D.glb", deepSeaFish);
    loadMultiMeshModel("./models/Guppy Fish.glb", guppyFish);
    loadMultiMeshModel("./models/Shiny Fish.glb", shinyFish);
    loadMultiMeshModel("./models/Porsche 911 Turbo 1975.glb", porscheModel);

    // Rozmieszczenie dekoracji na scenie.
    // Dno (cube.obj ma wierzcholki +-10): translate(0,-2,0)*scale(40,0.2,40)
    //   => gora piasku na y = 10*0.2 - 2 = 0.0 (NIE -1.9!). Stad floorY = 0.
    // placeProp() sam liczy AABB (po PreTransformVertices), skaluje do zadanego
    // rozmiaru w jednostkach swiata i SADZA model dnem na floorY - nic nie tonie.
    const float floorY = 0.0f;
    sceneProps.clear();

    // === KORALOWCE (gesty rif) === targetSize = najwiekszy wymiar w jedn. swiata
    sceneProps.push_back({&coralModel1, placeProp(coralModel1, glm::vec3(-8.0f, floorY, -5.0f), 3.4f, 0.0f), {glm::vec3(0.85f,0.35f,0.30f), 0.0f, 0.7f}});
    sceneProps.push_back({&coralModel1, placeProp(coralModel1, glm::vec3( 3.0f, floorY, -3.0f), 3.0f, 0.8f), {glm::vec3(0.75f,0.25f,0.20f), 0.0f, 0.7f}});
    sceneProps.push_back({&coralModel1, placeProp(coralModel1, glm::vec3(12.0f, floorY,  5.0f), 4.2f, 2.2f), {glm::vec3(0.80f,0.40f,0.55f), 0.0f, 0.7f}});
    sceneProps.push_back({&coralModel2, placeProp(coralModel2, glm::vec3( 5.0f, floorY,  8.0f), 4.0f, 0.0f), {glm::vec3(0.90f,0.55f,0.40f), 0.0f, 0.6f}});
    sceneProps.push_back({&coralModel2, placeProp(coralModel2, glm::vec3(-4.0f, floorY, -9.0f), 3.2f, 1.5f), {glm::vec3(0.95f,0.40f,0.30f), 0.0f, 0.55f}});
    sceneProps.push_back({&coralPiece,  placeProp(coralPiece,  glm::vec3(-3.0f, floorY,  7.0f), 2.6f, 0.0f), {glm::vec3(0.70f,0.30f,0.50f), 0.0f, 0.65f}});
    sceneProps.push_back({&coralPiece,  placeProp(coralPiece,  glm::vec3(-6.0f, floorY, 10.0f), 2.2f, 2.4f), {glm::vec3(0.65f,0.50f,0.70f), 0.0f, 0.6f}});
    sceneProps.push_back({&coralPiece,  placeProp(coralPiece,  glm::vec3( 9.0f, floorY, -6.0f), 3.0f, 0.5f), {glm::vec3(0.80f,0.35f,0.45f), 0.0f, 0.7f}});
    sceneProps.push_back({&crescentCoral, placeProp(crescentCoral, glm::vec3(10.0f, floorY, -3.0f), 4.2f, 0.0f), {glm::vec3(0.95f,0.80f,0.30f), 0.0f, 0.55f}});
    sceneProps.push_back({&crescentCoral, placeProp(crescentCoral, glm::vec3(-10.0f,floorY,  4.0f), 3.6f, 3.0f), {glm::vec3(0.90f,0.70f,0.25f), 0.0f, 0.5f}});
    sceneProps.push_back({&coralFish,   placeProp(coralFish,   glm::vec3( 7.0f, floorY, -8.0f), 3.4f, 0.0f), {glm::vec3(0.90f,0.45f,0.25f), 0.0f, 0.5f}});

    // === PORSCHE 911 zatopione === targetSize = dlugosc auta w jedn. swiata.
    // Najpierw sadzimy auto na dnie (placeProp), potem przechylamy je na bok o -8st
    // WOKOL punktu kontaktu z piaskiem - jeden bok lekko wbity w dno, jak wrak.
    {
        glm::vec3 porschePos(15.0f, floorY, -10.0f);
        glm::mat4 porscheM = glm::translate(porschePos)
            * glm::rotate(glm::radians(-8.0f), glm::vec3(0,0,1))
            * glm::translate(-porschePos)
            * placeProp(porscheModel, porschePos, 4.6f, glm::radians(35.0f));
        sceneProps.push_back({&porscheModel, porscheM,
            {glm::vec3(0.72f,0.07f,0.07f), 0.9f, 0.30f}});  // Guards Red, metallic
    }
    std::cout << "[PROPS] Zaladowano " << sceneProps.size() << " dekoracji (floorY=0)" << std::endl;

    // Diagnostyka: wypisz AABB kazdego modelu po PreTransformVertices (do strojenia skal).
    auto logAABB = [](const char* name, const MultiMeshModel& m) {
        glm::vec3 mn, mx; modelAABB(m, mn, mx); glm::vec3 sz = mx - mn;
        std::cout << "[AABB] " << name << " size=(" << sz.x << " x " << sz.y << " x " << sz.z
                  << ")  Y=[" << mn.y << ", " << mx.y << "]" << std::endl;
    };
    logAABB("coral.glb", coralModel1);
    logAABB("Coral 3D Model.glb", coralModel2);
    logAABB("Coral Piece.glb", coralPiece);
    logAABB("Crescent Moon Coral.glb", crescentCoral);
    logAABB("coral_fish.glb", coralFish);
    logAABB("Deep Sea Fish 3D.glb", deepSeaFish);
    logAABB("Guppy Fish.glb", guppyFish);
    logAABB("Shiny Fish.glb", shinyFish);
    logAABB("Porsche.glb", porscheModel);

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

    // --- Trzecia sciezka (Sketchfab fish) ---
    fishSplineC.addControlPoint(glm::vec3(-4.0f, 1.5f,  5.0f));
    fishSplineC.addControlPoint(glm::vec3( 3.0f, 2.5f,  8.0f));
    fishSplineC.addControlPoint(glm::vec3( 8.0f, 1.0f,  2.0f));
    fishSplineC.addControlPoint(glm::vec3( 4.0f, 3.0f, -5.0f));
    fishSplineC.addControlPoint(glm::vec3(-3.0f, 2.0f, -7.0f));
    fishSplineC.addControlPoint(glm::vec3(-8.0f, 1.5f, -2.0f));
    fishSplineC.buildFrames(128);

    // --- Ryby Sketchfab na splajnach ---
    // FishAnimation(path, speed, tOffset, scale=1, swimPhase) - skala/orientacja idzie
    // do localFix (fishLocalFix): auto-skala do targetSize + centrowanie + wyrownanie
    // dlugiej osi ciala do kierunku plyniecia. Rysowane: frameMatrix() * localFix.
    sketchFish.clear();
    // headingDeg: obrot wyrownujacy nos do kierunku plyniecia. Wszystkie modele
    // okazaly sie odwrocone tylem -> 180. Jesli ktoras ryba dalej plynie tylem,
    // zmien jej headingDeg o 180 (albo +-90 jesli plynie bokiem).
    auto addSketchFish = [&](Spline* path, float speed, float tOff, MultiMeshModel* model,
                             float targetSize, float headingDeg, PBRMaterial mat) {
        SketchFishInstance sf{ FishAnimation(path, speed, tOff, 1.0f, tOff * 6.2831853f), model, mat,
                               fishLocalFix(*model, targetSize, headingDeg), targetSize };
        sketchFish.push_back(sf);
    };
    // targetSize = dlugosc ryby w jedn. swiata; predkosci podbite, by ruch byl wyrazny.
    addSketchFish(&debugSpline, 0.060f, 0.15f, &deepSeaFish, 1.8f, 180.0f, {glm::vec3(0.35f,0.55f,0.80f), 0.1f, 0.4f});
    addSketchFish(&fishSplineB, 0.075f, 0.40f, &guppyFish,   1.0f, 180.0f, {glm::vec3(0.90f,0.65f,0.20f), 0.1f, 0.35f});
    addSketchFish(&fishSplineC, 0.055f, 0.60f, &shinyFish,   1.4f, 180.0f, {glm::vec3(0.40f,0.75f,0.90f), 0.3f, 0.25f});
    addSketchFish(&fishSplineC, 0.080f, 0.20f, &deepSeaFish, 1.6f, 180.0f, {glm::vec3(0.25f,0.45f,0.70f), 0.1f, 0.45f});
    addSketchFish(&debugSpline, 0.090f, 0.80f, &guppyFish,   0.9f, 180.0f, {glm::vec3(0.85f,0.55f,0.15f), 0.1f, 0.3f});
    std::cout << "[SKETCH] ryb Sketchfab na splajnach: " << sketchFish.size() << "\n";

    // --- Nurek (animacja szkieletowa) ---
    // ETAP 1: nurek plywa W MIEJSCU w widocznym punkcie, by zweryfikowac skinning.
    if (diver.load("./models/scene.gltf"))
    {
        glm::vec3 sz = diver.aabbMax - diver.aabbMin;
        float bindH = glm::max(sz.y, 0.0001f);
        float targetHeight = 2.4f;                       // wysokosc nurka w jedn. swiata
        diverBaseScale = targetHeight / bindH;
        diverCenter    = 0.5f * (diver.aabbMin + diver.aabbMax);
        diverBones.reserve(diver.numBones());
        std::cout << "[DIVER] baseScale=" << diverBaseScale << " (bindH=" << bindH << ")" << std::endl;
    }

    int w, h; glfwGetFramebufferSize(window, &w, &h);
    framebuffer_size_callback(window, w, h);
}

inline void shutdown(GLFWwindow* window)
{
    shaderLoader.DeleteProgram(programPBR);
    shaderLoader.DeleteProgram(programSkybox);
    shaderLoader.DeleteProgram(programDebugLine);
    shaderLoader.DeleteProgram(programFish);
    shaderLoader.DeleteProgram(programSketchFish);
    shaderLoader.DeleteProgram(programSkinned);
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

        if (showPanel) {
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
        ImGui::Text("Dekoracje Sketchfab");
        ImGui::Checkbox("Pokaz dekoracje (korale/ryby/Porsche)", &showProps);
        ImGui::Text("Props: %d", (int)sceneProps.size());
        ImGui::Separator();
        ImGui::Text("NUREK (animacja szkieletowa)");
        if (diver.valid())
        {
            ImGui::Checkbox("Pokaz nurka", &showDiver);
            ImGui::Text("Meshy: %d  Kosci: %d  Klipy: %d",
                        (int)diver.meshes().size(), diver.numBones(), diver.numClips());
            ImGui::SliderInt("Klip animacji", &diverClip, 0, glm::max(0, diver.numClips() - 1));
            ImGui::SliderFloat("Tempo animacji", &diverAnimSpeed, 0.0f, 3.0f);
            ImGui::SliderFloat3("Pozycja nurka", (float*)&diverPos, -20.0f, 20.0f);
            ImGui::SliderFloat3("Obrot (pitch/yaw/roll)", (float*)&diverRotDeg, -180.0f, 180.0f);
            ImGui::SliderFloat("Skala x", &diverScaleMul, 0.1f, 4.0f);
        }
        else ImGui::TextDisabled("(nie zaladowano models/scene.gltf)");
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
        }

        updateCursorMode(window);

        renderScene(window);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
