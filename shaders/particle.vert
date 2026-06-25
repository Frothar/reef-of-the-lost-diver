#version 410 core

// MRZ-07: babelki jako point sprites.
layout(location = 0) in vec4 posSize;   // xyz = pozycja w swiecie, w = rozmiar bazowy
layout(location = 1) in float alpha;

uniform mat4 view;
uniform mat4 projection;
uniform float sizeScale;   // skala rozmiaru punktu (zalezna od wysokosci okna)

out float vAlpha;

void main()
{
    vec4 viewPos = view * vec4(posSize.xyz, 1.0);
    gl_Position = projection * viewPos;

    // Perspektywiczne skalowanie: blizej = wieksze.
    float dist = max(-viewPos.z, 0.001);
    gl_PointSize = clamp(posSize.w * sizeScale / dist, 2.0, 48.0);

    vAlpha = alpha;
}
