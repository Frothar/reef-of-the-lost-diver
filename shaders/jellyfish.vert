#version 410 core
//
// jellyfish.vert - pulsujaca meduza (NED-06).
//
// Deformacja w przestrzeni lokalnej, rozrozniana po znaku Y:
//   - DZWON (y >= 0): pulsowanie - promien (x,z) skaluje sie sinusem, wysokosc
//     odwrotnie (squash-stretch). Kontrakcja dzwonu = napped do gory.
//   - CZULKI (y < 0): kolysanie - boczne przesuniecie rosnace ku koncowi czulka,
//     z faza zalezna od kata, zeby czulki nie ruszaly sie identycznie.
//
// Fragment: wspolny pbr.frag (te same varyings co pbr.vert).

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform float pulseAmplitude;     // jak mocno dzwon pulsuje
uniform float pulseSpeed;
uniform float tentacleAmplitude;  // jak mocno koysza sie czulki
uniform float tentacleSpeed;
uniform float tentacleLength;     // do znormalizowania glebokosci czulka

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out mat3 TBN;

void main()
{
    vec3 pos = vertexPosition;
    float pulse = sin(time * pulseSpeed);

    if (pos.y >= 0.0)
    {
        // DZWON: promien rosnie/maleje, wysokosc przeciwnie (zachowanie objetosci)
        float s  = 1.0 + pulseAmplitude * pulse;
        float sy = 1.0 - 0.40 * pulseAmplitude * pulse;
        pos.x *= s;
        pos.z *= s;
        pos.y *= sy;
    }
    else
    {
        // CZULKI: kolysanie rosnace ku koncowi, faza zalezna od kata
        float depth  = clamp(-pos.y / max(tentacleLength, 0.0001), 0.0, 1.0);
        float tphase = atan(vertexPosition.z, vertexPosition.x); // per-czulek
        pos.x += sin(time * tentacleSpeed + depth * 4.0 + tphase) * tentacleAmplitude * depth;
        pos.z += cos(time * tentacleSpeed * 0.9 + depth * 4.0 + tphase) * tentacleAmplitude * depth;
    }

    worldPos    = vec3(model * vec4(pos, 1.0));
    worldNormal = normalize(mat3(transpose(inverse(model))) * vertexNormal);
    texCoord    = vertexTexCoord;

    vec3 T = normalize(mat3(model) * vertexTangent);
    vec3 B = normalize(mat3(model) * vertexBitangent);
    TBN = mat3(T, B, worldNormal);

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
