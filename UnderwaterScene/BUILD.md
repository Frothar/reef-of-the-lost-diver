# Building — Reef of the Lost Diver

The team works in **Visual Studio on Windows**. This is the supported build path.
The project targets **OpenGL 4.1 core / GLSL 410**.

---

## 1. First-time setup (every teammate, once)

### a) Install Visual Studio with the C++ workload

Install **Visual Studio 2022 or 2026** (Community is free) and, in the Visual Studio
Installer, tick the **"Desktop development with C++"** workload. Without it there is no
C++ compiler and nothing builds.

### b) Restore the prebuilt libraries

The heavy Windows libraries are **not** in the git repo (they are git-ignored). After
cloning you must put them back into `UnderwaterScene/dependencies/`:

> Copy the **`dependencies/`** folder from the GRK course framework
> (`Grafika komputerowa - projekt z zajęć/grk/cw 7/dependencies`) into
> `UnderwaterScene/` so the folder `UnderwaterScene/dependencies/` contains
> `assimp/`, `glew-2.0.0/`, `glfw-3.3.8.bin.WIN32/`, `glm/`, `imgui/`.

*(`glm/` and `imgui/` are already committed to the repo; the rest come from the course
framework.)*

The runtime DLLs (`glew32.dll`, `glfw3.dll`, `assimp-*.dll`, `zlib*.dll`) are committed in
the project root, and a post-build step copies them next to the executable automatically.

---

## 2. Build & run

1. Open **`UnderwaterScene.sln`** in Visual Studio.
2. Make sure the configuration is **`Debug | x86`** (the bundled libraries are 32-bit —
   **do not** switch to x64).
3. Press **F5** (build + run with debugger) or **Ctrl+F5** (run without debugger).

The debugger working directory is set to the project folder, so the relative asset paths
(`shaders/`, `models/`, `textures/`) resolve correctly.

---

## 3. Toolset mismatch (important for the team)

The project's platform toolset is set to **v145 (Visual Studio 2026)**. If you open it in
**Visual Studio 2022**, you will get an error like *"v145 build tools not found"*.

Fix it locally:

> Right-click the project in Solution Explorer → **Retarget Projects** → pick the toolset
> you have installed (e.g. **v143** for VS 2022) → OK.

To avoid churn, **agree as a team on one Visual Studio version**, or simply retarget
locally and don't commit that one-line change in `UnderwaterScene.vcxproj`.

---

## 4. Troubleshooting

| Symptom | Fix |
|---------|-----|
| `MSB8020: v145 build tools not found` | Retarget Projects (see §3) |
| `cannot open ... glew32.lib / glfw3.lib / assimp-*.lib` | `dependencies/` not restored — see §1b |
| App starts then closes, console shows `ERROR::ASSIMP` or cubemap load fail | Wrong working directory — run from Visual Studio (F5), not by double-clicking the exe in `Debug/` |
| Missing `.dll` popup when running the exe directly | Run via F5, or copy the root `*.dll` next to the exe |
| Black window | Check the console for shader-compile errors; GLSL must stay `#version 410` |

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

---

## Other platforms (optional, unsupported)

A `CMakeLists.txt` exists for macOS/Linux (`brew install glfw glew assimp` /
`apt install libglfw3-dev libglew-dev libassimp-dev`, then `cmake -S . -B build &&
cmake --build build`). The team builds on Windows/VS, so this path is not actively
tested — use it only if you specifically need a non-Windows build.
