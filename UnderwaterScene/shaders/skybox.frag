#version 410 core
// skybox.frag - podwodny skybox (NED-05, metoda obowiazkowa M6).
//
// Efekty:
//   - Gradient kierunkowy: gora jasniejsza (swiatlo slonca z powierzchni),
//     dol ciemniejszy (glebina). Spelnia wymog "gora jasniejsza, dol ciemniejszy".
//   - Animowane kaustyki na gornej polkuli: sinus po czasie i kierunku,
//     symuluje swiatlo filtrowane przez fale na powierzchni.
//   - waterTint moduluje caly wynik dla spojnego koloru podwodnego.

in vec3 texDir;

uniform samplerCube skybox;
uniform vec3  waterTint;    // kolor wody z panelu ImGui (rgb)
uniform float time;         // glfwGetTime(), do animacji kaustyk

out vec4 fragColor;

void main()
{
    vec3 dir = normalize(texDir);

    // --- probka cubemapy ---
    vec3 cubeColor = texture(skybox, dir).rgb;

    // --- gradient glebinowy ---
    // dir.y w [-1, 1]: +1 = prosto w gore (ku powierzchni), -1 = prosto w dol (glebina).
    // smoothstep daje gladkie przejscie bez ostrych krawedzi.
    float upFactor   = smoothstep(-1.0, 0.6, dir.y);   // 0 na dnie, 1 na gorze
    float depthDark  = mix(0.04, 1.0, upFactor);        // dol bardzo ciemny

    // Kolor wody: dol ciemno-granatowy, gora niebiesko-zielona (slonce przez wode).
    vec3 deepColor    = vec3(0.0, 0.02, 0.08);          // glebina - prawie czarna
    vec3 surfaceColor = waterTint * vec3(1.6, 1.4, 1.2); // powierzchnia - jasniejsza od tinta

    vec3 underwaterGrad = mix(deepColor, surfaceColor, upFactor);

    // --- animowane kaustyki (tylko gorna polkula) ---
    // Proste, tanie: kilka fal sinusoidalnych po wspolrzednych kierunku i czasie.
    float causticMask = smoothstep(0.0, 0.5, dir.y); // tylko gora
    float c1 = sin(dir.x * 8.0 + dir.z * 6.0 + time * 1.3);
    float c2 = sin(dir.x * 5.0 - dir.z * 9.0 + time * 0.9 + 1.5);
    float c3 = sin(dir.x * 12.0 + dir.z * 4.0 - time * 1.7 + 3.0);
    float caustic = ((c1 + c2 + c3) / 3.0 * 0.5 + 0.5); // [0,1]
    caustic = mix(1.0, 1.0 + caustic * 0.18, causticMask); // subtelne rozjasnienie

    // --- zlozenie ---
    vec3 result = cubeColor * underwaterGrad * depthDark * caustic;

    fragColor = vec4(result, 1.0);
}
