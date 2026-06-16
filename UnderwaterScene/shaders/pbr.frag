#version 410 core

// OLE-01/02/03: Cook-Torrance PBR + normal mapping (M1) w przestrzeni stycznej.
// Varyings musza byc zgodne z pbr.vert, fish.vert (NED-03) i jellyfish.vert (NED-06).

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;
in mat3 TBN;

uniform vec3 cameraPos;
uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3  albedo;
uniform float metallic;
uniform float roughness;

uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;       // OLE-03: normal map w przestrzeni stycznej

uniform bool useAlbedoMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAoMap;
uniform bool useNormalMap;          // OLE-03: flaga wlaczajaca normal mapping

uniform vec2 uvScale;

uniform vec3  fogColor;
uniform float fogDensity;

out vec4 fragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float rough)
{
    float a  = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float rough)
{
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, rough) * GeometrySchlickGGX(NdotL, rough);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ReinhardToneMapping(vec3 hdr)
{
    return hdr / (hdr + vec3(1.0));
}

void main()
{
    vec2 uv = texCoord * uvScale;

    vec3 albedoColor = albedo;
    if (useAlbedoMap)
        albedoColor = pow(texture(albedoMap, uv).rgb, vec3(2.2));

    float metallicVal = metallic;
    if (useMetallicMap)
        metallicVal = texture(metallicMap, uv).r;

    float roughnessVal = roughness;
    if (useRoughnessMap)
        roughnessVal = texture(roughnessMap, uv).r;

    float aoVal = 1.0;
    if (useAoMap)
        aoVal = texture(aoMap, uv).r;

    // --- OLE-03: Normal mapping (metoda obowiazkowa M1) ---
    // Jesli mapa normalnych jest aktywna, probkujemy ja, przemapowujemy z [0,1]
    // na [-1,1] i transformujemy przez macierz TBN (tangent space -> world space).
    // Bez mapy - uzywamy geometrycznej normalnej z vertex shadera.
    vec3 N;
    if (useNormalMap)
    {
        // Probka z normal mapy: wartosci [0,1] -> przestrzen styczna [-1,1]
        vec3 tangentNormal = texture(normalMap, uv).rgb * 2.0 - 1.0;
        // Transformacja z przestrzeni stycznej do swiata przez TBN
        N = normalize(TBN * tangentNormal);
    }
    else
    {
        N = normalize(worldNormal);
    }

    vec3 V = normalize(cameraPos - worldPos);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedoColor, metallicVal);

    // Zaburzona normalna N uzywa sie we WSZYSTKICH obliczeniach PBR
    float NDF = DistributionGGX(N, H, roughnessVal);
    float G   = GeometrySmith(N, V, L, roughnessVal);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicVal);

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedoColor / PI + specular) * lightColor * NdotL;

    vec3 ambient = vec3(0.03) * albedoColor * aoVal;
    vec3 color = ambient + Lo;

    float dist = length(cameraPos - worldPos);
    float fog = 1.0 - exp(-fogDensity * dist);
    color = mix(color, fogColor, clamp(fog, 0.0, 1.0));

    color = ReinhardToneMapping(color);
    color = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);
}
