# Jak zbudować projekt

Projekt jest wieloplatformowy: **OpenGL 4.1 / GLSL `#version 410`**.
Wspieramy dwie ścieżki budowania, obie sprawdzone:

- **Windows** → Visual Studio (`UnderwaterScene.sln`)
- **macOS / Linux** → CMake (`CMakeLists.txt`)

Sterowanie, opis metod i zrzuty ekranu są w głównym [README](README.md).

---

## Windows — Visual Studio

### 1. Visual Studio z C++

Zainstaluj **Visual Studio 2022 albo nowsze** (wystarczy Community) i w instalatorze
zaznacz pakiet **„Programowanie aplikacji klasycznych w C++"** (Desktop development
with C++). Bez tego nie ma kompilatora.

### 2. Dorzuć biblioteki

Ciężkie biblioteki Windows nie są w repo (są w `.gitignore`). Po sklonowaniu trzeba je
wrzucić z powrotem do `dependencies/`:

Skopiuj zawartość folderu `dependencies/` z frameworka z zajęć
(`Grafika komputerowa - projekt z zajęć/grk/cw 7/dependencies`) do `dependencies/`
w katalogu projektu, tak żeby były tam: `assimp/`, `glew-2.0.0/`,
`glfw-3.3.8.bin.WIN32/`, `glm/`, `imgui/`.

(`glm/` i `imgui/` są już w repo — resztę bierzesz z frameworka.)

DLL-ki potrzebne przy uruchomieniu (`glew32.dll`, `glfw3.dll`, `assimp-*.dll`,
`zlib*.dll`) siedzą w głównym folderze projektu i kopiują się same obok `.exe` po buildzie.

### 3. Budowanie i uruchomienie

1. Otwórz `UnderwaterScene.sln` w Visual Studio.
2. Ustaw konfigurację **`Debug | x86`** (biblioteki są 32-bitowe, więc **nie** zmieniaj na x64).
3. Wciśnij `F5` (build + uruchomienie z debuggerem) albo `Ctrl+F5` (samo uruchomienie).

Katalog roboczy jest ustawiony na folder projektu, więc ścieżki do `shaders/`, `models/`
i `textures/` działają.

### Problem z toolsetem (gdy macie różne wersje VS)

Jeśli projekt jest ustawiony na nowszy toolset niż masz (np. „v145 build tools not found"
w VS 2022):

> Prawym na projekt w Solution Explorer → **Retarget Projects** → wybierz toolset, który
> masz (np. **v143** dla VS 2022) → OK.

Żeby nie robić bałaganu w historii — albo umówcie się na jedną wersję VS, albo
retargetujcie lokalnie i nie commitujcie tej zmiany w `.vcxproj`.

---

## macOS / Linux — CMake

GLM i ImGui są w repo i kompilują się ze źródeł, więc instalujesz tylko resztę zależności.

```bash
# macOS (Homebrew)
brew install cmake glfw glew assimp

# Linux (Debian/Ubuntu)
sudo apt install cmake libglfw3-dev libglew-dev libassimp-dev
```

Build i uruchomienie z katalogu projektu (root repozytorium):

```bash
cmake -S . -B build
cmake --build build -j8
./UnderwaterScene
```

`CMakeLists.txt` ustawia katalog wyjściowy na folder projektu, więc binarka stoi obok
`shaders/`, `models/` i `textures/` — ścieżki względne rozwiązują się same.

Sprawdzone na Apple Silicon (OpenGL 4.1 przez Metal); na macOS ostrzeżenia o
deprecacji OpenGL są wyciszone (`GL_SILENCE_DEPRECATION`), API nadal działa do 4.1.

---

## Jak coś nie działa

- **`v145 build tools not found` (Windows)** — retarget toolsetu (patrz wyżej).
- **`cannot open ... glew32.lib / glfw3.lib / assimp-*.lib` (Windows)** — nie dorzuciłeś
  `dependencies/` (krok 2).
- **Aplikacja odpala się i od razu zamyka, w konsoli `ERROR::ASSIMP` lub błąd cubemapy** —
  zły katalog roboczy. Na Windows odpalaj przez `F5` w VS (nie klikając `.exe` w `Debug/`);
  na macOS/Linux uruchamiaj `./UnderwaterScene` z katalogu projektu.
- **Okienko z brakującym `.dll` (Windows)** — odpal przez `F5` albo skopiuj DLL-ki obok `.exe`.
- **Czarny ekran** — zerknij do konsoli, czy nie ma błędu kompilacji shadera; GLSL musi
  zostać `#version 410`. Brak pliku shadera również wypisze komunikat w konsoli.
- **`Can't find package GLFW3 / GLEW / assimp` (macOS/Linux)** — brakuje zależności,
  doinstaluj je przez `brew` / `apt` (patrz wyżej).
