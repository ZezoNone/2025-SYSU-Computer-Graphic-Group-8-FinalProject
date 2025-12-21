#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
in mat3 TBN;           
in vec3 TangentViewPos; 
in vec3 TangentFragPos; 

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D metallic_roughnessMap;   
uniform sampler2D emissiveMap;
uniform sampler2D transmissionMap;
uniform sampler2D heightMap;
//gltf
uniform bool gltf;
//useMaterial?
uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicRoughnessMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAOMap;
uniform bool useEmissiveMap;
uniform bool useheightMap;
uniform bool useTransmissionMap;
//Factor
uniform vec4 materialBaseColor;
uniform vec3 materialEmissive;
uniform float materialMetallic;
uniform float materialRoughness;
uniform float materialTransmission;
//POM
uniform bool usePOM;
uniform float heightScale; 
uniform bool heightMapInvert;   
// glass
uniform bool isGlass;
uniform bool doubleSided;
// IBL
uniform samplerCube irradianceMap;   
uniform samplerCube prefilterMap;     
uniform sampler2D brdfLUT;    
uniform bool useIBL;
//
uniform vec3 environmentLight;
// point lights
#define MAX_POINT_LIGHTS 4
struct PointLightBase 
{
    vec3 position;
    vec3 color;
    vec3 direction;
    float cutOff;
    float far_plane;
};
uniform PointLightBase pointLightsBase[MAX_POINT_LIGHTS];
uniform samplerCube pointLightDepthCubemaps[MAX_POINT_LIGHTS];
uniform int pointLightCount;


// parallel lights
struct DirLight {
    vec3 direction; // 从光源指向场景的方向
    vec3 color;
    bool enabled;
};
uniform DirLight dirLight;

uniform vec3 camPos;

const float PI = 3.14159265359;

// 采样偏移数组
vec3 sampleOffsetDirections[20] = vec3[] (
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0)
);
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, float currentHeightScale)
{
    // 预计算导数
    vec2 dx = dFdx(texCoords);
    vec2 dy = dFdy(texCoords);

    // 动态层数 (根据视角优化性能)
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    // 视角越平(z越小)，层数越多
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    
    // 修正偏移方向
    vec2 P = viewDir.xy / max(viewDir.z, 0.00001) * currentHeightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    vec2  currentTexCoords = texCoords;
    vec2 finalTexCoords;

    if(useheightMap)
    {
        float heightValue = textureGrad(heightMap, currentTexCoords, dx, dy).r;
        if (heightMapInvert) heightValue = 1.0 - heightValue;

        float currentDepthMapValue = heightValue;
  
        while(currentLayerDepth < currentDepthMapValue)
        {
            currentTexCoords -= deltaTexCoords;
        
            heightValue = textureGrad(heightMap, currentTexCoords, dx, dy).r;
            if (heightMapInvert) heightValue = 1.0 - heightValue;
        
            currentDepthMapValue = heightValue;  
            currentLayerDepth += layerDepth;  
        }
    
        //  深度插值
        vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
        float afterDepth  = currentDepthMapValue - currentLayerDepth;
    
        float prevHeightValue = textureGrad(heightMap, prevTexCoords, dx, dy).r;
        if (heightMapInvert) prevHeightValue = 1.0 - prevHeightValue;
    
        float beforeDepth = prevHeightValue - (currentLayerDepth - layerDepth);
   
        float weight = afterDepth / (afterDepth - beforeDepth);
        finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    }
    return finalTexCoords;
}

vec3 getNormalFromMap(vec2 uv)
{
    if (!useNormalMap) return normalize(Normal);

    vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;

    return normalize(TBN * tangentNormal);
}

// GGX/Trowbridge-Reitz 法线分布函数
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom + 0.0001;

    return nom / denom;
}

// 几何遮挡函数（Schlick-GGX）
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// 几何阴影函数（Smith）
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


// 菲涅尔方程（基础版）
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 菲涅尔方程（带粗糙度，用于IBL）
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 软阴影计算
float calculatePointShadow(vec3 fragPos, int lightIndex, vec3 N) 
{
    vec3 lightPos = pointLightsBase[lightIndex].position;
    float far_plane = pointLightsBase[lightIndex].far_plane;
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float NdotL = max(dot(N, normalize(lightPos - fragPos)), 0.0);
    
    float bias = usePOM ? 0.08 : mix(0.015, 0.005, NdotL);
    
    if (isGlass) bias = 0.5; // 玻璃特殊处理
    
    int samples = 20;
    float diskRadius = (1.0 + (length(camPos - fragPos) / far_plane)) / 25.0;
    
    for(int i = 0; i < samples; ++i) 
    {
        float closestDepth = texture(pointLightDepthCubemaps[lightIndex], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;
        if(currentDepth - bias > closestDepth) 
            shadow += 1.0;
    }
    shadow /= float(samples);
    if(currentDepth > far_plane) 
        shadow = 0.0;

    return shadow;
}

vec3 dirLightCalculate(vec3 albedo, vec3 N, vec3 F0, float roughness, float metallic, float alpha)
{
    vec3 Lo = vec3(0.0);

    vec3 V = normalize(camPos - WorldPos);
    vec3 L = normalize(-dirLight.direction);
    vec3 H = normalize(V + L);
    vec3 radiance = dirLight.color;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
        
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);        

    if (isGlass) 
    {
        // 玻璃透射近似
        float thicknessFactor = 1.0 - pow(max(dot(N, V), 0.0), 2.0);
        vec3 transmitLight = dirLight.color * albedo * alpha * thicknessFactor * (1.0 - roughness * 0.5);
        Lo += (specular * radiance * NdotL) + (transmitLight * kD);
    } 
    else 
    {
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    return Lo;
}

// ----------------------------------------------------------------------------
void main()
{	
    // 距离淡出
    float viewDistance = length(camPos - WorldPos);
    float pomFadeStart = 10.0;
    float pomFadeEnd = 30.0;
    float pomFactor = 1.0 - clamp((viewDistance - pomFadeStart) / (pomFadeEnd - pomFadeStart), 0.0, 1.0);
    
    vec2 finalTexCoords = TexCoords;
    if (usePOM && pomFactor > 0.01 && !isGlass)
    {
        // 计算切线空间视线方向
        vec3 viewDirTangent = normalize(TangentViewPos - TangentFragPos);
        
        // 动态调整高度缩放 (远处减弱为0)
        float effectiveHeightScale = heightScale * pomFactor;
        
        finalTexCoords = ParallaxMapping(TexCoords, viewDirTangent, effectiveHeightScale);
        
        // 丢弃边缘 (防止纹理重复处的伪影)
        //if(finalTexCoords.x > 1.0 || finalTexCoords.y > 1.0 || finalTexCoords.x < 0.0 || finalTexCoords.y < 0.0)
        //  discard; 
    }

    vec4 albedoData = materialBaseColor;
    float metallic = materialMetallic;
    float roughness = materialRoughness;
    float ao = 1.0;
    vec3 emissive = vec3(0.0);

    if(useAlbedoMap) 
    {
        vec4 texColor = texture(albedoMap, finalTexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2)); 
        albedoData *= texColor;
    }
    if (gltf)
    {
        if(useMetallicRoughnessMap)
        {
            vec4 mrSample = texture(metallic_roughnessMap, finalTexCoords);
            metallic *= mrSample.b;
            roughness *= mrSample.g;
        }
        else
        {
            if(useMetallicMap)
            {
                 metallic *= texture(metallicMap, finalTexCoords).r;
            }
            if(useRoughnessMap)
            {
                roughness *= texture(roughnessMap, finalTexCoords).r;
            }
        }
    }
    else
    {
        if(useMetallicMap)
        {
            metallic *= texture(metallicMap, finalTexCoords).r;
        }
        if(useRoughnessMap)
        {
            roughness *= texture(roughnessMap, finalTexCoords).r;
        }
    }

    if(useAOMap)
    {
        ao = texture(aoMap, finalTexCoords).r;
    }

    if (useEmissiveMap) 
    {
        emissive = texture(emissiveMap, finalTexCoords).rgb;
        emissive = pow(emissive, vec3(2.2)); // 转换到线性空间
        emissive *= materialEmissive;  // 乘上发光因子
    }

    // 数据归一化
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.01, 0.99);
    ao = clamp(ao, 0.0, 1.0);

    // 提取 RGB 用于光照计算
    vec3 albedo = albedoData.rgb;
    float alpha = albedoData.a;

    vec3 N = getNormalFromMap(finalTexCoords);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N); 

    if (doubleSided && dot(N, V) < 0.0) N = -N;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    vec3 Lo = vec3(0.0);

    //if(dirLight.enabled)
    //{
    //    Lo += dirLightCalculate(albedo, N, F0, roughness, metallic, alpha);
    //}

    for(int i = 0; i < pointLightCount; ++i) 
    {   
        PointLightBase light = pointLightsBase[i];
        vec3 L = normalize(light.position - WorldPos);
        //float theta = dot(L, normalize(-light.direction));
        //if(theta < light.cutOff) 
        //{
            //continue;
        //}
        float distance = length(light.position - WorldPos);
        vec3 H = normalize(V + L);

        if(distance > light.far_plane){continue;};

        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light.color * attenuation;

        // BRDF计算
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.001) * max(dot(N, L), 0.001);
        vec3 specular = numerator / denominator;
        
        // 能量守恒
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	 

        vec3 diffusePart = kD  * albedo / PI;

            
        // 阴影计算
        float shadow = calculatePointShadow(WorldPos, i, N);
        float shadowFactor = 1.0 - shadow;

        float NdotL = max(dot(N, L), 0.0);        
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadowFactor;
    }   
    vec3 ambient;
    if(useIBL)
    {
        // IBL环境光计算
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;	  

        // 漫反射 IBL
        vec3 irradiance = texture(irradianceMap, N).rgb;
        irradiance = clamp(irradiance, vec3(0.0), vec3(0.5)); // 限制环境光强度
        vec3 diffuse = irradiance * albedo;

        // 镜面反射 IBL
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        // 透射 IBL (看穿背景)

        // 环境光叠加AO
        ambient = (kD * diffuse + specular) * ao;

    }
    // 最终颜色计算
    else
    {
        ambient = environmentLight * albedo * ao;
    }
    vec3 color = ambient + Lo + emissive;
    color = color / (color + vec3(1.0)); // Reinhard色调映射
    color = pow(color, vec3(1.0/2.2));  // Gamma校正
    

    FragColor = vec4(color, alpha);
    
}