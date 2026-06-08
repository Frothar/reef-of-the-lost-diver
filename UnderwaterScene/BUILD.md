# Jak zbudować projekt

Robimy wszyscy w **Visual Studio na Windowsie** - i to jest droga, którą wspieramy.
Projekt chodzi na OpenGL 4.1 (shadery `#version 410`).

## Raz na początku (każdy u siebie)

### 1. Visual Studio z C++

Zainstaluj **Visual Studio 2022 albo 2026** (wystarczy Community) i w instalatorze zaznacz
pakiet **"Programowanie aplikacji klasycznych w C++"** (Desktop development with C++).
Bez tego nie ma kompilatora i nic się nie zbuduje.

### 2. Dorzuć biblioteki

Ciężkie biblioteki Windows nie są w repo (są w .gitignore). Po sklonowaniu trzeba je wrzucić
z powrotem do `UnderwaterScene/dependencies/`:

Skopiuj folder `dependencies/` z frameworka z zajęć
(`Grafika komputerowa - projekt z zajęć/grk/cw 7/dependencies`) do `UnderwaterScene/`, tak żeby
w `UnderwaterScene/dependencies/` były: `assimp/`, `glew-2.0.0/`, `glfw-3.3.8.bin.WIN32/`,
`glm/`, `imgui/`.

(`glm/` i `imgui/` już są w repo, resztę bierzesz z frameworka.)

DLL-ki potrzebne przy uruchomieniu (`glew32.dll`, `glfw3.dll`, `assimp-*.dll`, `zlib*.dll`)
siedzą w głównym folderze projektu i kopiują się same obok .exe po buildzie.

## Budowanie i odpalanie

1. Otwórz `UnderwaterScene.sln` w Visual Studio.
2. Upewnij się, że konfiguracja to `Debug | x86` (biblioteki są 32-bit, więc **nie** zmieniaj na x64).
3. Wciśnij F5 (build + uruchomienie z debuggerem) albo Ctrl+F5 (samo uruchomienie).

Katalog roboczy jest ustawiony na folder projektu, więc ścieżki do `shaders/`, `models/`
i `textures/` działają.

## Problem z toolsetem (ważne, jak macie różne wersje VS)

Projekt jest ustawiony na toolset **v145 (Visual Studio 2026)**. Jak otworzysz go w
**VS 2022**, wyskoczy coś w stylu "v145 build tools not found".

Naprawa u siebie: prawym na projekt w Solution Explorer, "Retarget Projects", wybierz
toolset który masz (np. **v143** dla VS 2022), OK.

Żeby nie robić bałaganu w historii - albo umówcie się na jedną wersję VS, albo po prostu
retargetujcie lokalnie i nie commitujcie tej jednej zmiany w `.vcxproj`.

## Jak coś nie działa

- `MSB8020: v145 build tools not found` - retarget (patrz wyżej)
- `cannot open ... glew32.lib / glfw3.lib / assimp-*.lib` - nie dorzuciłeś `dependencies/` (punkt 2)
- Aplikacja odpala się i od razu zamyka, w konsoli `ERROR::ASSIMP` albo błąd cubemapy - zły katalog
  roboczy, odpalaj przez F5 w VS, a nie klikając .exe w folderze `Debug/`
- Okienko z brakującym `.dll` przy ręcznym odpaleniu .exe - odpal przez F5 albo skopiuj DLL-ki obok .exe
- Czarny ekran - zerknij do konsoli czy nie ma błędu kompilacji shadera; GLSL musi zostać `#version 410`

## Sterowanie w szkielecie

- `WSAD` - pływanie
- `Spacja` / `Lewy Ctrl` - góra / dół
- `Lewy Shift` (przytrzymane) - szybciej
- mysz - rozglądanie
- `Caps Lock` - przełącz mysz / panel ImGui
- `Esc` - wyjście

Ta kamera to tymczasowy placeholder, docelowo zastępuje ją kamera na kwaternionach (zadanie MRZ-02).

## Co jest w szkielecie

- okno + kontekst OpenGL 4.1, GLEW, GLFW, panelik ImGui
- klasy Core z zajęć: `Shader_Loader`, `Render_Utils` (RenderContext, DrawContext, wczytywanie
  modeli przez Assimp), `Texture` (LoadTexture, LoadCubemap, SetActiveTexture), `Camera`, SOIL
- skybox z cubemapy, oświetlone dno i testowa kula, no i mgła podwodna
- shadery `#version 410 core`: `underwater.vert/frag`, `skybox.vert/frag`

Stąd budujemy dalej rzeczy z `../TASKS.md`.

## Inne systemy (opcjonalnie, nie testowane)

Jest też `CMakeLists.txt` pod Maca/Linuksa (`brew install glfw glew assimp` albo
`apt install libglfw3-dev libglew-dev libassimp-dev`, potem `cmake -S . -B build &&
cmake --build build`). My robimy na Windows/VS, więc to nie jest sprawdzane - używaj tylko
jak naprawdę musisz zbudować poza Windowsem.
