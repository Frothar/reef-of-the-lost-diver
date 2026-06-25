#version 410 core
// water_overlay.frag - pelnoekranowy podwodny overlay (lekka wersja OLE-07).
//
// Rysowany na samym koncu klatki z alpha blendingiem. Daje wrazenie zanurzenia:
//   - niebiesko-zielony tint na calym ekranie (mocniejszy przy krawedziach = winieta),
//   - delikatne falowanie/jasniejsze plamy (przesuwane w czasie),
//   - subtelne pojasnienie u gory ekranu (swiatlo z powierzchni).
//
// To NIE jest pelne post-processing OLE-07 (brak FBO/depth) - celowo tanie i
// nieinwazyjne, mozna pozniej zastapic wersja Olejnika z renderem do tekstury.

in vec2 uv;

uniform vec3  waterColor;       // spojny z reszta sceny (panel ImGui)
uniform float time;
uniform float overlayStrength;  // 0 = wylaczony, 1 = pelny

out vec4 fragColor;

void main()
{
    vec2 c = uv - 0.5;

    // Winieta: 0 w srodku, ~1 przy krawedziach.
    float vign = smoothstep(0.15, 0.8, length(c));

    // Delikatne, powolne falowanie swiatla pod woda.
    float ripple = sin((uv.x * 6.0 + uv.y * 4.0) + time * 0.6)
                 * sin((uv.y * 7.0 - uv.x * 3.0) - time * 0.4);
    ripple = ripple * 0.5 + 0.5;                 // [0,1]

    // Gora ekranu odrobine jasniejsza (swiatlo slonca z powierzchni).
    float topGlow = smoothstep(0.3, 1.0, uv.y) * 0.12;

    // Kolor i krycie: srodek lekko przejrzysty, brzegi mocniej zabarwione.
    vec3  tint  = waterColor * (0.85 + 0.30 * ripple) + topGlow;
    float alpha = mix(0.10, 0.55, vign) * overlayStrength;

    fragColor = vec4(tint, alpha);
}
