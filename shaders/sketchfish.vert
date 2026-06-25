#version 410 core
//
// sketchfish.vert - pływanie ryb Sketchfab (multi-mesh GLB) TĄ SAMĄ deformacją
// co fish.vert (NED-03, A10), ale z dodatkowym krokiem normalizacji.
//
// Modele Sketchfab mają surowe, ogromne i przesunięte współrzędne oraz różną oś
// ciała. Najpierw "preTransform" (localFix z C++) sprowadza wierzchołek do
// znormalizowanej przestrzeni ciała: wyśrodkowanej, z długą osią wzdłuż Z,
// przeskalowanej do bodyLength jednostek. Dopiero w tej przestrzeni robimy falę
// boczną (jak fish.vert), a potem "model" (ramka PTF splajnu) sadza rybę w świecie.
//
// Fragment: wspólny pbr.frag (te same out-y co pbr.vert).

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 model;         // ramka PTF splajnu (umiejscowienie w świecie)
uniform mat4 preTransform;  // localFix: normalizacja modelu do przestrzeni ciała
uniform mat4 view;
uniform mat4 projection;

// --- A10 parametry animacji (te same uniformy co fish.vert) ---
uniform float time;
uniform float waveAmplitude;   // amplituda fali (ułamek długości ciała)
uniform float waveFrequency;   // ile fal na długość ciała
uniform float waveSpeed;       // prędkość machania
uniform float bodyLength;      // długość ciała w Z po normalizacji (= targetSize)
uniform float finAmplitude;    // amplituda ruchu pionowego brzegów

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    // 1) Normalizacja do przestrzeni ciała (wyśrodkowana, oś ciała wzdłuż Z).
    vec3 pos = vec3(preTransform * vec4(vertexPosition, 1.0));
    mat3 preN = mat3(transpose(inverse(preTransform)));
    vec3 nrm = normalize(preN * vertexNormal);
    vec3 tan = preN * vertexTangent;
    vec3 bit = preN * vertexBitangent;

    float L = max(bodyLength, 0.0001);

    // 2) Fala ciała: przesunięcie X zależne od pozycji wzdłuż Z (jak fish.vert).
    //    zN w ~[-0.5, 0.5]; u w [0,1] (głowa->ogon); ogon macha najmocniej.
    float zN         = pos.z / L;
    float u          = clamp(zN + 0.5, 0.0, 1.0);
    float bodyFactor = smoothstep(0.0, 1.0, u);
    float phase      = zN * waveFrequency - time * waveSpeed;
    float sway       = sin(phase) * (waveAmplitude * L) * bodyFactor;
    pos.x += sway;

    // pochodna sway po pos.z (do analitycznej korekty normalnej)
    float bodyFactorDeriv = (6.0 * u - 6.0 * u * u);                  // d(smoothstep)/du
    float swayDeriv = waveAmplitude * (bodyFactorDeriv * sin(phase)
                                       + bodyFactor * cos(phase) * waveFrequency);
    nrm.z = nrm.z - swayDeriv * nrm.x;

    // 3) Płetwy/brzegi: lekki ruch pionowy mocniejszy przy brzegach |x|.
    float finPhase = zN * waveFrequency * 1.5 - time * waveSpeed * 1.7;
    float fin      = sin(finPhase) * (finAmplitude * L) * abs(pos.x);
    pos.y += fin;
    nrm = normalize(nrm);

    // 4) Umiejscowienie w świecie ramką PTF.
    worldPos    = vec3(model * vec4(pos, 1.0));
    worldNormal = normalize(mat3(transpose(inverse(model))) * nrm);
    texCoord    = vertexTexCoord;

    vec3 T = normalize(mat3(model) * tan);
    vec3 B = normalize(mat3(model) * bit);
    TBN = mat3(T, B, worldNormal);

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
