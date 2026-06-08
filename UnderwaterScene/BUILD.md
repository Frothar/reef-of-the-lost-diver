# Building — Reef of the Lost Diver

This project builds on **Windows** (Visual Studio) and **macOS / Linux** (CMake).
It targets **OpenGL 4.1 core / GLSL 410** so the same code and shaders run on every
platform (Apple caps OpenGL at 4.1).

---

## Windows — Visual Studio (recommended for Windows users)

Everything is preconfigured. The prebuilt libraries live in `dependencies/`
(GLFW, GLEW, Assimp) and the required runtime DLLs sit in the project root.

1. Open **`UnderwaterScene.sln`** in Visual Studio 2022/2026.
2. Select the **`Debug | x86`** (Win32) configuration. *(The bundled libraries are
   32-bit — do not switch to x64.)*
3. Press **F5** (Build & Run).

The debugger working directory is set to the project folder, so the relative asset
paths (`shaders/`, `models/`, `textures/`) resolve correctly. A post-build step copies
the runtime DLLs next to the executable.

> **Fresh clone?** The heavy prebuilt libraries (`dependencies/assimp`, `glew-2.0.0`,
> `glfw-3.3.8.bin.WIN32`) are git-ignored. Copy the `dependencies/` folder from the
> GRK course framework (`Grafika komputerowa - projekt z zajęć/grk/cw 7/dependencies`)
> into this project, then build.

---

## macOS — CMake + Homebrew

```bash
brew install cmake glfw glew assimp

cd UnderwaterScene
cmake -S . -B build
cmake --build build
./UnderwaterScene          # run from the project root (assets load relatively)
```

If OpenGL-deprecation warnings appear, they are harmless — OpenGL 4.1 is fully
functional on macOS. Apple Silicon (M1/M2/M3) and Intel both work.

---

## Linux — CMake

```bash
sudo apt install cmake libglfw3-dev libglew-dev libassimp-dev libglm-dev
cd UnderwaterScene
cmake -S . -B build
cmake --build build
./UnderwaterScene
```

---

## Controls (starter scene)

| Input | Action |
|-------|--------|
| `W A S D` | Swim forward / left / back / right |
| `Space` / `Left Ctrl` | Ascend / descend |
| `Left Shift` (hold) | Boost speed |
| Mouse | Look around |
| `Tab` | Toggle mouse-look ↔ ImGui panel |
| `Esc` | Quit |

The free-fly camera here is a placeholder — replace it with the quaternion camera
(task `MRZ-02`).

---

## What's in the starter

- Window + OpenGL 4.1 context, GLEW, GLFW, ImGui debug panel
- `Core` framework from the course labs: `Shader_Loader`, `Render_Utils`
  (`RenderContext`, `DrawContext`, Assimp mesh loading), `Texture`
  (`LoadTexture`, `LoadCubemap`, `SetActiveTexture`), `Camera`, SOIL
- Cubemap skybox + lit seabed + a test sphere, with exponential underwater fog
- Shaders in `#version 410 core`: `underwater.vert/frag`, `skybox.vert/frag`

From here, implement the modules listed in `../TASKS.md`.
