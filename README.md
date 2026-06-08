# 🌊 Reef of the Lost Diver — Interactive Underwater Scene

An interactive real-time 3D underwater scene built with **C++**, **OpenGL**, and **GLSL** for the Computer Graphics (GRK) course at Adam Mickiewicz University.

![Screenshot placeholder](screenshots/scene_overview.png)

---

## 👥 Group 13

| Member | Role |
|--------|------|
| **Mróz** | Scene Composition & Integration — quaternion camera, moving lights (B13), bubble particles, interactions, README, build system |
| **Nędzyński** | Animation, Physics & Environment — skeletal/swimming animation (A10), Parallel Transport Frames, splines, underwater skybox |
| **Olejnik** | Rendering Pipeline & Shaders — PBR lighting, normal mapping, shadow mapping, multi-light support |

---

## 🎯 Chosen Methods

| Category | ID | Method |
|----------|----|--------|
| **Advanced (A)** | A10 | Skeletal animation / vertex-shader swimming animation |
| **Simpler (B)** | B13 | Moving point lights / submarine headlights |

**Combination:** A10 + B13

---

## 📋 Methods & Progress

> Status of the required and chosen methods — update the checkboxes as you implement them.
> So far only the project environment (`MRZ-01`) is done; the methods below are the plan.
> Full task breakdown in [`TASKS.md`](TASKS.md).

### Mandatory Methods

| Status | Method | Owner | Description |
|--------|--------|-------|-------------|
| `[ ]` | **Normal mapping** | Olejnik | Tangent-space normal maps applied to seabed, shipwreck hull, and coral surfaces using TBN matrices for per-fragment detail |
| `[ ]` | **PBR lighting** | Olejnik | Cook-Torrance BRDF with metallic/roughness workflow, GGX distribution, Schlick Fresnel, and Smith geometry function |
| `[ ]` | **Quaternion camera control** | Mróz | Gimbal-lock-free camera with quaternion-based rotation, 6DOF underwater swimming movement |
| `[ ]` | **Shadow mapping** | Olejnik | Directional light shadow map with depth FBO (2048×2048), slope-scaled bias, and 3×3 PCF soft shadows |
| `[ ]` | **Parallel Transport Frames** | Nędzyński | Stable orientation frames along Catmull-Rom splines for twist-free fish path following |
| `[ ]` | **Underwater skybox/cubemap** | Nędzyński | 6-face cubemap with deep blue-green underwater atmosphere, rendered position-independently |

### `[ ]` Chosen Method A10 — Swimming Animation *(Nędzyński)*

Vertex-shader body undulation for fish and sea creatures. Sine-wave displacement increases from head to tail, with separate fin animation. Parameters (amplitude, frequency, speed) are controllable per species.

### `[ ]` Chosen Method B13 — Moving Lights *(Mróz)*

- **Diver headlamp** — user-controlled spotlight with toggleable on/off state, color cycling, and adjustable intensity
- **Bioluminescent organisms** — animated point lights with pulsing intensity and slow drift movement (soft blue, green, purple glow)

---

## 🏗️ Building the Project

### Prerequisites

- **Visual Studio 2022 or 2026** (Community is fine) with the **"Desktop development with C++"** workload
- An **OpenGL 4.1+** capable GPU (any modern GPU)
- Libraries used (already provided — see the note below):
  - [GLFW](https://www.glfw.org/) — windowing and input
  - [GLEW](http://glew.sourceforge.net/) — OpenGL extension loader (`glew.h` / `glew32.dll`)
  - [GLM](https://github.com/g-truc/glm) — mathematics (header-only, committed to the repo)
  - [Assimp](https://github.com/assimp/assimp) — model loading
  - [SOIL](https://github.com/littlewhite-tb/soil2) — texture loading

> **Note:** We build on top of the GRK course lab framework (see `Grafika komputerowa - projekt z zajęć/`, exercise `cw 7`), which provides `Shader_Loader` and the `Core`/`Render_Utils` helpers (`loadModelToContext`, `LoadTexture`, `LoadCubemap`, `SetActiveTexture`, etc.) built on **GLEW + SOIL**, not GLAD/stb_image. We extend this base with our own modules and ship it as a self-contained Visual Studio project under `UnderwaterScene/`. Shaders are `#version 410 core` (OpenGL 4.1).

### Build Instructions

The whole team builds in **Visual Studio on Windows**. The full step-by-step — including
restoring the prebuilt libraries and the toolset-retarget note — is in
**[`UnderwaterScene/BUILD.md`](UnderwaterScene/BUILD.md)**. In short:

1. Clone the repo:
   ```bash
   git clone https://github.com/Frothar/reef-of-the-lost-diver.git
   ```
2. Restore `UnderwaterScene/dependencies/` from the course framework's
   `cw 7/dependencies` — the heavy Windows libraries are git-ignored, while `glm` and
   `imgui` are already in the repo.
3. Open **`UnderwaterScene/UnderwaterScene.sln`**, select **`Debug | x86`**, press **F5**.

> Using Visual Studio 2022 and hit *"v145 build tools not found"*? Right-click the project →
> **Retarget Projects** → pick **v143**. See `BUILD.md` §3.

---

## 🎮 Controls

> ℹ️ **These are the target controls for the finished product.** The current starter scene
> uses a free-fly camera: `W A S D` to swim, `Space`/`Left Ctrl` up/down, `Left Shift` to
> boost, mouse to look, `Tab` to toggle the ImGui panel, `Esc` to quit. The interactions
> below (headlamp, scaring fish, bioluminescence) are added with tasks `MRZ-05`/`MRZ-06`.

### Camera Movement

| Key | Action |
|-----|--------|
| `W` | Move forward |
| `S` | Move backward |
| `A` | Strafe left |
| `D` | Strafe right |
| `Space` | Ascend |
| `Left Shift` | Descend |
| `Mouse` | Look around (pitch & yaw) |
| `Q` / `E` | Roll left / right |
| `Left Shift` (hold) | Boost swim speed |

### Scene Interactions

| Key | Action |
|-----|--------|
| `F` | Toggle diver's headlamp on/off |
| `C` | Cycle headlamp color (white → warm → cool blue) |
| `+` / `-` | Increase / decrease headlamp intensity |
| `Mouse scroll` | Adjust headlamp intensity (alternative) |
| `E` or `Left click` | Scare nearby fish (triggers flee behavior) |
| `B` | Toggle bioluminescent lights on/off |

---

## 🖼️ Screenshots

> *Screenshots will be added before the final submission.*

| View | Screenshot |
|------|------------|
| Scene overview | ![Overview](screenshots/scene_overview.png) |
| PBR materials & normal mapping | ![PBR](screenshots/pbr_materials.png) |
| Shadow mapping | ![Shadows](screenshots/shadows.png) |
| Fish swimming animation (A10) | ![Fish](screenshots/fish_animation.png) |
| Moving lights & headlamp (B13) | ![Lights](screenshots/moving_lights.png) |
| Underwater skybox | ![Skybox](screenshots/skybox.png) |

---

## 📁 Project Structure

```
Projekt - Grafika/
├── README.md  TASKS.md  TIMELINE.md      # planning docs
└── UnderwaterScene/                      # the Visual Studio project
    ├── UnderwaterScene.sln / .vcxproj    # VS solution (Win32, toolset v145)
    ├── CMakeLists.txt                    # optional macOS/Linux build (untested)
    ├── BUILD.md                          # build instructions
    ├── src/
    │   ├── main.cpp                      # entry point, window + GL context init
    │   ├── scene_underwater.hpp          # starter scene: camera, skybox, draw loop
    │   ├── Shader_Loader.{h,cpp}         # Core: shader loading & linking
    │   ├── Render_Utils.{h,cpp}          # Core: RenderContext, Assimp mesh loading
    │   ├── Texture.{h,cpp}               # Core: textures + cubemap (SOIL)
    │   ├── Camera.{h,cpp}                # Core: view / projection helpers
    │   └── SOIL/                         # image loading
    ├── shaders/                          # underwater.vert/frag, skybox.vert/frag (#version 410)
    ├── models/   textures/  (+ skybox/)  screenshots/
    └── dependencies/                     # glm, imgui (committed) + glew/glfw/assimp (restored locally)
```

> **Planned modules** (added during the project — see [`TASKS.md`](TASKS.md)):
> `Model`/`Mesh`, `Light`, `ShadowMap`, `Skybox`, `Spline` (+ PTF), `FishAnimation`,
> `ParticleSystem`, `Scene`, plus the `pbr`, `shadow_depth`, `fish` and `particle` shaders.

---

## 📚 Key References

- [LearnOpenGL](https://learnopengl.com) — PBR, Normal Mapping, Shadow Mapping, Cubemaps, Skeletal Animation
- [opengl-tutorial.org](https://www.opengl-tutorial.org) — Quaternions, Shadow Mapping, Normal Mapping
- [GPU Gems](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects) — Animation, Water Effects
- [giordi91 — Parallel Transport](https://giordi91.github.io/post/2018-31-07-parallel-transport/) — PTF implementation
- [GRK Course Page](https://wp.faculty.wmi.amu.edu.pl/GRK.html) — Lab framework and materials

---

## 📄 License

This project was created for educational purposes as part of the Computer Graphics (GRK) course, 2025/2026.
