#version 410 core
//
// fish.vert - animacja plywania ryby (NED-03, METODA A10, 30 pkt).
//
// Deformacja ciala w przestrzeni LOKALNEJ (przed macierza model):
//   - fala boczna wzdluz osi Z (glowa przy z<=0, ogon ku +z), rosnaca ku ogonowi
//   - osobna sinusoida na pletwy (ruch w pionie, mocniejszy na brzegach ciala)
//   - normalna przeliczona ANALITYCZNIE z pochodnej deformacji (poprawne swiatlo)
//
// Fragment: uzywamy wspolnego pbr.frag (te same varyings co pbr.vert), zeby
// oswietlenie ryby bylo spojne z reszta sceny (OLE-01). Stad identyczny uklad
// out-ow: worldPos, worldNormal, texCoord, TBN.
//
// Vertex layout zgodny z Core::RenderContext::initFromAssimpMesh.

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// --- A10 parametry animacji (sterowalne uniformami / ImGui) ---
uniform float time;
uniform float waveAmplitude;   // amplituda fali bocznej (lokalne jednostki)
uniform float waveFrequency;   // ile fal przypada na dlugosc ciala
uniform float waveSpeed;       // predkosc machania
uniform float fishLength;      // dlugosc ryby w lokalnym Z (do znormalizowania fali)
uniform float finAmplitude;    // amplituda ruchu pletw (osobna sinusoida)

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    vec3 pos = vertexPosition;
    vec3 nrm = vertexNormal;

    float L = max(fishLength, 0.0001);

    // --- fala ciala: przesuniecie w X zalezne od Z i czasu ---
    // bodyFactor rosnie od glowy (0) do ogona (1) -> glowa stabilna, ogon macha najmocniej
    float u          = clamp(pos.z / L, 0.0, 1.0);
    float bodyFactor = smoothstep(0.0, 1.0, u);
    float phase      = pos.z * waveFrequency - time * waveSpeed;
    float sway       = sin(phase) * waveAmplitude * bodyFactor;

    // pochodna sway po z (regula iloczynu) - potrzebna do korekty normalnej
    float bodyFactorDeriv = (6.0 * u - 6.0 * u * u) / L;          // d/dz smoothstep(z/L)
    float swayDeriv = waveAmplitude * (bodyFactorDeriv * sin(phase)
                                       + bodyFactor * cos(phase) * waveFrequency);

    pos.x += sway;

    // --- pletwy: osobna, szybsza sinusoida, ruch w pionie mocniejszy przy brzegach |x| ---
    float finPhase = pos.z * waveFrequency * 1.5 - time * waveSpeed * 1.7;
    float fin      = sin(finPhase) * finAmplitude * abs(vertexPosition.x);
    pos.y += fin;

    // --- korekta normalnej po deformacji (analitycznie) ---
    // Deformacja x += sway(z) daje Jacobian, ktorego inverse-transpose przechyla
    // normalna w plaszczyznie X-Z: n.z' = n.z - swayDeriv * n.x.
    // Wplyw pletw (y zalezne od z) dokladamy podobnie dla skladowej z.
    nrm.z = nrm.z - swayDeriv * nrm.x;
    float finDeriv = cos(finPhase) * (waveFrequency * 1.5) * finAmplitude * abs(vertexPosition.x);
    nrm.z = nrm.z - finDeriv * nrm.y;
    nrm = normalize(nrm);

    // --- transformacja do swiata ---
    worldPos    = vec3(model * vec4(pos, 1.0));
    worldNormal = normalize(mat3(transpose(inverse(model))) * nrm);
    texCoord    = vertexTexCoord;

    vec3 T = normalize(mat3(model) * vertexTangent);
    vec3 B = normalize(mat3(model) * vertexBitangent);
    TBN = mat3(T, B, worldNormal);

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
