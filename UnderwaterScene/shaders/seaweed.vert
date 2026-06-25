#version 410 core
//
// seaweed.vert - falowanie wodorostow pod woda (NED).
//
// Deformacja wierzcholkow w przestrzeni LOKALNEJ (przed macierza model):
//   - przesuniecie boczne (X i Z) zalezy od wysokosci wierzcholka (Y):
//     podstawa stoi nieruchomo, wierzcholki wygetkow machaja najsilniej.
//   - dwie niezalezne sinusoidy o roznych czestotliwosciach i fazach
//     daja naturalny, nieregularny ruch wodorostow.
//   - faza kazdej instancji (seedOffset) sprawia, ze kazdy wodorost
//     kolebie sie w innym momencie - nie robia tego synchronicznie.
//
// Fragment: wspolny pbr.frag - identyczne varyings co pbr.vert.
//

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Parametry animacji
uniform float time;
uniform float swayAmplitude; // max przesuniecie boczne wierzcholka (lok. jednostki)
uniform float swaySpeed;     // predkosc kolysania
uniform float swayFreq;      // czestotliwosc przestrzenna (ile fal na wysokosc)
uniform float minY;          // dolna granica modelu lokalnie
uniform float maxY;          // gorna granica modelu lokalnie
uniform float seedOffset;    // per-instancja offset fazy, by wodorosty nie byly synchroniczne

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    vec3 pos = vertexPosition;
    vec3 nrm = vertexNormal;

    float H = max(maxY - minY, 0.001);

    // Znormalizowana wysokosc w lokalnym ukladzie wspolrzednych
    float t = clamp((pos.y - minY) / H, 0.0, 1.0);

    // Szescian t: ruch powolny przy dole, gwaltowny na gorze (bardziej realistyczny)
    float factor = t * t * t;

    // Dwa przebiegi sinusoidy: glowne kolysanie (wolne) + drganie (szybkie i slabe)
    float phase1 = time * swaySpeed        + seedOffset;
    float phase2 = time * swaySpeed * 1.7f + seedOffset * 2.3f;

    float swayX = sin(phase1) * swayAmplitude * factor;
    float swayZ = sin(phase2) * swayAmplitude * 0.45f * factor;

    pos.x += swayX;
    pos.z += swayZ;

    // Korekta normalnej: pochyl normaln razem z wierzcholkiem (uproszczona)
    // - dodajemy skladowa skierowana wzdluz osi deformacji, proporcjonalna
    //   do pochodnej factor (czyli 3*t^2 / H)
    float dFactor = 3.0 * t * t / H;
    nrm.y -= cos(phase1) * swayAmplitude * swaySpeed * dFactor * 0.3;
    nrm = normalize(nrm);

    // Transformacja do przestrzeni swiata
    worldPos    = vec3(model * vec4(pos, 1.0));
    worldNormal = normalize(mat3(transpose(inverse(model))) * nrm);
    texCoord    = vertexTexCoord;

    vec3 T = normalize(mat3(model) * vertexTangent);
    vec3 B = normalize(mat3(model) * vertexBitangent);
    TBN = mat3(T, B, worldNormal);

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
