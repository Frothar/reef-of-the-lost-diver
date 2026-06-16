#version 410 core

// OLE-01/02/03/04/05: Cook-Torrance PBR + normal mapping (M1) + shadow mapping (M4)
//                     + wiele swiatel (PointLight, SpotLight).
// Varyings musza byc zgodne z pbr.vert, fish.vert (NED-03) i jellyfish.vert (NED-06).

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;
in mat3 TBN;

uniform vec3 cameraPos;

// --- Swiatlo kierunkowe (slonce) ---
uniform vec3 lightDir;
uniform vec3 lightColor;

// --- Materialy PBR ---
uniform vec3  albedo;
uniform float metallic;
uniform float roughness;

uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;       // OLE-03

uniform bool useAlbedoMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAoMap;
uniform bool useNormalMap;          // OLE-03

uniform vec2 uvScale;

uniform vec3  fogColor;
uniform float fogDensity;

// --- OLE-04: Shadow mapping ---
uniform mat4 lightSpaceMatrix;
uniform sampler2D shadowMap;
uniform bool useShadows;

// --- OLE-05: Wiele swiatel ---
const int MAX_POINT_LIGHTS = 8;
const int MAX_SPOT_LIGHTS  = 4;

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;    // mnoznik jasnosci
    float constant;     // tlumienie: 1.0
    float linear;       // tlumienie: spadek liniowy
    float quadratic;    // tlumienie: spadek kwadratowy
};

struct SpotLight {
    vec3  position;
    vec3  direction;
    vec3  color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float innerCutoff;  // cos(kat wewnetrzny) - pelna jasnosc
    float outerCutoff;  // cos(kat zewnetrzny) - zanika do 0
};

uniform int       numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform int       numSpotLights;
uniform SpotLight  spotLights[MAX_SPOT_LIGHTS];

out vec4 fragColor;

const float PI = 3.14159265359;

// --- OLE-04: obliczenie wspolczynnika cienia ---
float ShadowCalculation(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 1.0;

    float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return 1.0 - shadow;
}

// --- Funkcje BRDF (wspolne dla wszystkich swiatel) ---
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

// --- OLE-05: obliczenie PBR dla jednego kierunku swiatla (reuzywalne) ---
vec3 calcPBR(vec3 N, vec3 V, vec3 L, vec3 radiance,
             vec3 albedoColor, float metallicVal, float roughnessVal, vec3 F0)
{
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughnessVal);
    float G   = GeometrySmith(N, V, L, roughnessVal);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 spec = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicVal);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedoColor / PI + spec) * radiance * NdotL;
}

// --- OLE-05: swiatlo punktowe z tlumieneim ---
vec3 calcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos,
                    vec3 albedoColor, float metallicVal, float roughnessVal, vec3 F0)
{
    vec3 L = normalize(light.position - fragPos);
    float dist = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    vec3 radiance = light.color * light.intensity * attenuation;
    return calcPBR(N, V, L, radiance, albedoColor, metallicVal, roughnessVal, F0);
}

// --- OLE-05: reflektor (spotlight) ze stozkiem + tlumienie ---
vec3 calcSpotLight(SpotLight light, vec3 N, vec3 V, vec3 fragPos,
                   vec3 albedoColor, float metallicVal, float roughnessVal, vec3 F0)
{
    vec3 L = normalize(light.position - fragPos);
    float dist = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    // Stozek reflektora: interpolacja miedzy inner a outer cutoff
    float theta   = dot(L, normalize(-light.direction));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float spotIntensity = clamp((theta - light.outerCutoff) / max(epsilon, 0.0001), 0.0, 1.0);

    vec3 radiance = light.color * light.intensity * attenuation * spotIntensity;
    return calcPBR(N, V, L, radiance, albedoColor, metallicVal, roughnessVal, F0);
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

    // OLE-03: Normal mapping
    vec3 N;
    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalMap, uv).rgb * 2.0 - 1.0;
        N = normalize(TBN * tangentNormal);
    }
    else
    {
        N = normalize(worldNormal);
    }

    vec3 V = normalize(cameraPos - worldPos);
    vec3 F0 = mix(vec3(0.04), albedoColor, metallicVal);

    // ===== Sumowanie radiancji Lo po wszystkich swiatach =====

    // 1) Swiatlo kierunkowe (slonce)
    vec3 L_dir = normalize(lightDir);
    vec3 Lo = calcPBR(N, V, L_dir, lightColor, albedoColor, metallicVal, roughnessVal, F0);

    // OLE-04: cienie wplywaja TYLKO na swiatlo kierunkowe
    if (useShadows)
    {
        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
        float shadowFactor = ShadowCalculation(fragPosLightSpace, N, L_dir);
        Lo *= shadowFactor;
    }

    // 2) OLE-05: swiatla punktowe
    for (int i = 0; i < numPointLights; ++i)
    {
        Lo += calcPointLight(pointLights[i], N, V, worldPos,
                             albedoColor, metallicVal, roughnessVal, F0);
    }

    // 3) OLE-05: reflektory (spotlighty)
    for (int i = 0; i < numSpotLights; ++i)
    {
        Lo += calcSpotLight(spotLights[i], N, V, worldPos,
                            albedoColor, metallicVal, roughnessVal, F0);
    }

    vec3 ambient = vec3(0.03) * albedoColor * aoVal;
    vec3 color = ambient + Lo;

    float dist = length(cameraPos - worldPos);
    float fog = 1.0 - exp(-fogDensity * dist);
    color = mix(color, fogColor, clamp(fog, 0.0, 1.0));

    color = ReinhardToneMapping(color);
    color = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);
}
