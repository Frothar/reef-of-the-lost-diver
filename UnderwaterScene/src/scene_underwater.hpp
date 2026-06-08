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
#include "backends/imgui_impl_glfw.h"
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
namespace {
    Core::Shader_Loader shaderLoader;

    GLuint programObject = 0;
    GLuint programSkybox = 0;
    GLuint programDebugLine = 0; // NED-01 podglad splajnu

    Core::RenderContext sphereContext;
    Core::RenderContext groundContext; // re-uses the cube model, scaled flat

    GLuint skyboxVAO = 0, skyboxVBO = 0;
    GLuint skyboxCubemap = 0;

    // --- Splajn (NED-01) -----------------------------------------------------
    Spline debugSpline;
    GLuint splineVAO = 0, splineVBO = 0;
    int    splineVertexCount = 0;
    bool   showSpline = true;

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
inline void drawObject(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color)
{
    glUseProgram(programObject);
    glm::mat4 viewProjection = createPerspectiveMatrix() * createCameraMatrix();
    glm::mat4 transformation = viewProjection * modelMatrix;

    glUniformMatrix4fv(glGetUniformLocation(programObject, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(programObject, "modelMatrix"),    1, GL_FALSE, (float*)&modelMatrix);
    glUniform3fv(glGetUniformLocation(programObject, "cameraPos"),   1, (float*)&camera.position);
    glUniform3fv(glGetUniformLocation(programObject, "lightDir"),    1, (float*)&sunDir);
    glUniform3fv(glGetUniformLocation(programObject, "lightColor"),  1, (float*)&sunColor);
    glUniform3fv(glGetUniformLocation(programObject, "objectColor"), 1, (float*)&color);
    glUniform3fv(glGetUniformLocation(programObject, "fogColor"),    1, (float*)&waterColor);
    glUniform1f(glGetUniformLocation(programObject, "fogDensity"), fogDensity);

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

inline void drawSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(programSkybox);

    // Strip translation from the view matrix so the skybox stays centred on the camera.
    glm::mat4 view = glm::mat4(glm::mat3(createCameraMatrix()));
    glm::mat4 viewProjection = createPerspectiveMatrix() * view;
    glUniformMatrix4fv(glGetUniformLocation(programSkybox, "viewProjection"), 1, GL_FALSE, (float*)&viewProjection);
    glUniform3fv(glGetUniformLocation(programSkybox, "waterTint"), 1, (float*)&waterColor);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glUniform1i(glGetUniformLocation(programSkybox, "skybox"), 0);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glUseProgram(0);
    glDepthFunc(GL_LESS);
}

inline void renderScene(GLFWwindow* window)
{
    glClearColor(waterColor.r * 0.6f, waterColor.g * 0.7f, waterColor.b * 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float time = (float)glfwGetTime();

    // Sandy seabed (flat scaled cube)
    drawObject(groundContext,
        glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)) * glm::scale(glm::vec3(40.0f, 0.2f, 40.0f)),
        glm::vec3(0.55f, 0.48f, 0.35f));

    // A slowly bobbing test sphere
    drawObject(sphereContext,
        glm::translate(glm::vec3(0.0f, 0.3f * std::sin(time), 0.0f)) * glm::scale(glm::vec3(1.0f)),
        glm::vec3(0.9f, 0.5f, 0.4f));

    // Splajn (NED-01) - podglad sciezki dla ryb
    drawSpline();

    // Skybox last (depth trick keeps it behind everything)
    drawSkybox();
}

// ---------------------------------------------------------------------------
// Model loading
// ---------------------------------------------------------------------------
inline void loadModelToContext(std::string path, Core::RenderContext& context)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    context.initFromAssimpMesh(scene->mMeshes[0]);
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
    if (!mouseLook) { firstMouse = true; return; }
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }

    float dx = ((float)xpos - lastX) * mouseSensitivity; // prawo dodatnie
    float dy = (lastY - (float)ypos) * mouseSensitivity; // gora dodatnia
    lastX = (float)xpos; lastY = (float)ypos;

    // yaw w prawo gdy mysz w prawo, pitch w gore gdy mysz w gore
    camera.addYawPitch(-dx, dy);
}

inline void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Tab toggles mouse-look so you can interact with the ImGui panel.
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        mouseLook = !mouseLook;
        glfwSetInputMode(window, GLFW_CURSOR, mouseLook ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
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

    programObject = shaderLoader.CreateProgram(
        (char*)"shaders/underwater.vert", (char*)"shaders/underwater.frag");
    programSkybox = shaderLoader.CreateProgram(
        (char*)"shaders/skybox.vert", (char*)"shaders/skybox.frag");
    programDebugLine = shaderLoader.CreateProgram(
        (char*)"shaders/debug_line.vert", (char*)"shaders/debug_line.frag");

    loadModelToContext("./models/sphere.obj", sphereContext);
    loadModelToContext("./models/cube.obj",   groundContext);

    // Skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
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

    int w, h; glfwGetFramebufferSize(window, &w, &h);
    framebuffer_size_callback(window, w, h);
}

inline void shutdown(GLFWwindow* window)
{
    shaderLoader.DeleteProgram(programObject);
    shaderLoader.DeleteProgram(programSkybox);
    shaderLoader.DeleteProgram(programDebugLine);
    glDeleteVertexArrays(1, &splineVAO);
    glDeleteBuffers(1, &splineVBO);
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
        ImGui::Text("TAB: mysz <-> panel");
        ImGui::Separator();
        ImGui::Text("Kamera: %6.1f %6.1f %6.1f", camera.position.x, camera.position.y, camera.position.z);
        ImGui::PushItemWidth(160.0f);
        ImGui::SliderFloat("Czulosc myszy", &mouseSensitivity, 0.02f, 0.4f);
        ImGui::Separator();
        ImGui::Checkbox("Pokaz splajn (NED-01)", &showSpline);
        ImGui::Separator();
        ImGui::SliderFloat("Fog density", &fogDensity, 0.0f, 0.15f);
        ImGui::ColorEdit3("Water color", (float*)&waterColor);
        ImGui::ColorEdit3("Sun color",   (float*)&sunColor);
        ImGui::SliderFloat3("Sun dir",   (float*)&sunDir, -1.0f, 1.0f);
        ImGui::PopItemWidth();
        ImGui::End();

        renderScene(window);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
