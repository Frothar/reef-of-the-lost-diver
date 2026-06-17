# Tablica zadań - podwodna scena (A10 + B13)

Postęp odhaczamy tak: `[ ]` (nie zaczęte) → `[/]` (w trakcie) → `[x]` (zrobione).

## Podział roboty

- **Olejnik** - metody obowiązkowe M1 (normal mapping), M2 (PBR), M4 (cienie). Razem ~30h.
  To jeden spójny pipeline shaderowy (PBR + normalne + cienie = jeden system).
- **Nędzyński** - metody obowiązkowe M5 (PTF), M6 (skybox) + metoda A10 (pływanie ryb, 30 pkt). ~31h.
  A10 samo w sobie jest warte tyle co wszystkie metody obowiązkowe razem.
- **Mróz** - metoda obowiązkowa M3 (kamera) + B13 (ruchome światła, 15 pkt) + cała infrastruktura
  (setup, modele, scena, bąbelki, interakcje, sklejenie). ~32h. Mniej matmy, więcej klejenia
  wszystkiego do kupy.

Priorytety: **P0** blokuje inne zadania, **P1** trzeba mieć na ocenę, **P2** dla spójności wizualnej,
**P3** miłe dodatki jak starczy czasu.

---

# Olejnik - rendering i shadery

Ogarnia PBR + normal mapping + cienie + obsługę wielu świateł. ~30h.

## OLE-01 (P0) Shader PBR - podstawa (~6h) - tydzień 1

Bazowy fragment shader z Cook-Torrance BRDF (na sztywnych materiałach, jedno światło kierunkowe).

- [x] `shaders/pbr.vert` - pozycja, normalna, texCoords do world space, macierz TBN
- [x] `shaders/pbr.frag` - Cook-Torrance z GGX (D), Schlick (F), Smith (G)
- [x] jedno światło kierunkowe (słońce przez wodę)
- [x] tone mapping (Reinhard albo ACES) + korekcja gamma
- [x] test na sztywnych wartościach albedo/metallic/roughness na prostym meshu

Jak wiemy, że gotowe: kula/kostka renderuje się z poprawnym PBR (highlight zmienia się z roughness),
metallic=0 wygląda jak plastik/kamień, metallic=1 jak metal.

Zależy od: okna od Mroza (MRZ-01) i jakiegoś wczytanego modelu.
Materiały: [LearnOpenGL PBR Theory](https://learnopengl.com/PBR/Theory),
[PBR Lighting](https://learnopengl.com/PBR/Lighting).

## OLE-02 (P1) Shader PBR - tekstury (~4h) - tydzień 2

Czytanie materiałów z tekstur zamiast ze sztywnych wartości.

- [x] samplery: `albedoMap`, `metallicMap`, `roughnessMap`, `aoMap`
- [x] flaga `useTexture` (albo maska) na mapę, żeby obiekty bez tekstur dalej działały
- [x] kod w C++ do wczytywania materiałów (tekstury, bindowanie do jednostek)
- [x] test na min. 2 zestawach tekstur (np. piasek + zardzewiały metal)

Gotowe gdy: obiekty renderują się z pełnymi zestawami PBR z [ambientCG](https://ambientcg.com),
a te bez tekstur wracają do wartości uniform.

Zależy od: OLE-01.

## OLE-03 (P1) Normal mapping (~5h) - tydzień 2 - METODA OBOWIĄZKOWA M1

Normal mapping w przestrzeni stycznej, wpięty w shader PBR.

- [x] tangent + bitangent na wierzchołek (Assimp to liczy)
- [x] macierz TBN w vertex shaderze, przekazana do fragmentu
- [x] we fragmencie: próbka z normal mapy, przemapowanie [0,1] → [-1,1], transformacja przez TBN
- [x] użycie zaburzonej normalnej `N` we wszystkich obliczeniach PBR
- [x] na min. 2 materiałach (dno + kadłub wraka albo koral)

Gotowe gdy: powierzchnia ma widoczne wyboje reagujące na światło, wyłączenie normal mapy daje
wyraźnie płaski wygląd, brak szwów na granicach UV.

Zależy od: OLE-02. Materiały: [LearnOpenGL Normal Mapping](https://learnopengl.com/Advanced-Lighting/Normal-Mapping).

## OLE-04 (P1) Shadow mapping (~8h) - tydzień 2 - METODA OBOWIĄZKOWA M4

Cienie dla głównego światła kierunkowego z filtrowaniem PCF.

- [x] FBO tylko z głębią (tekstura 2048x2048, `GL_DEPTH_COMPONENT`)
- [x] `shaders/shadow_depth.vert` + `.frag` - minimalny shader zapisujący głębię
- [x] macierz ortho + view dla światła
- [x] przebieg cieni: renderowanie geometrii do FBO głębi
- [x] przebieg główny: fragment do przestrzeni światła, próbka mapy cieni, porównanie głębi
- [x] bias zależny od kąta: `bias = max(0.05 * (1.0 - dot(N, L)), 0.005)`
- [x] PCF 3x3 na miękkie krawędzie
- [x] wpięcie współczynnika cienia w wyjście shadera PBR

Gotowe gdy: wrak (albo duży obiekt) rzuca widoczny cień na dno, brak migoczących kropek na
oświetlonych powierzchniach, krawędzie miękkie a nie poszarpane.

Zależy od: OLE-01 i tego, żeby Mróz miał min. 2 obiekty w scenie.
Materiały: [LearnOpenGL Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping).

## OLE-05 (P1) Wiele świateł w shaderze PBR (~4h) - tydzień 2-3

Obsługa wielu typów świateł z systemu Mroza.

- [x] tablice uniform `PointLight[MAX]` i `SpotLight[MAX]`
- [x] `calcPointLight()` - PBR z tłumieniem punktowym
- [x] `calcSpotLight()` - PBR ze stożkiem reflektora + tłumienie
- [x] sumowanie radiancji `Lo` po wszystkich aktywnych światłach
- [x] dogadać z Mrozem układ struktur i nazwy uniformów

Gotowe gdy: scena dobrze oświetlona słońcem + ruchomymi światłami + reflektorem, punktowe mają
widoczny spadek z odległością, reflektor ma stożek (inner/outer cutoff).

Zależy od: OLE-01, MRZ-05.

## OLE-06 (P2) Poprawki i polerka cieni (~3h) - tydzień 3

- [x] peter-panning (cień odkleja się od obiektu) - poprawa biasu
- [x] pokrycie mapy cieni (obiekty poza frustum światła) - poprawa rozmiaru ortho
- [x] opcjonalnie front-face culling w przebiegu cieni
- [x] opcjonalnie druga mapa cieni dla reflektora latarki

Gotowe gdy: brak artefaktów z typowych kątów kamery, światło nie przecieka przez obiekty.
Zależy od: OLE-04.

## OLE-07 (P3) Post-processing - kolorystyka podwodna (~3h) - tydzień 3-4

- [x] renderowanie sceny do pośredniego FBO (kolor + głębia)
- [x] fullscreen quad z niebiesko-zielonym tintem
- [x] mgła zależna od głębi (wykładnicza, niebiesko-zielona)
- [x] opcjonalnie lekka aberracja chromatyczna / zniekształcenie na brzegach

Gotowe gdy: scena bardziej "podwodna", parametry łatwe do podkręcenia.
Zależy od: reszty renderingu.

---

# Nędzyński - animacja, fizyka, środowisko

Ryby (A10) + PTF + skybox + meduzy. ~31h.

## NED-01 (P0) Splajny (~5h) - tydzień 1

Klasa splajnu Catmull-Rom do gładkich ścieżek w 3D.

- [x] `src/Spline.h` / `.cpp`
- [x] `addControlPoint(glm::vec3)`, `evaluate(float t)`, `evaluateTangent(float t)`
- [x] interpolacja Catmull-Rom między punktami
- [x] ścieżki zapętlone (ostatni punkt łączy się z pierwszym)
- [x] test: render splajnu jako linii, żeby sprawdzić gładkość

Gotowe gdy: ścieżka z 5+ punktami daje gładką, ciągłą krzywą, `evaluate(0)` = pierwszy punkt,
tangenty gładkie i niezerowe.

Bez zależności, można od razu.
Materiały: [Catmull-Rom](https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline).

## NED-02 (P0) Parallel Transport Frames (~5h) - tydzień 1 - METODA OBOWIĄZKOWA M5

Stabilne ramki orientacji wzdłuż splajnu (bez skręcania) do orientacji ryb.

- [x] PTF w `Spline.cpp` (albo osobny moduł)
- [x] próbkowanie splajnu w N punktach, tangent w każdym
- [x] ramka startowa: dowolna normalna prostopadła do pierwszego tangentu
- [x] dla kolejnych punktów: obrót poprzedniej normalnej do nowego tangentu (cross + kąt)
- [x] zapis ramek jako `(T, N, B)` albo `glm::mat4` na próbkę
- [x] metoda `getFrame(float t)` → pozycja, tangent, normalna, bitangent

Gotowe gdy: obiekt na splajnie trzyma stabilną orientację (bez losowego obracania/skręcania),
działa na krzywych o różnej krzywiźnie i na prostych odcinkach. Test: małe osie/box na każdej ramce.

Zależy od: NED-01.
Materiały: [Parallel Transport (giordi91)](https://giordi91.github.io/post/2018-31-07-parallel-transport/).

## NED-03 (P1) Pływanie ryb - vertex shader (~7h) - tydzień 2 - TO JEST METODA A10 (30 pkt)

Falowanie ciała ryby przez sinusoidę w vertex shaderze.

- [x] `shaders/fish.vert` - bazuje na vertex shaderze PBR + deformacja
- [x] `shaders/fish.frag` - jak `pbr.frag` (wspólny kod oświetlenia: fish.vert + pbr.frag)
- [x] fala ciała: `deformed.x += sin(z * freq - time * speed) * amplitude * bodyFactor`
- [x] `bodyFactor = smoothstep(0, 1, z / fishLength)` - fala rośnie ku ogonowi
- [x] animacja płetw: osobna sinusoida
- [x] uniformy: `time`, `waveAmplitude`, `waveFrequency`, `waveSpeed`, `fishLength`
- [x] przeliczenie normalnych po deformacji (różnice skończone albo analitycznie)

Gotowe gdy: ciało ryby faluje jak przy pływaniu, głowa stabilna a ogon macha najmocniej, płetwy
ruszają się niezależnie, parametry da się zmieniać uniformami, oświetlenie poprawne na zdeformowanej
powierzchni (zaktualizowane normalne).

Zależy od: OLE-01 (na nim bazuje shader ryby).
Materiały: [GPU Gems - Animation in the Dawn Demo](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-4-animation-dawn-demo).

## NED-04 (P1) Ryby jadące po splajnie z PTF (~4h) - tydzień 2

- [x] `src/FishAnimation.h` / `.cpp`
- [x] animacja parametru `t` po splajnie w czasie (stała prędkość albo wygładzona)
- [x] w każdej klatce: pozycja + ramka z PTF → macierz modelu
- [x] macierz modelu: kolumny = (B, N, -T) z ramki + pozycja
- [x] połączenie orientacji z PTF z deformacją pływania z vertex shadera
- [x] kilka ryb na różnych ścieżkach (albo ta sama ścieżka z przesunięciem w czasie)

Gotowe gdy: ryba płynnie jedzie po ścieżce, zawsze patrzy do przodu wzdłuż krzywej (bez przeskoków),
animacja pływania gra na wierzchu, min. 2 ryby na różnych ścieżkach.

Zależy od: NED-01, NED-02, NED-03.

## NED-05 (P1) Podwodny skybox / cubemapa (~4h) - tydzień 1 - METODA OBOWIĄZKOWA M6

- [x] 6 tekstur ścian cubemapy (głębokie niebiesko-zielone)
- [x] wczytanie przez `GL_TEXTURE_CUBE_MAP` (6 ścian)
- [x] `shaders/skybox.vert` + `.frag`
- [x] vertex shader: ucięcie translacji z view (`mat4(mat3(view))`), trik `pos.xyww`
- [x] fragment: próbka cubemapy wektorem kierunku
- [x] render z `GL_LEQUAL` i wyłączonym zapisem głębi
- [x] skybox nie rusza się z kamerą (tylko się obraca)

Gotowe gdy: tło wygląda jak podwodne środowisko we wszystkie strony, skybox stoi przy ruchu kamery
(tylko się obraca), góra jaśniejsza (słońce), dół ciemniejszy (głębia).

Zależy od: MRZ-01. (Częściowo już jest w szkielecie.)
Materiały: [LearnOpenGL Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps).

## NED-06 (P2) Pulsujące meduzy (~3h) - tydzień 3

- [x] deformacja pulsowania w vertex shaderze (skalowanie dzwonu sinusem)
- [x] można użyć shadera ryby z innymi parametrami albo osobnego
- [x] synchronizacja pulsowania z ruchem do góry (puls = napęd)
- [x] lekkie ruchy czułek (sinus na dolnych wierzchołkach)

Gotowe gdy: meduza widocznie pulsuje dzwonem, ruch wygląda organicznie, czułki się kołyszą.
Zależy od: NED-03.

## NED-07 (P3) Więcej gatunków ryb (~3h) - tydzień 3-4

- [ ] 2-3 "profile" ryb z różną amplitudą/częstotliwością/prędkością/rozmiarem
- [ ] różne modele albo przeskalowane wersje tego samego
- [ ] różne ścieżki i prędkości

Gotowe gdy: min. 2 wyraźnie różne typy ryb pływają w scenie, każdy inaczej.
Zależy od: NED-03, NED-04.

---

# Mróz - scena, kamera, światła

Setup + kamera na kwaternionach + światła (B13) + bąbelki + scena + interakcje. ~32h.

## MRZ-01 (DONE) Setup projektu i okno (~4h) - tydzień 1

Projekt postawiony z frameworka z zajęć (cw 7) jako samodzielny projekt **Visual Studio** w
`UnderwaterScene/`, z dołączonymi bibliotekami.

- [x] solucja VS `UnderwaterScene.sln` / `.vcxproj` (Win32, toolset v145) z GLFW, GLEW, GLM, Assimp, SOIL, ImGui
- [x] `src/main.cpp` - okno GLFW, kontekst OpenGL 4.1 core, init GLEW, pętla główna
- [x] `src/scene_underwater.hpp` - szkielet sceny (kamera, skybox, pętla rysowania)
- [x] wczytywanie shaderów przez Core `Shader_Loader` (osobny `Shader.h` niepotrzebny)
- [x] `.gitignore` + `.gitattributes`
- [x] `CMakeLists.txt` pod Maca/Linuksa (opcjonalny, nie testowany - robimy na Windows/VS)
- [x] kompiluje się i odpala: okno wstaje, w konsoli `OpenGL 4.1.0`

Gotowe. Reszta buduje na tym. Instrukcja: `UnderwaterScene/BUILD.md`.

## MRZ-02 (DONE) Kamera na kwaternionach (~5h) - tydzień 1 - METODA OBOWIĄZKOWA M3

- [x] `src/QuaternionCamera.h` (klasa `Camera`, header-only, żeby nie kolidować z Core/Camera.h)
- [x] orientacja jako `glm::quat`, pozycja jako `glm::vec3`
- [x] mysz → kwaterniony yaw/pitch składane w lokalnym układzie (brak gimbal locka)
- [x] wektory forward/right/up z orientacji
- [x] view przez `glm::lookAt(position, position + front, up)`
- [x] ruch WSAD wzdłuż lokalnych osi
- [x] Spacja / Lewy Ctrl góra-dół (względem świata), Lewy Shift przyspieszenie
- [x] Q/E przechył (roll) - to pokazuje sens kwaternionów (pełne 6DOF)

Gotowe: kamera obraca się gładko we wszystkie strony bez gimbal locka, da się obrócić "przez czubek
głowy" bez przeskoku, ruch jak pływanie (6DOF). Wpięta w scenę zamiast tymczasowej kamery free-fly,
zbudowane i przetestowane.

Zależy od: MRZ-01.
Materiały: [opengl-tutorial Quaternions](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/).

## MRZ-03 (P0) Wczytywanie modeli (~6h) - tydzień 1

- [ ] `src/Model.h` / `.cpp` - wczytanie modelu przez Assimp
- [ ] `src/Mesh.h` / `.cpp` - opakowanie VAO/VBO/EBO
- [ ] wyciąganie wierzchołków (pozycja, normalna, texCoords, tangent, bitangent) i indeksów
- [ ] wczytanie tekstur z materiałów modelu
- [ ] `src/Texture.h` / `.cpp` - wczytywanie obrazków przez SOIL, jednostki teksturujące
- [ ] test: wczytanie i render prostego .obj z teksturą

(Uwaga: część tego, Texture i RenderContext, jest już w klasach Core ze szkieletu - można rozbudować.)

Gotowe gdy: testowy model się wyświetla z teksturą, tangent/bitangent dostępne dla normal mappingu Olejnika.
Zależy od: MRZ-01, MRZ-02.

## MRZ-04 (P1) Scena i rozmieszczenie obiektów (~4h) - tydzień 2-3

- [ ] `src/Scene.h` / `.cpp`
- [ ] lista obiektów: model, pozycja, rotacja, skala
- [ ] `Scene::render(Shader&)` - iteracja, ustawienie macierzy modelu, rysowanie
- [ ] osobne wywołania dla przebiegu cieni i głównego
- [ ] rozmieszczenie: dno (płaszczyzna/heightmapa), koralowiec (3-5 modeli), wrak (duży, rzuca cień), nurek
- [ ] sensowna skala względem siebie (ryby małe, wrak duży)

Gotowe gdy: scena ma rozpoznawalną podwodną kompozycję, obiekty w sensownej skali, przebieg cieni
może użyć tego samego rysowania z innym shaderem.

Zależy od: MRZ-03 i znalezionych modeli (ALL-01).

## MRZ-05 (DONE) Ruchome światła i latarka - B13 (~5h) - tydzień 2 - TO JEST METODA B13 (15 pkt)

- [x] struktury świateł `PointLightCPU` / `SpotLightCPU` + DirLight w shaderze (struktury siedzą
      w `scene_underwater.hpp`, dorobione przy integracji OLE-05 - osobny `Light.h` niepotrzebny)
- [x] latarka nurka (reflektor): przyczepiona do kamery (pozycja + kierunek), F włącza/wyłącza,
      C zmienia kolor (biały → ciepły → chłodny niebieski), +/- albo scroll zmienia jasność
- [x] światła bioluminescencji (3): pozycja dryfuje `sin(time)`, jasność pulsuje
      `sin(time * pulseSpeed)`, kolory niebieski/zielony/fioletowy, B włącza/wyłącza
- [x] wysyłanie danych świateł do shadera co klatkę (point + spot)

Gotowe: latarka oświetla obiekty stożkiem jadącym z kamerą, F/C/+/-/scroll działają, bioluminescencja
świeci i pulsuje. Wykorzystany gotowy spotlight z OLE-05 (wcześniej `numSpotLights` było 0).

Zależy od: MRZ-02, OLE-05.
Materiały: [LearnOpenGL Light Casters](https://learnopengl.com/Lighting/Light-casters).

## MRZ-06 (P1) Interakcje (~3h) - tydzień 3 - MIN. 3 WYMAGANE

- [x] latarka on/off (F) - część MRZ-05
- [x] zmiana koloru latarki (C) - część MRZ-05
- [x] jasność latarki (+/- albo scroll) - część MRZ-05
- [x] straszenie ryb (lewy klik - E zajęte przez roll kamery): ryby w promieniu smigają szybciej
      (mnożnik prędkości na splajnie) i machają ogonem jak szalone, reset po 3 s
- [x] bioluminescencja on/off (B) - część MRZ-05
- [x] rejestracja callbacków klawiszy w GLFW (key + scroll)
- [ ] spisanie sterowania do README

Już mamy 4 działające interakcje (F/C/jasność/B) - minimum 3 spełnione. Zostaje straszenie ryb.
Gotowe gdy: min. 3 interakcje da się pokazać, każda daje widoczny efekt od razu, sterowanie intuicyjne.
Zależy od: MRZ-05, NED-04 (do straszenia ryb).

## MRZ-07 (DONE) Bąbelki - system cząstek (~5h) - tydzień 3

- [x] `src/ParticleSystem.h` (klasa `BubbleSystem`, header-only)
- [x] cząstka: pozycja, prędkość, czas życia, rozmiar, alpha
- [x] spawn z emiterów przy dnie (4 kominy)
- [x] update: w górę z lekkim dryfem na boki
- [x] render jako point sprites (zawsze do kamery) z alpha blendingiem, bez zapisu głębi
- [x] recykling martwych cząstek (respawn)
- [x] `shaders/particle.vert` + `.frag`

Gotowe: 160 bąbelków wznosi się z dna z chwianiem, zanikają pod koniec życia i respawnują się.
Rysowane po skyboxie, przed overlayem wody. Toggle + kolor w panelu ImGui.

Zależy od: MRZ-02.

## MRZ-08 (P2) Sklejenie całości + README (~5h) - tydzień 3-4

- [ ] kolejność w pętli: czas/deltaTime → input → animacje → przebieg cieni → przebieg główny
      (skybox, scena, ryby, cząstki) → ewentualny post-processing → swap buffers
- [ ] wszystkie uniformy co klatkę (kamera, światła, mapa cieni, czas)
- [ ] obsługa resize okna (aktualizacja projekcji)
- [ ] README: sterowanie, metody, instrukcja, screeny
- [ ] wypchnięcie finalnej wersji na GitHub

Gotowe gdy: wszystko działa razem bez wywałek, stabilne 30+ FPS, brak problemów z kolejnością
renderowania (skybox za wszystkim, przezroczyste na końcu), README pozwala komuś nowemu zbudować i odpalić.

Zależy od: wszystkich zadań P0 i P1.

---

# Zadania wspólne

## ALL-01 (P1) Znalezienie i przygotowanie modeli 3D (~2h każdy) - tydzień 1

- [ ] ryby (1-2 gatunki) - .obj/.fbx z czystym UV
- [ ] wrak - duży model, zamknięta geometria (na cienie)
- [ ] korale (3-5 sztuk) - małe ozdobne modele
- [ ] dno - płaszczyzna albo prosty mesh
- [ ] nurek (opcjonalnie)
- [ ] meduza - model z osobnym dzwonem + czułkami
- [ ] sprawdzić, że wszystko ładuje się przez Assimp bez błędów
- [ ] poukładać w `models/`

Gdzie szukać: [Sketchfab](https://sketchfab.com) (filtr: free, downloadable),
[TurboSquid](https://www.turbosquid.com/Search/3D-Models/free/underwater), [Free3D](https://free3d.com).

## ALL-02 (P1) Znalezienie i przygotowanie tekstur (~2h każdy) - tydzień 1-2

- [ ] zestawy PBR (albedo + normal + metallic + roughness + AO): piasek, zardzewiały metal, koral/skała
- [ ] 6 ścian cubemapy na podwodny skybox
- [ ] tekstura bąbelka (mała, okrągła, półprzezroczysta)
- [ ] poukładać w `textures/`

Gdzie: [ambientCG](https://ambientcg.com) (CC0), [Poly Haven](https://polyhaven.com).

## ALL-03 (P2) Przegląd kodu i bugi (~3h każdy) - tydzień 3-4

- [ ] przejrzeć nawzajem kod pod kątem poprawności i spójności
- [ ] naprawić bugi z łączenia (nazwy uniformów, kolejność macierzy itd.)
- [ ] optymalizacja jak FPS spada poniżej 30

## ALL-04 (P2) Szlif końcowy (~2h każdy) - tydzień 4

- [ ] spójna paleta kolorów (niebiesko-zielono-turkusowe)
- [ ] kompozycja sceny dobra z domyślnej pozycji kamery
- [ ] wszystkie efekty widoczne i do pokazania w 5 minut
- [ ] brak wywałek i glitchy podczas demo

## ALL-05 (P2) Przygotowanie demo i prezentacji (~2h każdy) - tydzień 4

- [ ] scenariusz demo z czasami (max 5 min)
- [ ] dobra pozycja startowa kamery
- [ ] próba z całą grupą - każdy opowiada o swoich metodach
- [ ] backupowe screeny/wideo na wypadek wywałki
- [ ] zrobienie screenów do README

Gotowe gdy: demo poniżej 5 minut, wszystkie wymagane metody pokazane, każdy umie wytłumaczyć swoją część.

---

# Podsumowanie godzin

Z grubsza (godziny):

- Olejnik: ~27h core + ~3h opcjonalne + ~11h wspólne = ~38-41h
- Nędzyński: ~25h core + ~3h opcjonalne + ~11h wspólne = ~36-39h
- Mróz: ~32h core + ~11h wspólne = ~43h

Mróz ma trochę więcej, bo infrastruktura (setup, modele, integracja) to klej trzymający wszystko
razem. Mniej kombinowania niż przy matmie shaderów czy animacji, ale równie potrzebne. Punktowo
wychodzi po równo: Olejnik ~15 pkt w metodach obowiązkowych, Nędzyński ~10 pkt obowiązkowe + 30 pkt
A10, Mróz ~5 pkt obowiązkowe + 15 pkt B13 + interakcje i spójność wizualna.

## Plan tygodniowy

- **Tydzień 1**: OLE-01 / NED-01, NED-02, NED-05 / MRZ-01, MRZ-02, MRZ-03 / ALL-01, ALL-02
- **Tydzień 2**: OLE-02, OLE-03, OLE-04, OLE-05 / NED-03, NED-04 / MRZ-04, MRZ-05
- **Tydzień 3**: OLE-06, OLE-07 / NED-06, NED-07 / MRZ-06, MRZ-07, MRZ-08 / ALL-03
- **Tydzień 4**: poprawki / ALL-04, ALL-05, dokończenie README
