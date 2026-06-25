#version 410 core
//
// postprocess.frag - OLE-07: post-processing podwodny.
//
// Efekty:
//   1. Niebiesko-zielony tint (kolorystyka podwodna)
//   2. Mgla zalezna od glebi (wykladnicza, niebiesko-zielona)
//   3. Lekka aberracja chromatyczna na brzegach (symuluje soczewke wody)
//   4. Vignette (przyciemnienie na brzegach)
//

in vec2 uv;

uniform sampler2D screenTexture;   // kolor sceny
uniform sampler2D depthTexture;    // bufor glebi sceny

uniform vec3  underwaterTint;      // kolor tintu podwodnego
uniform float tintStrength;        // sila tintu (0 = brak, 1 = pelny)
uniform float depthFogDensity;     // gestosc mgly glebi
uniform float depthFogStart;       // odleglosc startowa mgly (linearyzowana glebia)
uniform vec3  depthFogColor;       // kolor mgly (niebieski-zielony)

uniform float nearPlane;           // near plane kamery
uniform float farPlane;            // far plane kamery

uniform float chromaticStrength;   // sila aberracji chromatycznej
uniform float vignetteStrength;    // sila vignette

uniform float time;                // czas (animacja)

out vec4 fragColor;

// Linearyzacja glebi z perspektywicznego bufora Z
float linearizeDepth(float d)
{
    float z = d * 2.0 - 1.0; // [0,1] -> [-1,1] NDC
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    // --- 1. Probka koloru sceny ---
    vec3 sceneColor = texture(screenTexture, uv).rgb;

    // --- 2. Glebia ---
    float rawDepth = texture(depthTexture, uv).r;
    float linDepth = linearizeDepth(rawDepth);
    // Normalizacja glebi do [0,1] (0=blisko, 1=daleko)
    float depthNorm = clamp((linDepth - nearPlane) / (farPlane - nearPlane), 0.0, 1.0);

    // --- 3. Mgla zalezna od glebi (wykladnicza) ---
    float fogFactor = 1.0 - exp(-depthFogDensity * max(linDepth - depthFogStart, 0.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 color = mix(sceneColor, depthFogColor, fogFactor);

    // --- 4. Niebiesko-zielony tint podwodny ---
    color = mix(color, color * underwaterTint, tintStrength);

    // --- 5. Aberracja chromatyczna (na brzegach ekranu) ---
    if (chromaticStrength > 0.001)
    {
        vec2 center = uv - 0.5;
        float distFromCenter = length(center);
        // Sila aberracji rosnie z odlegloscia od centrum
        float aberration = chromaticStrength * distFromCenter * distFromCenter;
        vec2 dir = normalize(center + 0.0001);

        float r = texture(screenTexture, uv + dir * aberration).r;
        float g = sceneColor.g; // zielony bez przesuniecia
        float b = texture(screenTexture, uv - dir * aberration).b;

        // Aberracja dolozana do koloru po tincie (blend z oryginalem)
        vec3 aberrated = vec3(r, g, b);
        aberrated = mix(aberrated, aberrated * underwaterTint, tintStrength);
        aberrated = mix(aberrated, depthFogColor, fogFactor);
        color = mix(color, aberrated, distFromCenter * 2.0);
    }

    // --- 6. Vignette (przyciemnienie na brzegach) ---
    if (vignetteStrength > 0.001)
    {
        vec2 vig = uv * (1.0 - uv);
        float vignette = vig.x * vig.y * 16.0;
        vignette = pow(vignette, vignetteStrength);
        color *= vignette;
    }

    fragColor = vec4(color, 1.0);
}
