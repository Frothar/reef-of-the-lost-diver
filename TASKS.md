# 📋 Task Board — GRK Underwater Scene (A10 + B13)

> Track progress by marking tasks: `[ ]` → `[/]` (in progress) → `[x]` (done)

---

## ⚖️ Workload Balance

| Person | Mandatory Methods | Chosen Methods | Effort Estimate | Tasks |
|--------|-------------------|----------------|-----------------|-------|
| **Olejnik** | M1 Normal mapping, M2 PBR, M4 Shadow mapping | — (shader support for all) | ~30h | 7 |
| **Nędzyński** | M5 PTF, M6 Skybox | A10 Fish animation (30 pts) | ~31h | 7 |
| **Mróz** | M3 Quaternion camera | B13 Moving lights (15 pts) | ~32h | 8 |
| **Shared** | — | — | ~8h | 5 |

> [!NOTE]
> Olejnik has 3 mandatory methods but they form one tightly coupled shader pipeline (PBR + normals + shadows = single system).
> Nędzyński owns A10 which alone is worth 30 pts (= all 6 mandatory methods combined).
> Mróz owns infrastructure + B13 + particles — essential glue that holds the project together.

---

## 🔴 Priority Legend

| Label | Meaning |
|-------|---------|
| 🔴 **P0** | Blocker — other tasks depend on this |
| 🟠 **P1** | Core — required for grading |
| 🟡 **P2** | Important — needed for visual coherence |
| 🟢 **P3** | Nice to have — bonus polish |

---

# 👤 Olejnik — Rendering Pipeline & Shaders

> **Focus:** PBR + normal mapping + shadow mapping + multi-light shader support
> **Mandatory methods:** M1 (Normal mapping), M2 (PBR lighting), M4 (Shadow mapping)
> **Estimated effort:** ~30 hours

---

### `OLE-01` 🔴 P0 · PBR Shader — Basic Setup (~6h)
**Week 1**

Write the base PBR fragment shader with the Cook-Torrance BRDF (hardcoded material params, single directional light).

**What to do:**
- [ ] Create `shaders/pbr.vert` — transform position, normal, texCoords to world space; pass TBN matrix
- [ ] Create `shaders/pbr.frag` — implement Cook-Torrance with GGX (D), Schlick (F), Smith (G)
- [ ] Support a single directional light (sun through water)
- [ ] Add Reinhard or ACES tone mapping + gamma correction
- [ ] Test with hardcoded albedo, metallic, roughness values on a simple mesh

**Acceptance criteria:**
- A sphere or cube renders with visually correct PBR shading (specular highlight changes with roughness)
- Metallic = 0 looks like plastic/stone, metallic = 1 looks like metal

**Dependencies:** Needs Mróz's window/context init (`MRZ-01`) and a loaded test model

**Resources:**
- [LearnOpenGL — PBR Theory](https://learnopengl.com/PBR/Theory)
- [LearnOpenGL — PBR Lighting](https://learnopengl.com/PBR/Lighting)

---

### `OLE-02` 🟠 P1 · PBR Shader — Texture Maps (~4h)
**Week 2**

Extend the PBR shader to read material properties from texture maps instead of uniforms.

**What to do:**
- [ ] Add sampler uniforms: `albedoMap`, `metallicMap`, `roughnessMap`, `aoMap`
- [ ] Add a `useTexture` flag (or bitmask) per map so objects without textures still work
- [ ] Implement material loading code in C++ (load textures, bind to correct units)
- [ ] Test with at least 2 different PBR texture sets (e.g. sand + rusty metal)

**Acceptance criteria:**
- Objects render correctly with full PBR texture sets from [ambientCG](https://ambientcg.com)
- Objects without texture maps fall back to uniform values

**Dependencies:** `OLE-01`

---

### `OLE-03` 🟠 P1 · Normal Mapping (~5h)
**Week 2** | **⭐ MANDATORY METHOD M1**

Implement tangent-space normal mapping integrated into the PBR shader.

**What to do:**
- [ ] Compute or load tangent + bitangent per vertex (Assimp can provide these)
- [ ] Build the TBN matrix in the vertex shader and pass to fragment shader
- [ ] In the fragment shader: sample normal map, remap from [0,1] to [-1,1], transform via TBN
- [ ] Use perturbed normal `N` in all PBR lighting calculations
- [ ] Apply on **at least 2 different materials** (sand seabed + shipwreck hull or coral)

**Acceptance criteria:**
- Surface shows visible bumps/detail that reacts to light direction
- Toggling the normal map off shows a clearly flatter surface
- No visible seam artifacts at UV boundaries

**Dependencies:** `OLE-02`

**Resources:**
- [LearnOpenGL — Normal Mapping](https://learnopengl.com/Advanced-Lighting/Normal-Mapping)

---

### `OLE-04` 🟠 P1 · Shadow Mapping (~8h)
**Week 2** | **⭐ MANDATORY METHOD M4**

Implement shadow mapping for the main directional light with PCF filtering.

**What to do:**
- [ ] Create a depth-only FBO (2048×2048 texture, `GL_DEPTH_COMPONENT`)
- [ ] Create `shaders/shadow_depth.vert` + `shaders/shadow_depth.frag` — minimal depth-write shader
- [ ] Set up the light's orthographic projection + view matrix
- [ ] **Shadow pass:** Render all shadow-casting geometry to the depth FBO
- [ ] **Main pass:** Transform fragment to light space, sample shadow map, compare depths
- [ ] Add slope-scaled bias: `bias = max(0.05 * (1.0 - dot(N, L)), 0.005)`
- [ ] Implement 3×3 PCF for soft shadow edges
- [ ] Integrate shadow factor into the PBR shader output

**Acceptance criteria:**
- The shipwreck (or large object) casts visible shadows onto the seabed
- No shadow acne (flickering dots on lit surfaces)
- Shadow edges are soft, not pixel-jagged

**Dependencies:** `OLE-01`, Mróz needs to have at least 2 objects in the scene

**Resources:**
- [LearnOpenGL — Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)

---

### `OLE-05` 🟠 P1 · Multi-Light Support in PBR Shader (~4h)
**Week 2–3**

Extend the PBR shader to handle multiple light types from Mróz's light system.

**What to do:**
- [ ] Add uniform arrays for `PointLight[MAX_POINT_LIGHTS]` and `SpotLight[MAX_SPOT_LIGHTS]`
- [ ] Implement `calcPointLight()` — PBR with point light attenuation
- [ ] Implement `calcSpotLight()` — PBR with spotlight cone + attenuation
- [ ] Accumulate radiance `Lo` across all active lights
- [ ] Coordinate with Mróz on the light struct layout and uniform names

**Acceptance criteria:**
- Scene correctly lit by directional sun + Mróz's moving point lights + spotlight
- Point lights have visible falloff with distance
- Spotlight has visible cone (inner/outer cutoff)

**Dependencies:** `OLE-01`, `MRZ-05` (light system)

---

### `OLE-06` 🟡 P2 · Shadow Artifact Debugging & Polish (~3h)
**Week 3**

Fix common shadow mapping issues.

**What to do:**
- [ ] Fix peter-panning (shadow detaches from object) — adjust bias
- [ ] Fix shadow map coverage (objects outside light frustum have no shadows) — adjust ortho size
- [ ] Optional: implement front-face culling during shadow pass to reduce acne
- [ ] Optional: add a second shadow map for the spotlight (Mróz's headlamp)

**Acceptance criteria:**
- Shadows are artifact-free from all common camera angles
- No light leaking through solid objects

**Dependencies:** `OLE-04`

---

### `OLE-07` 🟢 P3 · Post-Processing — Underwater Color Grading (~3h)
**Week 3–4**

Optional post-processing pass for underwater atmosphere.

**What to do:**
- [ ] Render scene to an intermediate FBO (color + depth)
- [ ] Apply a fullscreen quad shader with blue-green tint
- [ ] Add depth-based fog (exponential, blue-green color) in post or in the PBR shader
- [ ] Optional: slight chromatic aberration or lens distortion at edges

**Acceptance criteria:**
- Scene feels more underwater with color shift and distance fog
- Parameters are easily tweakable

**Dependencies:** All core rendering tasks done

---

# 👤 Nędzyński — Animation, Physics & Environment

> **Focus:** Fish animation (A10) + Parallel Transport Frames + Skybox + Jellyfish
> **Mandatory methods:** M5 (Parallel Transport Frames), M6 (Underwater Skybox/Cubemap)
> **Chosen method:** A10 — Skeletal/swimming animation (30 pts)
> **Estimated effort:** ~31 hours

---

### `NED-01` 🔴 P0 · Spline Path System (~5h)
**Week 1**

Implement a Catmull-Rom spline class for defining smooth paths in 3D.

**What to do:**
- [ ] Create `src/Spline.h` / `src/Spline.cpp`
- [ ] Implement `addControlPoint(glm::vec3)`, `evaluate(float t)`, `evaluateTangent(float t)`
- [ ] Use the Catmull-Rom formula for interpolation between control points
- [ ] Handle looping paths (connect last point back to first)
- [ ] Test: render the spline as a line strip to verify smoothness

**Acceptance criteria:**
- A path with 5+ control points produces a smooth, continuous curve
- `evaluate(0.0)` = first point, `evaluate(1.0)` = last point (or loop back)
- Tangent vectors are smooth and non-zero

**Dependencies:** None — can start immediately

**Resources:**
- [Catmull-Rom splines](https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline)

---

### `NED-02` 🔴 P0 · Parallel Transport Frames (~5h)
**Week 1** | **⭐ MANDATORY METHOD M5**

Compute twist-free orientation frames along the spline for fish orientation.

**What to do:**
- [ ] Implement PTF computation in `Spline.cpp` (or separate module)
- [ ] Sample spline at N points, compute tangent at each point
- [ ] Set initial frame: pick an arbitrary normal perpendicular to the first tangent
- [ ] For each subsequent point: rotate previous normal to align with new tangent using cross product + angle method
- [ ] Store frames as `(T, N, B)` triples or `glm::mat4` per sample
- [ ] Provide a method: `getFrame(float t) → (position, tangent, normal, bitangent)`

**Acceptance criteria:**
- An object following the spline maintains stable orientation (no random flipping or twisting)
- Works correctly on curves with varying curvature including straight sections
- Visually test by rendering small axes or a box at each frame

**Dependencies:** `NED-01`

**Resources:**
- [giordi91 — Parallel Transport](https://giordi91.github.io/post/2018-31-07-parallel-transport/)

---

### `NED-03` 🟠 P1 · Fish Swimming Animation — Vertex Shader (~7h)
**Week 2** | **⭐ THIS IS THE A10 METHOD (30 pts)**

Implement sine-wave body undulation in the vertex shader.

**What to do:**
- [ ] Create `shaders/fish.vert` — copy PBR vertex shader as base, add deformation
- [ ] Create `shaders/fish.frag` — same as `pbr.frag` (or import shared lighting code)
- [ ] Implement body wave: `deformed.x += sin(z * freq - time * speed) * amplitude * bodyFactor`
- [ ] `bodyFactor` = `smoothstep(0, 1, z / fishLength)` — wave increases toward tail
- [ ] Add fin animation: separate sine function for pectoral/dorsal fins
- [ ] Add uniforms: `time`, `waveAmplitude`, `waveFrequency`, `waveSpeed`, `fishLength`
- [ ] Recalculate normals after deformation (finite differences or analytical)

**Acceptance criteria:**
- Fish body visibly undulates like a swimming fish
- Head is relatively stable, tail swings the most
- Fins flap independently from the body
- Animation parameters are adjustable via uniforms
- Lighting is correct on the deformed surface (normals updated)

**Dependencies:** `OLE-01` (PBR shader to base the fish shader on)

**Resources:**
- [GPU Gems 1 — Animation in the Dawn Demo](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-4-animation-dawn-demo)

---

### `NED-04` 🟠 P1 · Fish Path Following with PTF (~4h)
**Week 2**

Move the animated fish along the spline using Parallel Transport Frames.

**What to do:**
- [ ] Create `src/FishAnimation.h` / `src/FishAnimation.cpp`
- [ ] Animate the `t` parameter along the spline over time (constant speed or eased)
- [ ] At each frame: get position + frame from PTF, build model matrix
- [ ] Model matrix: columns = (B, N, -T) from the frame + position for translation
- [ ] Combine PTF orientation with vertex-shader swimming deformation
- [ ] Support multiple fish on different paths (or same path with time offset)

**Acceptance criteria:**
- Fish smoothly follows the spline path
- Fish faces forward along the curve at all times (no snapping or flipping)
- Swimming animation plays on top of the path movement
- At least 2 fish visible on different paths

**Dependencies:** `NED-01`, `NED-02`, `NED-03`

---

### `NED-05` 🟠 P1 · Underwater Skybox / Cubemap (~4h)
**Week 1** | **⭐ MANDATORY METHOD M6**

Render a cubemap skybox as the underwater background.

**What to do:**
- [ ] Find or create 6 underwater cubemap face textures (deep blue-green tones)
- [ ] Load the cubemap using `GL_TEXTURE_CUBE_MAP` with all 6 faces
- [ ] Create `shaders/skybox.vert` + `shaders/skybox.frag`
- [ ] Vertex shader: strip translation from view matrix (`mat4(mat3(view))`), use `pos.xyww` depth trick
- [ ] Fragment shader: sample cubemap with direction vector
- [ ] Render with `GL_LEQUAL` depth func and depth write disabled
- [ ] Ensure skybox doesn't move with camera (only rotates)

**Acceptance criteria:**
- Background shows a convincing underwater environment in all directions
- Skybox stays fixed when camera translates (only rotates)
- Top face is brighter (sunlight), bottom face is darker (abyss)

**Dependencies:** `MRZ-01` (window/context)

**Resources:**
- [LearnOpenGL — Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)

---

### `NED-06` 🟡 P2 · Jellyfish Pulsing Animation (~3h)
**Week 3**

Vertex-shader pulsing for jellyfish models.

**What to do:**
- [ ] Add a pulsing deformation in the vertex shader (scale bell up/down with sine wave)
- [ ] Can reuse the fish shader with different parameters, or create a separate shader
- [ ] Sync pulsing with upward movement (pulse = propulsion)
- [ ] Add slight tentacle trailing motion (sine wave on lower vertices, like fish body but vertical)

**Acceptance criteria:**
- Jellyfish visibly pulsates its bell
- Movement looks organic and rhythmic
- Tentacles trail and sway

**Dependencies:** `NED-03` (reuse vertex deformation approach)

---

### `NED-07` 🟢 P3 · Multiple Fish Species (~3h)
**Week 3–4**

Create visual variety with different animation parameters.

**What to do:**
- [ ] Define 2–3 fish "profiles" with different wave amplitude, frequency, speed, and size
- [ ] Use different models or scaled versions of the same model
- [ ] Give each species different spline paths and speeds

**Acceptance criteria:**
- At least 2 visually distinct fish types swimming in the scene
- Different species have noticeably different swimming styles

**Dependencies:** `NED-03`, `NED-04`

---

# 👤 Mróz — Scene Composition, Camera & Lights

> **Focus:** Project setup + quaternion camera + moving lights (B13) + particles + scene + interactions
> **Mandatory methods:** M3 (Quaternion camera control)
> **Chosen method:** B13 — Moving point lights (15 pts)
> **Estimated effort:** ~32 hours

---

### `MRZ-01` ✅ P0 · Project Setup & Window Init (~4h) — **DONE**
**Week 1**

Set up the project from the course lab framework (`cw 7`: GLEW + SOIL + Core/Render_Utils/Shader_Loader/Texture/Camera) as a self-contained **Visual Studio** project under `UnderwaterScene/`, with the dependencies bundled.

**What was done:**
- [x] Visual Studio solution `UnderwaterScene.sln` / `.vcxproj` (Win32, toolset v145) with GLFW, GLEW, GLM, Assimp, SOIL, ImGui wired up
- [x] `src/main.cpp` — GLFW window, **OpenGL 4.1 core** context, GLEW init, main loop
- [x] `src/scene_underwater.hpp` — starter scene (free-fly camera, skybox, draw loop)
- [x] Shader loading via the Core `Shader_Loader` (no separate `Shader.h` needed)
- [x] `.gitignore` + `.gitattributes` (build dirs, heavy deps, line-ending normalization)
- [x] `CMakeLists.txt` for macOS/Linux (optional, untested — team is on Windows/VS)
- [x] Builds and runs (verified): window opens, console prints `OpenGL 4.1.0`

**Acceptance criteria:**
- [x] Building in Visual Studio (`Debug | x86`, F5) produces a running executable
- [x] Window opens with a cleared underwater color
- [x] Shader loading utility works (`underwater` + `skybox` programs compile and render)

**Dependencies:** None — **done first.** Everyone else builds on top of this. See `UnderwaterScene/BUILD.md`.

---

### `MRZ-02` 🔴 P0 · Quaternion Camera (~5h)
**Week 1** | **⭐ MANDATORY METHOD M3**

Implement quaternion-based camera with underwater swim controls.

**What to do:**
- [ ] Create `src/Camera.h` / `src/Camera.cpp`
- [ ] Store orientation as `glm::quat`, position as `glm::vec3`
- [ ] Mouse input → create pitch/yaw quaternions → compose with current orientation
- [ ] Extract forward/right/up vectors from quaternion rotation matrix
- [ ] Build view matrix: `glm::lookAt(position, position + forward, up)`
- [ ] WASD movement along local forward/right axes
- [ ] Space / LShift for ascend / descend
- [ ] Optional: Q/E for roll, slight camera sway

**Acceptance criteria:**
- Camera rotates smoothly in all directions without gimbal lock
- Looking straight up then rotating sideways doesn't cause snapping (proves no gimbal lock)
- Movement feels like underwater swimming (smooth, 6DOF)

**Dependencies:** `MRZ-01`

**Resources:**
- [opengl-tutorial.org — Quaternions](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/)

---

### `MRZ-03` 🔴 P0 · Model Loading (~6h)
**Week 1**

Load OBJ/glTF models with Assimp and render them.

**What to do:**
- [ ] Create `src/Model.h` / `src/Model.cpp` — load model via Assimp
- [ ] Create `src/Mesh.h` / `src/Mesh.cpp` — VAO/VBO/EBO wrapper
- [ ] Extract vertices (position, normal, texCoords, tangent, bitangent) and indices
- [ ] Load associated textures from model's material info
- [ ] Create `src/Texture.h` / `src/Texture.cpp` — load images with SOIL (matching the course framework's `Core::LoadTexture`), manage texture units
- [ ] Test: load and render a simple .obj model with texture

**Acceptance criteria:**
- A test model (e.g. a fish or cube) loads and displays correctly
- Textures are applied and visible
- Tangent/bitangent data is available for Olejnik's normal mapping

**Dependencies:** `MRZ-01`, `MRZ-02` (camera to see the model)

---

### `MRZ-04` 🟠 P1 · Scene Graph & Object Placement (~4h)
**Week 2–3**

Manage all scene objects with transforms and a structured scene graph.

**What to do:**
- [ ] Create `src/Scene.h` / `src/Scene.cpp`
- [ ] Store a list of scene objects with: model reference, position, rotation, scale
- [ ] Implement `Scene::render(Shader&)` — iterate objects, set model matrix, draw
- [ ] Support separate render calls for shadow pass vs. main pass
- [ ] Place scene elements:
  - [ ] Sandy seabed (flat plane or heightmap mesh)
  - [ ] Coral reef (3–5 coral models placed in a group)
  - [ ] Shipwreck (large model, shadow-casting)
  - [ ] Diver model (near camera or as separate entity)
- [ ] Ensure all models are at correct scale relative to each other

**Acceptance criteria:**
- Scene has a recognizable underwater composition when viewed from the camera
- Objects are at appropriate scale (fish are small, wreck is large)
- Shadow pass can reuse the scene's draw call with a different shader

**Dependencies:** `MRZ-03`, needs some models found/downloaded (shared task)

---

### `MRZ-05` 🟠 P1 · Moving Point Lights & Headlamp — B13 (~5h)
**Week 2** | **⭐ THIS IS THE B13 METHOD (15 pts)**

Implement the light management system with moving lights.

**What to do:**
- [ ] Create `src/Light.h` / `src/Light.cpp`
- [ ] Define structs: `DirLight`, `PointLight`, `SpotLight` (matching shader uniforms)
- [ ] **Diver headlamp (spotlight):**
  - [ ] Attach to camera position + forward direction
  - [ ] Toggle on/off with `F` key
  - [ ] Color cycle with `C` key (white → warm yellow → cool blue)
  - [ ] Intensity adjust with `+`/`-` or mouse scroll
- [ ] **Bioluminescent point lights (2–4):**
  - [ ] Animated position: slow drift with `sin(time)` offsets
  - [ ] Animated intensity: gentle pulsing with `sin(time * pulseSpeed)`
  - [ ] Colors: soft blue, green, purple
  - [ ] Toggle with `B` key
- [ ] Upload all light data to shader each frame via uniforms

**Acceptance criteria:**
- Headlamp illuminates objects in a visible cone that follows the camera
- Toggling headlamp on/off is clearly visible
- Bioluminescent lights glow and pulse, casting colored light on nearby objects
- At least one light is animated (moves or pulses) at all times

**Dependencies:** `MRZ-02` (camera for headlamp attachment), `OLE-05` (shader multi-light support)

**Resources:**
- [LearnOpenGL — Light Casters](https://learnopengl.com/Lighting/Light-casters)

---

### `MRZ-06` 🟠 P1 · Interactions (~3h)
**Week 3** | **⭐ MINIMUM 3 REQUIRED**

Implement user interactions beyond camera control.

**What to do:**
- [ ] **Interaction 1: Toggle headlamp** (`F` key) — already part of `MRZ-05`
- [ ] **Interaction 2: Headlamp color cycle** (`C` key) — already part of `MRZ-05`
- [ ] **Interaction 3: Headlamp intensity** (`+`/`-` or scroll) — already part of `MRZ-05`
- [ ] **Interaction 4: Scare fish** (`E` or left click):
  - [ ] On trigger: find fish within a radius of camera
  - [ ] Increase their animation speed (waveSpeed uniform) temporarily
  - [ ] Move them away from camera position
  - [ ] Reset after a few seconds
- [ ] **Interaction 5: Toggle bioluminescence** (`B` key) — already part of `MRZ-05`
- [ ] Register all key callbacks in GLFW input handling
- [ ] Document all controls for the README

**Acceptance criteria:**
- At least 3 interactions are clearly demonstrable
- Each interaction has visible, immediate feedback in the scene
- Controls are intuitive and documented

**Dependencies:** `MRZ-05`, `NED-04` (for fish scare interaction)

---

### `MRZ-07` 🟡 P2 · Bubble Particle System (~5h)
**Week 3**

CPU-side particle system for rising bubbles.

**What to do:**
- [ ] Create `src/ParticleSystem.h` / `src/ParticleSystem.cpp`
- [ ] Each particle: position, velocity, lifetime, size, alpha
- [ ] Spawn bubbles from defined emitter positions (seabed vents, diver, wreck)
- [ ] Update: rise upward with slight random horizontal drift
- [ ] Render as billboarded quads (always face camera) with alpha blending
- [ ] Recycle dead particles (respawn when lifetime expires)
- [ ] Create `shaders/particle.vert` + `shaders/particle.frag`

**Acceptance criteria:**
- Bubbles rise from the bottom with slight wobble
- Bubbles fade out near the top or after lifetime
- No harsh edges where bubbles clip geometry (soft alpha)
- At least 50–100 bubbles active for visual impact

**Dependencies:** `MRZ-02` (camera for billboarding)

---

### `MRZ-08` 🟡 P2 · Main Loop Integration & README (~5h)
**Week 3–4**

Wire everything together in `main.cpp` and write the final README.

**What to do:**
- [ ] **Render loop order:**
  1. Update time, deltaTime
  2. Process input (camera + interactions)
  3. Update animations (fish positions, light animation, particles)
  4. Shadow pass (bind shadow FBO, render scene with depth shader)
  5. Main pass (bind default FBO, render skybox, scene, fish, particles)
  6. Optional post-processing
  7. Swap buffers
- [ ] Pass all necessary uniforms per frame (camera, lights, shadow map, time)
- [ ] Handle window resize (update projection matrix)
- [ ] **README.md** — controls, methods, build instructions, screenshots
- [ ] Push final version to GitHub

**Acceptance criteria:**
- All systems work together in a single frame without crashes
- Stable 30+ FPS
- No visible rendering order issues (skybox behind everything, transparent objects last)
- README allows a new person to build and run the project

**Dependencies:** All P0 and P1 tasks from all members

---

# 🤝 Shared Tasks

---

### `ALL-01` 🟠 P1 · Find & Prepare 3D Models (~2h each)
**Week 1** | **All members**

**What to do:**
- [ ] **Fish** (1–2 species) — .obj or .fbx with clean UVs
- [ ] **Shipwreck** — large model, closed geometry (for shadows)
- [ ] **Corals** (3–5 varieties) — small decorative models
- [ ] **Seabed** — flat plane or simple mesh (will add normal map for detail)
- [ ] **Diver** (optional) — simple figure model
- [ ] **Jellyfish** — model with separate bell + tentacles (for animation)
- [ ] Verify all models load with Assimp without errors
- [ ] Organize in `models/` subdirectories

**Sources:**
- [Sketchfab](https://sketchfab.com) (filter: free, downloadable)
- [TurboSquid](https://www.turbosquid.com/Search/3D-Models/free/underwater)
- [Free3D](https://free3d.com)

---

### `ALL-02` 🟠 P1 · Find & Prepare Textures (~2h each)
**Week 1–2** | **All members**

**What to do:**
- [ ] **PBR texture sets** (albedo + normal + metallic + roughness + AO):
  - [ ] Sand/ground
  - [ ] Rusty metal (wreck)
  - [ ] Coral/rock
- [ ] **Cubemap faces** (6 images for underwater skybox)
- [ ] **Bubble texture** (small, circular, semi-transparent)
- [ ] Organize in `textures/` subdirectories

**Sources:**
- [ambientCG](https://ambientcg.com) — CC0 PBR sets
- [Poly Haven](https://polyhaven.com) — textures + HDRI

---

### `ALL-03` 🟡 P2 · Code Review & Bug Fixing (~3h each)
**Week 3–4** | **All members**

- [ ] Review each other's code for correctness and consistency
- [ ] Fix integration bugs (shader uniform mismatches, wrong matrix order, etc.)
- [ ] Performance optimization if FPS drops below 30

---

### `ALL-04` 🟡 P2 · Final Polish (~2h each)
**Week 4** | **All members**

- [ ] Consistent color palette (blue/green/teal underwater tones)
- [ ] Scene composition looks good from the default camera position
- [ ] All effects are visible and demonstrable within 5 minutes
- [ ] No crashes or visual glitches during demo flow

---

### `ALL-05` 🟡 P2 · Demo Preparation & Presentation (~2h each)
**Week 4** | **All members**

**What to do:**
- [ ] Write a demo script with timing (5 min max)
- [ ] Set up a good starting camera position
- [ ] Rehearse with all group members — each person presents their methods
- [ ] Prepare backup screenshots/video in case of crash
- [ ] Take screenshots for README

**Acceptance criteria:**
- Demo runs under 5 minutes
- All required methods are visibly demonstrated
- Each group member can explain their part

---

# 📊 Summary

## Effort Breakdown (estimated hours)

| Task | Olejnik | Nędzyński | Mróz | Shared |
|------|---------|-----------|------|--------|
| PBR Basic | **6h** | | | |
| PBR Textures | **4h** | | | |
| Normal Mapping | **5h** | | | |
| Shadow Mapping | **8h** | | | |
| Multi-light Shader | **4h** | | | |
| Shadow Polish | 3h | | | |
| Post-processing | 3h | | | |
| Spline Paths | | **5h** | | |
| PTF | | **5h** | | |
| Fish Animation A10 | | **7h** | | |
| Fish + PTF | | **4h** | | |
| Skybox/Cubemap | | **4h** | | |
| Jellyfish | | 3h | | |
| Multiple Species | | 3h | | |
| Project Setup | | | **4h** | |
| Quaternion Camera | | | **5h** | |
| Model Loading | | | **6h** | |
| Scene Graph | | | **4h** | |
| Moving Lights B13 | | | **5h** | |
| Interactions | | | **3h** | |
| Bubble Particles | | | **5h** | |
| Integration + README | | | **5h** | |
| Find Models | | | | 2h each |
| Find Textures | | | | 2h each |
| Code Review | | | | 3h each |
| Final Polish | | | | 2h each |
| Demo Prep | | | | 2h each |
| **Core total** | **27h** | **25h** | **32h** | |
| **+ Optional (P3)** | +3h | +3h | — | |
| **+ Shared** | +11h | +11h | +11h | |
| **≈ Grand total** | **~38–41h** | **~36–39h** | **~43h** | |

> [!NOTE]
> Mróz has ~4h more than others in raw estimates because infrastructure tasks (setup, model loading, integration) are the glue that enables everything else. These tasks have lighter cognitive load than shader math or animation algorithms but are equally essential. The grading value is also balanced: Olejnik covers ~15 pts in mandatory methods, Nędzyński covers ~10 pts mandatory + 30 pts A10, and Mróz covers ~5 pts mandatory + 15 pts B13 + all interactions and visual coherence contributions.

## By Week

| Week | Olejnik | Nędzyński | Mróz | Shared |
|------|---------|-----------|------|--------|
| **1** | `OLE-01` PBR basic | `NED-01` Spline, `NED-02` PTF, `NED-05` Skybox | `MRZ-01` Setup, `MRZ-02` Camera, `MRZ-03` Models | `ALL-01` Models, `ALL-02` Textures |
| **2** | `OLE-02` PBR tex, `OLE-03` Normal, `OLE-04` Shadows, `OLE-05` Multi-light | `NED-03` Fish anim, `NED-04` Fish+PTF | `MRZ-04` Scene, `MRZ-05` Lights B13 | |
| **3** | `OLE-06` Shadow polish, `OLE-07` Post-proc | `NED-06` Jellyfish, `NED-07` Fish species | `MRZ-06` Interactions, `MRZ-07` Bubbles, `MRZ-08` Integration | `ALL-03` Code review |
| **4** | Bug fixes | Bug fixes | `MRZ-08` README (finish) | `ALL-04` Polish, `ALL-05` Demo |

## Task Count Per Person

| Person | 🔴 P0 | 🟠 P1 | 🟡 P2 | 🟢 P3 | Total |
|--------|-------|-------|-------|-------|-------|
| **Olejnik** | 1 | 3 | 1 | 1 | **6 (+1 optional)** |
| **Nędzyński** | 2 | 3 | 1 | 1 | **6 (+1 optional)** |
| **Mróz** | 3 | 2 | 2 | 0 | **7 (+1 merged)** |
| **Shared** | 0 | 2 | 3 | 0 | **5** |
