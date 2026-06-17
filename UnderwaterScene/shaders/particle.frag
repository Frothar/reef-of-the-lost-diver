#version 410 core

in float vAlpha;

uniform vec3 bubbleColor;

out vec4 fragColor;

void main()
{
    // gl_PointCoord: (0,0) lewy-gorny, (1,1) prawy-dolny rog sprite'a.
    vec2 c = gl_PointCoord - vec2(0.5);
    float d = length(c);
    if (d > 0.5) discard;

    // Miekka krawedz + jasniejszy pierscien (wyglad banki powietrza).
    float disk = smoothstep(0.5, 0.32, d);
    float ring = smoothstep(0.5, 0.44, d) * 0.5;

    float a = vAlpha * disk;
    vec3 col = bubbleColor + ring;   // pierscien rozjasnia brzeg
    fragColor = vec4(col, a);
}
