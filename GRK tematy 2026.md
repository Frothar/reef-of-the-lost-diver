**Projekt zaliczeniowy GRK**  
 **Interaktywna scena podwodna w OpenGL/GLSL**

**Specyfikacja dla studentów**  
 Termin oddania i prezentacji: podczas egzaminu w sesji egzaminacyjnej. Dokładna data zostanie podana później.

# **0 Specyfikacja ogólna i zasady organizacyjne**

**Motyw projektu:** interaktywny świat podwodny. Każda grupa przygotowuje aplikację 3D w C++/OpenGL/GLSL przedstawiającą spójną scenę podwodną, np. rafę koralową, wrak, łódź podwodną, nurka, organizmy morskie, pęcherzyki powietrza, prądy wodne, roślinność, ruiny lub inne elementy pasujące do wspólnego motywu.

·        **Skład grupy:** projekt jest realizowany w grupach trzyosobowych. Grupy powinny wpisać imiona i nazwiska członków przy wybranej kombinacji metod w dokumencie lub arkuszu zapisów.  
·        Wybór metod: każda grupa wybiera dokładnie jedną metodę z kategorii A oraz dokładnie jedną metodę z kategorii B. Oprócz tego każda grupa musi zaimplementować wszystkie metody obowiązkowe: normal mapping, PBR lighting, quaternion camera control, shadow mapping, Parallel Transport Frames oraz underwater skybox/cubemap.  
·        **Unikalność kombinacji:** każda kombinacja metody A i metody B musi być unikalna. Dwie grupy nie mogą wybrać tej samej pary A+B.  
·        **Limit popularności metod:** każda pojedyncza metoda z kategorii A oraz każda pojedyncza metoda z kategorii B może zostać wybrana maksymalnie przez trzy grupy. Obowiązuje zasada first come, first served.  
·        **Zamknięcie zapisów:** grupy mają dwa tygodnie na wpisanie składu i zablokowanie wyboru metod. Po tym czasie dokument zapisów zostanie ustawiony jako nieedytowalny, a późniejsze zmiany będą możliwe tylko po uzgodnieniu z prowadzącym.  
·        **Repozytorium GitHub:** kod projektu musi zostać umieszczony w repozytorium GitHub. Repozytorium musi być dostępne dla prowadzącego przed terminem egzaminu. Dokładny termin egzaminu i prezentacji zostanie opublikowany później w tym miesiącu.  
·        **Prezentacja podczas egzaminu:** w czasie egzaminu spotykamy się wspólnie; każda grupa wykonuje maksymalnie 5-minutowe demo aplikacji i krótko wyjaśnia, co zostało zaimplementowane, jakie metody wybrano oraz jakie elementy interaktywne i wizualne są najważniejsze.  
·        **Interaktywność:** interaktywność jest obowiązkowa. Każda aplikacja musi zawierać co najmniej dwie interakcje ze sceną wykonywane w czasie działania programu, niezależne od sterowania kamerą. Przykłady: aktywowanie świateł łodzi podwodnej, zbieranie obiektów, otwieranie skrzyni, zmiana trybu ruchu stworzenia, sterowanie prądem wodnym, przełączanie animacji, kliknięcie obiektów lub wpływanie na zachowanie ryb.  
·        **Ocena:** oceniane będą przede wszystkim kompletność i poprawność implementacji metod, integracja technik w działającej aplikacji, stabilność programu, czytelność kodu i repozytorium, jakość demonstracji oraz estetyka i spójność interaktywnej sceny 3D.

**Uwaga organizacyjna:** wybór metod należy traktować jako deklarację techniczną. Wybrana metoda A i wybrana metoda B powinny być widoczne w finalnej aplikacji, opisane w README oraz zaprezentowane podczas 5-minutowego demo.



Rys. 1 Inspiracja wizualna: rafa koralowa i podwodna atmosfera. Źródło: [Wikimedia Commons — Underwater photo of coral reef.jpg](https://commons.wikimedia.org/wiki/File:Underwater_photo_of_coral_reef.jpg); autor: Jerry Reid, U.S. Fish and Wildlife Service; status: public domain.

# **1 Cel projektu**

Celem projektu jest przygotowanie interaktywnej aplikacji graficznej w C++ z użyciem OpenGL i GLSL. Motywem wszystkich projektów jest scena podwodna: rafa, wrak, batyskaf, ruiny, jaskinia, głębina oceaniczna, laboratorium podwodne albo inna spójna przestrzeń morska. Projekt ma pokazać zarówno poprawną implementację wymaganych metod renderingu, jak i umiejętność zintegrowania kilku technik w jedną działającą scenę czasu rzeczywistego.

Scena musi zawierać co najmniej trzy znaczące interakcje poza samym ruchem kamery, np. wybór i aktywację obiektu, sterowanie światłem, zbieranie obiektów, zmianę parametrów środowiska, interakcję z rybą/stworem, otwieranie skrzyni lub sterowanie batyskafem.

# **2 Wymagania techniczne**

·        Projekt musi być aplikacją C++ opartą na OpenGL i GLSL.  
·        Należy korzystać z frameworku i konwencji używanych na laboratoriach. Punktem odniesienia są paczki laboratoryjne ze strony kursu GRK.  
·        Dozwolone jest użycie wybranej biblioteki UI, np. Dear ImGui, ale UI nie może zastąpić implementacji wymaganych metod renderingu.  
·        Nie wolno używać gotowych silników renderujących lub gier, takich jak Unity, Unreal Engine, Godot, Blender Game Engine itp.  
·        Strona kursu: [GRK — materiały i laboratoria](https://wp.faculty.wmi.amu.edu.pl/GRK.html)

# **3 Metody obowiązkowe dla wszystkich grup**

Poniższe metody są obowiązkowe dla każdej grupy i nie liczą się jako metody dodatkowe z list A/B. Każda z nich musi być zintegrowana z finalną sceną i pokazana podczas prezentacji.

·        **Normal mapping:** Widoczne użycie map normalnych na co najmniej dwóch różnych materiałach; poprawna przestrzeń styczna TBN.  
·        **PBR lighting:** Shader PBR w wariancie metallic/roughness albo równoważnym; parametry materiałów muszą być możliwe do zademonstrowania.  
·        **Quaternion camera control:** Kamera sterowana kwaternionami, bez gimbal lock; ruch i rotacja muszą być płynne.  
·        **Shadow mapping:** Mapa cienia dla co najmniej jednego źródła światła; należy ograniczyć podstawowe artefakty przez bias lub PCF.  
·        **Parallel Transport Frames:** Ramki transportu równoległego wykorzystane znacząco, np. do ruchu ryb, kamery po splajnie, macki, kabla lub generacji tunelu/przewodu.  
·        **Underwater skybox/cubemap:** Cubemapa środowiskowa renderowana jako tło sceny; musi tworzyć spójną podwodną atmosferę i poprawnie działać niezależnie od przesunięcia kamery.

# **4 System wyboru metod dodatkowych**

Każda grupa wybiera jedną metodę z listy A oraz jedną metodę z listy B. Wybór odbywa się według zasady „kto pierwszy, ten lepszy” w arkuszu Google Sheets lub pliku Excel udostępnionym przez prowadzącego.

·        Każda dokładna kombinacja A+B musi być unikalna. Nie mogą istnieć dwie grupy z tą samą parą metod, np. A03+B10.  
·        Każda pojedyncza metoda z listy A lub B może być wybrana maksymalnie przez 3 grupy w całym kursie.  
·        Wpis w arkuszu powinien zawierać: numer grupy, imiona i nazwiska studentów, wybrane ID i nazwy metod, kombinację A+B, adres repozytorium GitHub i status prezentacji.  
·        Zmiana metod po wpisaniu do arkusza wymaga aktualizacji wpisu i zachowania limitów dostępności.

## **4.1 Lista A — metody zaawansowane**

Tabela 1 Metody zaawansowane do wyboru.


| ID  | Metoda                                                 | Opis                                                                               | Minimum implementacyjne                                                                      | Motyw podwodny                                                             |
| --- | ------------------------------------------------------ | ---------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| A01 | Volumetric underwater light shafts / single scattering | Podwodne smugi światła albo uproszczone pojedyncze rozpraszanie w post-processie.  | Efekt zależny od położenia światła i gęstości ośrodka; możliwość regulacji intensywności.    | Promienie słońca przez powierzchnię wody lub światło reflektora batyskafu. |
| A02 | Projected or screen-space caustics                     | Kaustyki rzutowane na geometrię lub liczone w przestrzeni ekranu.                  | Animowany wzorzec kaustyk spójny z ruchem wody i geometrią sceny.                            | Jasne wzory światła na dnie, wraku, skałach i rafie.                       |
| A03 | Gerstner-wave water surface                            | Powierzchnia wody z przesunięciem wierzchołków i dynamicznymi normalnymi.          | Suma kilku fal Gerstnera w shaderze wierzchołków; normalne wykorzystywane w oświetleniu.     | Lustro wody widziane od spodu, wpływające na refleksy i kaustyki.          |
| A04 | Transform-feedback GPU particles                       | Symulacja cząstek po stronie GPU z użyciem transform feedback.                     | Buforowanie stanu cząstek na GPU, aktualizacja i render w pętli bez pełnej symulacji CPU.    | Bąble, plankton, osad unoszony przez prąd lub ruch łodzi podwodnej.        |
| A05 | Deferred shading / G-buffer                            | Renderowanie sceny przez G-buffer i osobny pass oświetlenia.                       | G-buffer z pozycją, normalną i kolorem/albedo; osobny pass dla wielu świateł.                | Wiele punktowych świateł, reflektorów i świecących organizmów.             |
| A06 | Screen-space refraction/reflection                     | Załamanie lub odbicie na podstawie tekstur renderowanych do buforów.               | Render-to-texture, odczyt sceny w drugim passie i zniekształcenie próbkowania normalą.       | Szyba batyskafu, bańki powietrza, powierzchnia wody, soczewkowanie.        |
| A07 | Instanced rendering with LOD                           | Masowe renderowanie powtarzalnych obiektów z poziomami szczegółowości.             | Instancing dla wielu obiektów oraz co najmniej dwa poziomy LOD zależne od odległości.        | Rafa, kamienie, koralowce, ławice ryb, kępy roślin.                        |
| A08 | Procedural coral or seaweed generation                 | Proceduralne tworzenie rafy, koralowców lub wodorostów.                            | Parametryczny generator geometrii albo instancji; widoczna różnorodność kształtu i rozmiaru. | Autorska rafa lub las wodorostów generowany z parametrów.                  |
| A09 | Ray-marched SDF object                                 | Obiekt proceduralny renderowany metodą ray marchingu i signed distance fields.     | Co najmniej jeden nietrywialny obiekt SDF z normalnymi i oświetleniem.                       | Meduza, organiczna forma, skała, artefakt lub świetlisty organizm.         |
| A10 | Skeletal animation or vertex-shader swimming animation | Animacja stworzenia przez skinning albo deformację wierzchołków.                   | Co najmniej jedno pływające stworzenie z animacją ciała/płetw albo kości.                    | Ryba, płaszczka, ośmiornica lub inny organizm morski.                      |
| A11 | Parallax occlusion mapping                             | Pozornie trójwymiarowa powierzchnia przez próbkowanie mapy wysokości.              | Steep/parallax occlusion mapping w przestrzeni stycznej, z kontrolą skali wysokości.         | Piasek, skały, zardzewiały wrak, relief rafy.                              |
| A12 | Depth-aware soft particles                             | Cząstki miękko zanikające przy przecięciach z geometrią na podstawie bufora głębi. | Odczyt głębi sceny i modulacja alfa cząstki zależnie od różnicy głębokości.                  | Osad, chmury piasku, bąble i pył wodny bez twardych przecięć.              |
| A13 | Environment cubemap reflections/refractions            | Odbicia i załamania z cubemapy z efektem Fresnela.                                 | Skybox/cubemap oraz shader z reflect/refract i mieszaniem zależnym od kąta widzenia.         | Szyba, metalowy hełm, perły, bańki powietrza, powierzchnie refleksyjne.    |
| A14 | Flow-map-driven underwater current distortion          | Zniekształcenia UV sterowane mapą przepływu.                                       | Flow mapa kontrolująca kierunek/prędkość przesuwania tekstur lub normal map.                 | Prądy wodne zniekształcające światło, rośliny lub plankton.                |


## **4.2 Lista B — metody mniej zaawansowane**

Tabela 2 Metody mniej zaawansowane do wyboru.


| ID  | Metoda                                         | Opis                                                                                           | Minimum implementacyjne                                                                                                    | Motyw podwodny                                                                    |
| --- | ---------------------------------------------- | ---------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| B01 | CPU-side particle system for bubbles           | System cząstek aktualizowany po stronie CPU.                                                   | Pozycja, prędkość, czas życia, respawn i blending cząstek.                                                                 | Bąbelki z nurka, batyskafu lub komina hydrotermalnego.                            |
| B02 | Billboarded plankton / sea snow                | Billboardy zawsze zwrócone do kamery.                                                          | Cząstki/billboardy z przezroczystością i prostym ruchem.                                                                   | Plankton, drobiny organiczne, podwodny pył.                                       |
| B03 | Animated procedural caustic texture            | Prostsza animowana tekstura kaustyk bez pełnej symulacji.                                      | Animacja przez przesuwanie, blendowanie lub sekwencję tekstur.                                                             | Światło falujące na dnie i ścianach wraku.                                        |
| B04 | Fish or submarine movement along a spline path | Obiekt porusza się po krzywej z orientacją zgodną z trajektorią.                               | Spline orientacja z kwaternionów lub ramek transportu równoległego.                                                        | Ryba, łódź podwodna albo kamera poruszająca się po ścieżce.                       |
| B05 | Seaweed/anemone waving in vertex shader        | Falowanie roślinności w shaderze wierzchołków.                                                 | Deformacja sinus/noise zależna od wysokości, czasu i kierunku prądu.                                                       | Wodorosty, ukwiały, miękkie koralowce.                                            |
| B06 | Interactive object picking                     | Wybór obiektu myszą lub celownikiem przez ray casting.                                         | Rzut promienia z ekranu do świata i reakcja wybranego obiektu.                                                             | Zbieranie skarbu, aktywacja lamp, otwieranie skrzyni.                             |
| B07 | Heightmap-based seabed mesh                    | Siatka dna generowana z mapy wysokości.                                                        | Wczytanie heightmapy i wygenerowanie wierzchołków, normalnych i UV.                                                        | Nierówne dno, wydmy piaskowe, skały.                                              |
| B08 | Boids algorithm / fish-school flocking         | Symulacja zachowania grupowego agentów przez lokalne reguły separacji, wyrównania i spójności. | Co najmniej 20–30 agentów/ryb aktualizowanych w czasie rzeczywistym, z regulowanym promieniem sąsiedztwa lub wagami reguł. | Ławica ryb lub małych organizmów reagująca na przeszkody, kamerę albo prąd wodny. |
| B09 | Loading and displaying OBJ models              | Wczytywanie zewnętrznych modeli z materiałami.                                                 | Co najmniej kilka modeli OBJ z teksturami i transformacjami.                                                               | Wrak, skały, koralowce, ryby, artefakty.                                          |
| B10 | Depth-based fog and color attenuation          | Mgła i tłumienie koloru zależne od odległości lub głębokości.                                  | Linear/exponential fog w shaderze, z parametrami kontrolnymi.                                                              | Niebiesko-zielone zanikanie obiektów w oddali.                                    |
| B11 | UI panel for controlling scene parameters      | Panel UI do sterowania parametrami sceny.                                                      | Suwaki/przełączniki dla światła, mgły, PBR, kamery lub debug views.                                                        | Regulacja prądu, gęstości mgły, intensywności reflektora.                         |
| B12 | Transparent objects and blending               | Przezroczystość z sortowaniem lub kontrolą kolejności renderowania.                            | Poprawne glBlendFunc i rysowanie przezroczystych obiektów po nieprzezroczystych.                                           | Bańki, szyby, meduzy, półprzezroczyste rośliny.                                   |
| B13 | Moving point lights or submarine headlights    | Ruchome światła punktowe albo reflektory.                                                      | Co najmniej jedno źródło światła sterowane lub animowane w czasie.                                                         | Latarka nurka, reflektor batyskafu, bioluminescencja.                             |
| B14 | Simple creature animation state machine        | Prosta maszyna stanów zachowania stworzenia.                                                   | Co najmniej trzy stany, np. idle, swim, flee, i przejścia między nimi.                                                     | Ryba uciekająca przed kamerą, meduza pulsująca, krab patrolujący teren.           |


# **5 Zakres sceny i interakcji**

Scena powinna być wizualnie spójna i jednoznacznie podwodna. Oczekiwane są elementy takie jak dno, skały, roślinność, ryby, koralowce, bąble, osad, wrak, ruiny, batyskaf, nurek, światła, mgła głębiowa, kaustyki albo inne zjawiska środowiska wodnego. Nie trzeba implementować wszystkich elementów; ważniejsza jest spójna kompozycja i wyraźna demonstracja metod.

·        Interakcja musi wpływać na scenę, obiekty, światła, parametry środowiska albo zachowanie stworzeń.  
·        Sterowanie kamerą jest wymagane, ale samo sterowanie kamerą nie spełnia wymogu interakcji.  
·        W README należy wypisać wszystkie klawisze, kontrolki myszy i elementy panelu UI.

# **6 Rezultaty do oddania**

1       Działający kod źródłowy C++/OpenGL/GLSL.  
2       Repozytorium GitHub, publiczne albo prywatne udostępnione prowadzącemu.  
3       Plik README.md z tytułem projektu, składem grupy, wybranymi metodami A/B, listą metod obowiązkowych, instrukcją budowania, opisem sterowania oraz zrzutami ekranu.  
4       Komplet shaderów, modeli, tekstur i plików konfiguracyjnych potrzebnych do uruchomienia projektu.  
5       Krótka prezentacja maksymalnie 5 minut podczas terminu egzaminu.

# **7 Prezentacja podczas egzaminu**

Prezentacja trwa maksymalnie 5 minut. Celem prezentacji jest pokazanie działającej aplikacji, a nie omawianie slajdów teoretycznych. Grupa powinna przygotować demonstrację w taki sposób, aby wszystkie wymagane metody były widoczne w krótkim czasie.

·        20–30 sekund: tytuł, skład grupy i krótki opis sceny.  
·        1–2 minuty: demonstracja metod obowiązkowych.  
·        1–2 minuty: demonstracja wybranych metod A i B.  
·        30–60 sekund: pokaz interakcji i parametrów sterujących.  
·        krótko: największy problem techniczny i sposób jego rozwiązania.

# **8 Kryteria oceny**

Maksymalna liczba punktów: 100


| Kryterium                           | Punkty | Opis                                                                                                                                                                                           |
| ----------------------------------- | ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Metody obowiązkowe                  | 30     | Poprawna implementacja normal mapping, PBR, kamery kwaternionowej, shadow mapping, Parallel Transport Frames oraz underwater skybox/cubemap; każda metoda musi być widoczna i zademonstrowana. |
| Metoda zaawansowana z listy A       | 30     | Jakość techniczna, integracja z pipeline, parametr kontrolny/debug i widoczny wpływ na scenę.                                                                                                  |
| Metoda mniej zaawansowana z listy B | 15     | Poprawna implementacja i sensowne wykorzystanie w scenie podwodnej.                                                                                                                            |
| Spójność wizualna sceny podwodnej   | 25     | Kompozycja, materiały, światło, skala, atmosfera i czytelność efektów.                                                                                                                         |


# **9 Arkusz zapisów metod**

Prowadzący udostępni arkusz Google Sheets: [GRK projekty 2026  Google Sheets](https://docs.google.com/spreadsheets/d/117W8I0gTl0tWk9_6cbQuPufUh-A6LNW32k1oy5Jtmis/edit?gid=0#gid=0)

·        MetodyAzaawansowane: ID, nazwa, opis, limit grup, aktualna liczba zapisów.  
·        MetodyBmniejzaawansowane: ID, nazwa, opis, limit grup, aktualna liczba zapisów.  
·        Kombinacje: wszystkie dostępne pary A+B, status, numer grupy.  
·        Zgloszeniagrup: numer grupy, student 1–3, wybrane ID/nazwy metod, kombinacja, timestamp, GitHub URL, uwagi prowadzącego, status prezentacji.

## **Przykładowe kolumny w zakładce Zgloszeniagrup:**

Numer grupy, Student 1, Student 2, Student 3, ID metody A, Nazwa metody A, ID metody B, Nazwa metody B, Kombinacja, Timestamp, GitHub URL, Uwagi, Status prezentacji

# **10 Materiały pomocnicze i źródła do metod**

Poniższe źródła są punktami startowymi. Student może korzystać z innych materiałów, ale implementacja musi być samodzielnie zintegrowana z kodem projektu i zrozumiana podczas prezentacji.

## **Metody obowiązkowe**

### **Normal mapping**

**Opis metody:** Normal mapping pozwala uzyskać wrażenie drobnych wypukłości i detali powierzchni bez zwiększania liczby trójkątów, przez modyfikację normalnych w shaderze na podstawie tekstury mapy normalnych.

Materiały pomocnicze:

·        [LearnOpenGL — Normal Mapping](https://learnopengl.com/Advanced-Lighting/Normal-Mapping)  
·        [opengl-tutorial.org — Tutorial 13: Normal Mapping](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/)  
·        [Rastertek — OpenGL 4 Normal Mapping](https://www.rastertek.com/gl4linuxtut20.html)

### **PBR lighting**

**Opis metody:** PBR lighting, czyli physically based rendering, modeluje oświetlenie materiałów w sposób bardziej zgodny z fizyką światła, zwykle przez parametry takie jak albedo, metallic, roughness i Fresnel.

Materiały pomocnicze:

·        [LearnOpenGL — Physically Based Rendering](https://learnopengl.com/PBR)  
·        [Rastertek — Physically Based Rendering](https://www.rastertek.com/gl4linuxtut52.html)  
·        [Moving Frostbite to Physically Based Rendering](https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)

### **Quaternion camera control**

**Opis metody:** Sterowanie kamerą za pomocą kwaternionów pozwala wykonywać płynne obroty w przestrzeni 3D bez problemu gimbal lock, który występuje przy naiwnym użyciu kątów Eulera.

Materiały pomocnicze:

·        [opengl-tutorial.org — Tutorial 17: Quaternions](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/)  
·        [Song Ho Ahn — OpenGL Camera](https://www.songho.ca/opengl/gl_camera.html)  
·        [GLM Manual](https://github.com/g-truc/glm/blob/master/manual.md)

### **Shadow mapping**

**Opis metody:** Shadow mapping polega na wyrenderowaniu sceny z punktu widzenia źródła światła do mapy głębi, a następnie użyciu tej mapy do określenia, które fragmenty sceny znajdują się w cieniu.

Materiały pomocnicze:

·        [LearnOpenGL — Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)  
·        [opengl-tutorial.org — Tutorial 16: Shadow Mapping](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/)  
·        [LearnOpenGL — Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)

### **Parallel Transport Frames**

**Opis metody:** Parallel Transport Frames służą do stabilnego wyznaczania lokalnych ramek orientacji wzdłuż krzywej, np. dla ruchu po splajnie, generowania tunelu, przewodu, macki lub trajektorii ryby, bez niepożądanych skrętów charakterystycznych dla ramek Freneta.

Materiały pomocnicze:

·        [Parallel transport on curve — giordi91](https://giordi91.github.io/post/2018-31-07-parallel-transport/)  
·        [SideFX / Entagma — Parallel Transport](https://www.sidefx.com/tutorials/parallel-transport/)

### **Underwater skybox/cubemap**

Opis metody: Underwater skybox/cubemap wykorzystuje cubemapę jako tło otaczające scenę, aby stworzyć iluzję głębokiej wody, odległej rafy lub podwodnego horyzontu bez przesuwania tła razem z kamerą.

Materiały pomocnicze:

·        [LearnOpenGL — Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)  
·        [OGLDev — SkyBox](https://ogldev.org/www/tutorial25/tutorial25.html)

## **Lista A — źródła do metod zaawansowanych**

### **A01 Volumetric light shafts**

**Opis metody:** Volumetric light shafts symulują widoczne smugi światła rozpraszającego się w wodzie, dzięki czemu scena lepiej oddaje głębię, zawiesinę i podwodną atmosferę.

Materiały pomocnicze:

·        [GPU Gems 3 — Volumetric Light Scattering as a Post-Process](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-13-volumetric-light-scattering-post-process)  
·        [NVIDIA Volumetric Lighting](https://developer.nvidia.com/volumetriclighting)

### **A02 Caustics**

**Opis metody:** Caustics polega na generowaniu lub projektowaniu jasnych wzorów świetlnych powstających po załamaniu światła na falującej powierzchni wody i widocznych np. na dnie, skałach lub wraku.

Materiały pomocnicze:

·        [GPU Gems 1 — Rendering Water Caustics](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-2-rendering-water-caustics)  
·        [OpenGL archive — Underwater Caustics](https://www.opengl.org/archives/resources/code/samples/mjktips/caustics/)

### **A03 Gerstner waves**

**Opis metody:** Fale Gerstnera pozwalają animować powierzchnię wody przez fizycznie inspirowane przemieszczenie wierzchołków i dynamiczne normalne, tworząc przekonujące fale w czasie rzeczywistym.

Materiały pomocnicze:

·        [GPU Gems 1 — Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models)  
·        [Catlike Coding — Waves](https://catlikecoding.com/unity/tutorials/flow/waves/)

### **A04 Transform-feedback GPU particles**

**Opis metody:** Transform feedback umożliwia aktualizowanie położenia i parametrów cząstek bezpośrednio na GPU, co pozwala tworzyć wydajne systemy bąbelków, planktonu lub zawiesiny.

Materiały pomocnicze:

·        [Open.GL — Transform Feedback](https://open.gl/feedback)  
·        [OGLDev — Particle System using Transform Feedback](https://ogldev.org/www/tutorial28/tutorial28.html)

### **A05 Deferred shading**

**Opis metody:** Deferred shading rozdziela renderowanie geometrii od obliczania oświetlenia, zapisując dane sceny do G-buffera i umożliwiając wydajne użycie wielu świateł w scenie podwodnej.

Materiały pomocnicze:

·        [LearnOpenGL — Deferred Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)  
·        [OGLDev — Deferred Shading Part 1](https://ogldev.org/www/tutorial35/tutorial35.html)

### **A06 Screen-space refraction/reflection**

**Opis metody:** Screen-space refraction/reflection wykorzystuje wyrenderowany obraz sceny jako teksturę wejściową, aby uzyskać zniekształcone odbicia lub załamania światła na wodzie, szybie, bańkach albo innych przezroczystych obiektach.

Materiały pomocnicze:

·        [LearnOpenGL — Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)  
·        [LearnOpenGL — Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)

### **A07 Instanced rendering with LOD**

**Opis metody:** Instanced rendering z LOD pozwala wydajnie rysować wiele podobnych obiektów, takich jak koralowce, skały, ryby lub rośliny, dobierając poziom szczegółowości zależnie od odległości od kamery.

Materiały pomocnicze:

·        [LearnOpenGL — Instancing](https://learnopengl.com/Advanced-OpenGL/Instancing)  
·        [opengl-tutorial.org — Particles / Instancing](https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/)

### **A08 Procedural coral/seaweed**

**Opis metody:** Proceduralne generowanie koralowców lub wodorostów tworzy wiele zróżnicowanych form roślinności i rafy na podstawie parametrów, szumu, reguł lub instancjonowania zamiast ręcznego modelowania każdego elementu.

Materiały pomocnicze:

·        [GPU Gems 1 — Rendering Countless Blades of Waving Grass](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-7-rendering-countless-blades-waving-grass)  
·        [GPU Gems 2 — Toward Photorealism in Virtual Botany](https://developer.nvidia.com/gpugems/gpugems2/part-i-geometric-complexity/chapter-1-toward-photorealism-virtual-botany)

### **A09 Ray-marched SDF object**

**Opis metody:** Ray-marched SDF object renderuje obiekt opisany funkcją odległości w shaderze fragmentów, pozwalając tworzyć proceduralne kształty, miękkie połączenia i nietypowe formy organiczne bez klasycznej siatki trójkątów.

Materiały pomocnicze:

·        [Cambridge — Ray Marching and Signed Distance Fields](https://www.cl.cam.ac.uk/teaching/1718/FGraphics/1.%20Ray%20Marching%20and%20Signed%20Distance%20Fields.pdf)  
·        [Maxime Heckel — Painting with Math: Raymarching](https://blog.maximeheckel.com/posts/painting-with-math-a-gentle-study-of-raymarching/)  
·        [Inigo Quilez — 3D SDF primitives](https://iquilezles.org/articles/distfunctions/)

### **A10 Skeletal/swimming animation**

**Opis metody:** Skeletal lub shaderowa animacja pływania pozwala poruszać stworzeniem morskim przez deformację siatki, kości albo funkcję falującą, tak aby obiekt nie przesuwał się jak sztywna bryła.

Materiały pomocnicze:

·        [LearnOpenGL — Skeletal Animation](https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation)  
·        [GPU Gems 1 — Animation in the Dawn Demo](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-4-animation-dawn-demo)

### **A11 Parallax occlusion mapping**

**Opis metody:** Parallax occlusion mapping symuluje głębię nierówności powierzchni na podstawie mapy wysokości, dzięki czemu piasek, skały lub fragmenty wraku wyglądają przestrzennie bez rzeczywistego zagęszczania geometrii.

Materiały pomocnicze:

·        [LearnOpenGL — Parallax Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping)  
·        [opengl-tutorial.org — Normal Mapping](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/)

### **A12 Depth-aware soft particles**

**Opis metody:** Depth-aware soft particles wygaszają cząstki w miejscach przecięcia z geometrią sceny na podstawie bufora głębi, dzięki czemu dym, piasek, plankton lub bąbelki nie tworzą ostrych krawędzi kontaktu.

Materiały pomocnicze:

·        [opengl-tutorial.org — Particles / Instancing](https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/)  
·        [LearnOpenGL — Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)  
·        [NVIDIA — Soft Particles whitepaper](https://developer.download.nvidia.com/whitepapers/2007/SDK10/SoftParticles_hi.pdf)

### **A13 Environment cubemap reflections/refractions**

**Opis metody:** Environment cubemap reflections/refractions wykorzystują teksturę otoczenia do obliczania odbić i załamań na powierzchniach takich jak szkło batyskafu, metal, woda lub pęcherzyki powietrza.

Materiały pomocnicze:

·        [LearnOpenGL — Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)  
·        [OGLDev — SkyBox](https://ogldev.org/www/tutorial25/tutorial25.html)  
·        [Scratchapixel — Reflection, Refraction and Fresnel](https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel.html)

### **A14 Flow-map current distortion**

**Opis metody:** Flow-map current distortion używa tekstury wektorowego przepływu do przesuwania współrzędnych UV lub normalnych, tworząc wrażenie prądów wodnych wpływających na światło, roślinność albo powierzchnie.

Materiały pomocnicze:

·        [Graphics Runner — Animating Water Using Flow Maps](https://graphicsrunner.blogspot.com/2010/08/water-using-flow-maps.html)  
·        [mtnphil — Water Flow Shader](https://mtnphil.wordpress.com/2012/08/25/water-flow-shader/)  
·        [Catlike Coding — Directional Flow](https://catlikecoding.com/unity/tutorials/flow/directional-flow/)

## **Lista B — źródła do metod mniej zaawansowanych**

### **B01 CPU particles**

**Opis metody:** CPU-side particle system aktualizuje cząstki po stronie procesora, przechowując ich pozycję, prędkość, kolor i czas życia, a następnie renderuje je jako np. bąbelki powietrza.

Materiały pomocnicze:

·        [LearnOpenGL — Particles](https://learnopengl.com/In-Practice/2D-Game/Particles)  
·        [opengl-tutorial.org — Particles / Instancing](https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/)

### **B02 Billboarded plankton**

**Opis metody:** Billboarded plankton renderuje małe półprzezroczyste cząstki lub płaszczyzny zawsze zwrócone do kamery, tworząc efekt planktonu, pyłu lub morskiego śniegu.

Materiały pomocnicze:

·        [opengl-tutorial.org — Billboards & Particles](https://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/)  
·        [LearnOpenGL — Blending](https://learnopengl.com/Advanced-OpenGL/Blending)

### **B03 Animated caustic texture**

**Opis metody:** Animated caustic texture polega na animowaniu lub mieszaniu tekstur jasnych wzorów kaustycznych rzutowanych na dno i obiekty, aby zasymulować migotanie światła pod wodą.

Materiały pomocnicze:

·        [GPU Gems 1 — Rendering Water Caustics](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-2-rendering-water-caustics)  
·        [OpenGL archive — Underwater Caustics](https://www.opengl.org/archives/resources/code/samples/mjktips/caustics/)

### **B04 Spline movement**

**Opis metody:** Spline movement prowadzi rybę, łódź podwodną, kamerę lub inny obiekt po krzywej, zapewniając gładki ruch i stabilną orientację wzdłuż zadanej trajektorii.

Materiały pomocnicze:

·        [opengl-tutorial.org — Quaternions](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/)  
·        [Parallel transport on curve — giordi91](https://giordi91.github.io/post/2018-31-07-parallel-transport/)

### **B05 Vertex shader waving**

**Opis metody:** Vertex shader waving animuje wierzchołki roślin, ukwiałów lub wodorostów funkcjami sinusoidalnymi, szumem albo parametrami prądu wodnego, bez zmiany siatki po stronie CPU.

Materiały pomocnicze:

·        [GPU Gems 1 — Waving Grass](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-7-rendering-countless-blades-waving-grass)  
·        [mtnphil — Wind animations for vegetation](https://mtnphil.wordpress.com/2011/10/18/wind-animations-for-vegetation/)

### **B06 Object picking**

**Opis metody:** Object picking pozwala użytkownikowi wskazywać obiekty myszą lub kursorem, np. aby aktywować mechanizm, zebrać przedmiot albo zmienić zachowanie elementu sceny.

Materiały pomocnicze:

·        [opengl-tutorial.org — Picking with Ray-OBB](https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/)  
·        [LearnOpenGL — Camera](https://learnopengl.com/Getting-started/Camera)

### **B07 Heightmap seabed**

**Opis metody:** Heightmap seabed generuje siatkę dna morskiego z obrazu wysokości, dzięki czemu można łatwo uzyskać pagórki, rowy, nierówności i podstawową topografię sceny.

Materiały pomocnicze:

·        [LearnOpenGL — Height Map](https://learnopengl.com/Guest-Articles/2021/Tessellation/Height-map)  
·        [LearnOpenGL — Textures](https://learnopengl.com/Getting-started/Textures)

### **B08 Boids algorithm / fish-school flocking**

Opis metody: Boids to algorytm symulacji zachowań grupowych, w którym z prostych lokalnych reguł separacji, wyrównania i spójności powstaje realistyczny ruch ławicy ryb lub stada.

Materiały pomocnicze:

·        [Craig Reynolds — Boids (Flocks, Herds, and Schools)](https://www.red3d.com/cwr/boids/)  
·        [Craig Reynolds — Flocks, Herds, and Schools: A Distributed Behavioral Model](https://www.cs.toronto.edu/~dt/siggraph97-course/cwr87/)  
·        [OpenSteer — Documentation](https://opensteer.sourceforge.net/doc.html)

### **B09 OBJ model loading**

**Opis metody:** OBJ model loading umożliwia wczytanie zewnętrznych modeli 3D, takich jak ryby, koralowce, wrak lub batyskaf, wraz z geometrią i teksturami potrzebnymi do sceny.

Materiały pomocnicze:

·        [LearnOpenGL — Model Loading](https://learnopengl.com/Model-Loading/Model)  
·        [Assimp — official repository](https://github.com/assimp/assimp)

### **B10 Depth fog**

**Opis metody:** Depth fog miesza kolor obiektu z kolorem środowiska zależnie od odległości od kamery lub głębokości, tworząc efekt zanikania widoczności w wodzie.

Materiały pomocnicze:

·        [Rastertek — Fog](https://www.rastertek.com/gl4linuxtut26.html)  
·        [GPU Gems 3 — Volumetric Light Scattering](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-13-volumetric-light-scattering-post-process)

### **B11 UI panel**

**Opis metody:** UI panel pozwala w czasie działania programu sterować parametrami sceny, takimi jak światła, mgła, kolory materiałów, intensywność efektów lub tryby debugowania.

Materiały pomocnicze:

·        [Dear ImGui — repository](https://github.com/ocornut/imgui)  
·        [ImGui GLFW  OpenGL3 example](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp)

### **B12 Transparency**

**Opis metody:** Transparency polega na poprawnym renderowaniu obiektów półprzezroczystych z użyciem blendingu, sortowania i kontroli zapisu do bufora głębi.

Materiały pomocnicze:

·        [LearnOpenGL — Blending](https://learnopengl.com/Advanced-OpenGL/Blending)  
·        [opengl-tutorial.org — Transparency](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-10-transparency/)

### **B13 Moving lights**

**Opis metody:** Moving lights wprowadzają animowane lub sterowane przez użytkownika źródła światła, np. reflektory batyskafu albo latarkę nurka, które dynamicznie wpływają na scenę.

Materiały pomocnicze:

·        [LearnOpenGL — Basic Lighting](https://learnopengl.com/Lighting/Basic-Lighting)  
·        [LearnOpenGL — Light Casters](https://learnopengl.com/Lighting/Light-casters)

### **B14 State machine**

**Opis metody:** State machine organizuje zachowanie stworzenia lub obiektu w przejrzyste stany, np. idle, swim, flee lub follow, oraz określa przejścia między nimi zależnie od interakcji i warunków sceny.

Materiały pomocnicze:

·        [LearnOpenGL — Skeletal Animation](https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation)  
·        [Game Programming Patterns — State](https://gameprogrammingpatterns.com/state.html)  