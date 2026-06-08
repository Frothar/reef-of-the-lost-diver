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

## ✅ Implemented Methods

### Mandatory Methods

| Method | Description |
|--------|-------------|
| **Normal mapping** | Tangent-space normal maps applied to seabed, shipwreck hull, and coral surfaces using TBN matrices for per-fragment detail |
| **PBR lighting** | Cook-Torrance BRDF with metallic/roughness workflow, GGX distribution, Schlick Fresnel, and Smith geometry function |
| **Quaternion camera control** | Gimbal-lock-free camera with quaternion-based rotation, 6DOF underwater swimming movement |
| **Shadow mapping** | Directional light shadow map with depth FBO (2048×2048), slope-scaled bias, and 3×3 PCF soft shadows |
| **Parallel Transport Frames** | Stable orientation frames along Catmull-Rom splines for twist-free fish path following |
| **Underwater skybox/cubemap** | 6-face cubemap with deep blue-green underwater atmosphere, rendered position-independently |

### Chosen Method A10 — Swimming Animation

Vertex-shader body undulation for fish and sea creatures. Sine-wave displacement increases from head to tail, with separate fin animation. Parameters (amplitude, frequency, speed) are controllable per species.

### Chosen Method B13 — Moving Lights

- **Diver headlamp** — user-controlled spotlight with toggleable on/off state, color cycling, and adjustable intensity
- **Bioluminescent organisms** — animated point lights with pulsing intensity and slow drift movement (soft blue, green, purple glow)

---

## 🏗️ Building the Project

### Prerequisites

- C++17 compatible compiler (Clang, GCC, or MSVC)
- CMake 3.16+
- OpenGL 4.1+
- The following libraries (included in `lib/` or fetched via CMake):
  - [GLFW](https://www.glfw.org/) — windowing and input
  - [GLEW](http://glew.sourceforge.net/) — OpenGL extension loader (matches the course lab framework, `glew.h`/`glew32.dll`)
  - [GLM](https://github.com/g-truc/glm) — mathematics
  - [Assimp](https://github.com/assimp/assimp) — model loading
  - [SOIL](https://github.com/littlewhite-tb/soil2) — texture loading (matches the course lab framework's `Texture.cpp`)

> **Note:** We build on top of the GRK course lab framework (`grk-cw.sln`, see `Grafika komputerowa - projekt z zajęć/`), which already provides `Shader_Loader` and the `Core`/`Render_Utils` helpers (`initVAO`, `loadModelToContext`, `LoadTexture`, `SetActiveTexture`, etc.) built on **GLEW + SOIL**, not GLAD/stb_image. We extend this base with our own `Model`/`Mesh`/`Texture` classes and migrate the build from the provided Visual Studio solution to CMake for cross-platform development — a deliberate departure from "use the framework as-is," not an oversight.

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/<your-repo-url>.git
cd Projekt_grafika_komputerowa

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)    # Linux/macOS
# or
cmake --build . --config Release   # Cross-platform

# Run the application
./UnderwaterScene
```

### macOS Notes

On macOS, OpenGL is deprecated but still functional. You may need to set:
```bash
export MACOSX_DEPLOYMENT_TARGET=10.15
```

---

## 🎮 Controls

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
Projekt_grafika_komputerowa/
├── README.md
├── CMakeLists.txt
├── src/
│   ├── main.cpp                    # Entry point, main loop
│   ├── Camera.h / Camera.cpp       # Quaternion camera
│   ├── Shader.h / Shader.cpp       # Shader loading & uniforms
│   ├── Model.h / Model.cpp         # Model loading (Assimp)
│   ├── Mesh.h / Mesh.cpp           # Mesh data (VAO, VBO, EBO)
│   ├── Texture.h / Texture.cpp     # Texture & cubemap loading
│   ├── Light.h / Light.cpp         # Light management
│   ├── ShadowMap.h / ShadowMap.cpp # Shadow FBO & rendering
│   ├── Skybox.h / Skybox.cpp       # Cubemap skybox
│   ├── Spline.h / Spline.cpp       # Catmull-Rom splines + PTF
│   ├── FishAnimation.h / .cpp      # Swimming animation
│   ├── ParticleSystem.h / .cpp     # Bubble particles
│   └── Scene.h / Scene.cpp         # Scene graph & objects
├── shaders/
│   ├── pbr.vert / pbr.frag         # PBR + normal mapping
│   ├── shadow_depth.vert / .frag   # Shadow pass
│   ├── skybox.vert / skybox.frag   # Skybox rendering
│   ├── fish.vert / fish.frag       # Fish swimming animation
│   └── particle.vert / .frag       # Bubble particles
├── models/                          # 3D models (.obj, .fbx, .gltf)
├── textures/                        # Albedo, normal, metallic, roughness, AO
│   └── skybox/                      # 6 cubemap faces
├── screenshots/                     # Screenshots for README
└── lib/                             # External dependencies
```

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
