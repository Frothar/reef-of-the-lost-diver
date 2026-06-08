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
#include "Spline.h"               // NED-01 splajn Catmull-Rom

#include <vector>

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

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
struct PBRMaterial
{
    glm::vec3 albedo    = glm::vec3(0.8f);
    float     metallic  = 0.0f;
    float     roughness = 0.5f;
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

namespace {
    Core::Shader_Loader shaderLoader;

    GLuint programPBR    = 0;
    GLuint programSkybox = 0;
    GLuint programDebugLine = 0; // NED-01 podglad splajnu
    GLuint programFish   = 0;    // NED-03 pływanie ryb (A10)
    GLuint programWaterOverlay = 0; // pelnoekranowy efekt wody

    Core::RenderContext sphereContext;
    Core::RenderContext cubeContext;
    Core::RenderContext groundContext; // re-uses the cube model, scaled flat
    Core::RenderContext fishContext;   // NED-03/ALL-01 model ryby (models/fish.obj)

    GLuint skyboxVAO = 0, skyboxVBO = 0;
    GLuint skyboxCubemap = 0;

    // --- Splajn (NED-01) -----------------------------------------------------
    Spline debugSpline;
    GLuint splineVAO = 0, splineVBO = 0;
    int    splineVertexCount = 0;
    bool   showSpline = true;

    // --- PTF debug (NED-02) --------------------------------------------------
    // Kolorowe osie (T=czerwona, N=zielona, B=niebieska) w co ktorej ramce PTF.
    GLuint ptfAxesVAO = 0, ptfAxesVBO = 0;
    int    ptfAxesVertexCount = 0;
    bool   showPTF = true;

    // --- Pelnoekranowy efekt wody --------------------------------------------
    GLuint waterQuadVAO = 0, waterQuadVBO = 0;
    bool   showWaterOverlay = true;
    float  waterOverlayStrength = 0.7f;

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

    // NED-03 (A10): animacja ryb (domyslne dostrojone do modelu models/fish.obj)
    FishParams  fishParams;
    PBRMaterial fishMaterial = { glm::vec3(0.30f, 0.55f, 0.75f), 0.1f, 0.45f }; // srebrzysto-niebieska
    bool        showFish = true;
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
// Drawing
// ---------------------------------------------------------------------------
inline void drawPBRObject(Core::RenderContext& context, glm::mat4 modelMatrix, const PBRMaterial& material)
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

    glUniform3fv(glGetUniformLocation(programPBR, "albedo"), 1, (float*)&material.albedo);
    glUniform1f(glGetUniformLocation(programPBR, "metallic"),  material.metallic);
    glUniform1f(glGetUniformLocation(programPBR, "roughness"), material.roughness);

    glUniform3fv(glGetUniformLocation(programPBR, "fogColor"),  1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programPBR, "fogDensity"), fogDensity);

    Core::DrawContext(context);
    glUseProgram(0);
}

// NED-03 (A10): rysuje rybe z deformacja plywania w fish.vert.
// phaseOffset pozwala desynchronizowac kilka ryb (rozne fazy machania ogonem).
inline void drawFish(Core::RenderContext& context, glm::mat4 modelMatrix,
                     const PBRMaterial& material, float time, float phaseOffset)
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

    glUniform3fv(glGetUniformLocation(programFish, "albedo"), 1, (float*)&material.albedo);
    glUniform1f(glGetUniformLocation(programFish, "metallic"),  material.metallic);
    glUniform1f(glGetUniformLocation(programFish, "roughness"), material.roughness);

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

// Podglad splajnu jako linia (NED-01) - sluzy do wizualnej kontroli gladkosci.
inline void drawSpline()
{
    if (!showSpline || splineVertexCount < 2) return;

    glUseProgram(programDebugLine);
    glm::mat4 viewProjection = createPerspectiveMatrix() * createCameraMatrix();
    glm::vec3 lineColor = glm::vec3(1.0f, 0.85f, 0.2f); // zolta linia, dobrze widoczna pod woda

    glUniformMatrix4fv(glGetUniformLocation(programDebugLine, "viewProjection"), 1, GL_FALSE, (float*)&viewProjection);
    glUniform3fv(glGetUniformLocation(programDebugLine, "lineColor"), 1, (float*)&lineColor);

    glBindVertexArray(splineVAO);
    glDrawArrays(GL_LINE_STRIP, 0, splineVertexCount);
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

inline void renderScene(GLFWwindow* window)
{
    glClearColor(waterColor.r * 0.6f, waterColor.g * 0.7f, waterColor.b * 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float time = (float)glfwGetTime();

    // Sandy seabed (flat scaled cube, matte dielectric)
    drawPBRObject(groundContext,
        glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)) * glm::scale(glm::vec3(40.0f, 0.2f, 40.0f)),
        groundMaterial);

    // Dielectric test sphere (~2 m, metallic=0, roughness tweakable in ImGui)
    drawPBRObject(sphereContext,
        glm::translate(glm::vec3(-1.5f, 1.0f + 0.15f * std::sin(time), 0.0f)) *
            glm::scale(glm::vec3(1.0f)),
        sphereMaterial);

    // Metallic test cube – models/cube.obj ma 20 jednostek boku; skalujemy do ~2 m
    const float metalCubeScale = 0.1f;
    drawPBRObject(cubeContext,
        glm::translate(glm::vec3(2.0f, 1.0f + 0.1f * std::sin(time * 0.7f), 0.0f)) *
            glm::rotate((float)time * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::vec3(metalCubeScale)),
        metalMaterial);

    // --- Ryby (NED-03, A10): falowanie ciala w fish.vert ---
    // Placeholder ciala: sphere.obj rozciagnieta wzdluz Z (elipsoida ~ ryba).
    // Glowa przy -Z (stabilna), ogon przy +Z (macha). NED-04 wpusci je na splajn z PTF.
    if (showFish)
    {
        // fish.obj ma juz proporcje ryby (glowa -Z, ogon +Z) - skalujemy JEDNOLICIE,
        // zeby nie zniekształcić ksztaltu. Fallback na kule jesli model sie nie wczytal.
        Core::RenderContext& fishMesh = (fishContext.vertexArray != 0) ? fishContext : sphereContext;

        glm::vec3 fishPositions[3] = {
            glm::vec3(-3.0f, 1.6f,  1.0f),
            glm::vec3( 0.5f, 2.2f, -1.5f),
            glm::vec3( 3.5f, 0.9f,  2.0f),
        };
        float fishPhase[3] = { 0.0f, 1.7f, 3.4f };
        float fishYaw[3]   = { 0.4f, -0.8f, 2.1f };
        float fishScale    = 1.6f; // jednolita skala

        // Zywe, tropikalne kolory - zeby ryby odcinaly sie od koloru wody.
        glm::vec3 fishColors[3] = {
            glm::vec3(0.95f, 0.45f, 0.10f), // pomaranczowa (jak blazenek)
            glm::vec3(0.95f, 0.80f, 0.18f), // zolta
            glm::vec3(0.85f, 0.22f, 0.40f), // czerwono-rozowa
        };

        for (int i = 0; i < 3; ++i)
        {
            PBRMaterial mat = fishMaterial;
            mat.albedo = fishColors[i];

            glm::mat4 m = glm::translate(fishPositions[i] + glm::vec3(0.0f, 0.15f * std::sin(time + fishPhase[i]), 0.0f))
                        * glm::rotate(fishYaw[i], glm::vec3(0.0f, 1.0f, 0.0f))
                        * glm::scale(glm::vec3(fishScale));
            drawFish(fishMesh, m, mat, time, fishPhase[i]);
        }
    }

    // Splajn (NED-01) - podglad sciezki dla ryb
    drawSpline();

    // Osie PTF (NED-02) - weryfikacja stabilnosci ramek
    drawPTFAxes();

    // Skybox last (depth trick keeps it behind everything)
    drawSkybox(time);

    // Pelnoekranowy efekt wody na samym wierzchu (po skyboxie i geometrii)
    drawWaterOverlay(time);
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
// Input callbacks
// ---------------------------------------------------------------------------
inline void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    aspectRatio = height > 0 ? width / (float)height : 1.0f;
    glViewport(0, 0, width, height);
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
}

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------
inline void init(GLFWwindow* window)
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    programPBR = shaderLoader.CreateProgram(
        (char*)"shaders/pbr.vert", (char*)"shaders/pbr.frag");
    programSkybox = shaderLoader.CreateProgram(
        (char*)"shaders/skybox.vert", (char*)"shaders/skybox.frag");
    programDebugLine = shaderLoader.CreateProgram(
        (char*)"shaders/debug_line.vert", (char*)"shaders/debug_line.frag");
    // NED-03: fish.vert deformuje, oswietlenie wspolne z pbr.frag (spojnosc z OLE-01)
    programFish = shaderLoader.CreateProgram(
        (char*)"shaders/fish.vert", (char*)"shaders/pbr.frag");
    programWaterOverlay = shaderLoader.CreateProgram(
        (char*)"shaders/water_overlay.vert", (char*)"shaders/water_overlay.frag");

    if (!loadModelToContext("./models/sphere.obj", sphereContext))
        std::cout << "Brak models/sphere.obj – dodaj model kuli do folderu models/" << std::endl;
    if (!loadModelToContext("./models/cube.obj", cubeContext))
        std::cout << "Brak models/cube.obj – metal cube nie bedzie widoczny" << std::endl;
    if (!loadModelToContext("./models/cube.obj", groundContext))
        std::cout << "Brak models/cube.obj – dno nie bedzie widoczne" << std::endl;
    if (!loadModelToContext("./models/fish.obj", fishContext))
        std::cout << "Brak models/fish.obj – ryby beda uzywac kuli jako placeholdera" << std::endl;

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

    int w, h; glfwGetFramebufferSize(window, &w, &h);
    framebuffer_size_callback(window, w, h);
}

inline void shutdown(GLFWwindow* window)
{
    shaderLoader.DeleteProgram(programPBR);
    shaderLoader.DeleteProgram(programSkybox);
    shaderLoader.DeleteProgram(programDebugLine);
    shaderLoader.DeleteProgram(programFish);
    shaderLoader.DeleteProgram(programWaterOverlay);
    glDeleteVertexArrays(1, &splineVAO);
    glDeleteBuffers(1, &splineVBO);
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
        ImGui::Separator();
        ImGui::Checkbox("Efekt wody (overlay)", &showWaterOverlay);
        ImGui::SliderFloat("Sila efektu wody", &waterOverlayStrength, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::SliderFloat("Fog density", &fogDensity, 0.0f, 0.15f);
        ImGui::ColorEdit3("Water color", (float*)&waterColor);
        ImGui::ColorEdit3("Sun color",   (float*)&sunColor);
        ImGui::SliderFloat3("Sun dir",   (float*)&sunDir, -1.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("OLE-01 PBR test (sphere)");
        ImGui::ColorEdit3("Albedo", (float*)&sphereMaterial.albedo);
        ImGui::SliderFloat("Metallic",  &sphereMaterial.metallic,  0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &sphereMaterial.roughness, 0.0f, 1.0f);
        ImGui::Text("Metal cube (po prawej od kuli): roughness %.2f", metalMaterial.roughness);
        ImGui::SliderFloat("Metal roughness", &metalMaterial.roughness, 0.0f, 1.0f);
        ImGui::TextDisabled("Material metalowy = kod PBR, nie osobny model");
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
