#version 330 core
out vec4 FragColor;

struct Material {
// sampler2D diffuse;  <-- 删掉
// sampler2D specular; <-- 删掉
    float shininess;
};

// 放在外面，直接对应 Assimp 的命名惯例
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 VertColor; // [新输入] 接收来自 C++ 的顶点颜色
// [新增] 接收光空间坐标
in vec4 FragPosLightSpace;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
// [新增] 阴影贴图采样器
uniform sampler2D shadowMap;

// function prototypes
// [修改] CalcDirLight 增加 shadow 参数
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float shadow);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo);

// [新增] 阴影计算函数
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 1. 归一化坐标 [-1, 1] -> [0, 1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // 如果超过视锥体远平面，不仅没有阴影，直接返回 0
    if(projCoords.z > 1.0)
    return 0.0;

    // 2. 获取最近深度 (从阴影贴图中)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // 3. 获取当前片段深度
    float currentDepth = projCoords.z;

    // 4. 阴影偏移 (Shadow Bias) - 解决“阴影痤疮” (Shadow Acne)
//    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float bias = max(0.015 * (1.0 - dot(normal, lightDir)), 0.0015);

    // 5. PCF (Percentage-closer filtering) - 软阴影
    // 采样周围 3x3 个像素平均一下
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // [关键修改] 预先计算“反照率”(Albedo)
    // 逻辑：最终基础色 = 纹理采样颜色 * 顶点颜色
    // 对于 Steve: texture(皮肤) * vec3(1,1,1) = 皮肤
    // 对于 钻石剑: texture(白色) * vec3(0,0.5,0.7) = 蓝色
//    vec4 texData = texture(material.diffuse, TexCoords);
    vec4 texData = texture(texture_diffuse1, TexCoords);

    // [可选] 透明度测试 (解决 Steve 帽子层遮挡问题)
    if(texData.a < 0.1) discard;

    vec3 albedo = vec3(texData) * VertColor;

    // [新增] 计算阴影 (只针对方向光)
    // 传入 normal 和 光线反方向 (指向光源)
    float shadow = ShadowCalculation(FragPosLightSpace, norm, normalize(-dirLight.direction));

    // == =====================================================
    // 我们将 albedo 传递给光照计算函数，避免在每个函数里重复采样和相乘
    // == =====================================================

    // phase 1: directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir, albedo, shadow);
    // phase 2: point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
    result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, albedo);
    // phase 3: spot light
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir, albedo);

    FragColor = vec4(result, 1.0);
//    FragColor = vec4(vec3(1.0 - shadow), 1.0);
}

// [修改] CalcDirLight 实现
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float shadow)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient = light.ambient * albedo;
    vec3 diffuse = light.diffuse * diff * albedo;
//    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords));

    // [核心] 阴影只影响漫反射和高光，不影响环境光
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    // [修改] 使用传入的 albedo
    vec3 ambient = light.ambient * albedo;
    vec3 diffuse = light.diffuse * diff * albedo;
//    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // combine results
    // [修改] 使用传入的 albedo
    vec3 ambient = light.ambient * albedo;
    vec3 diffuse = light.diffuse * diff * albedo;
//    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords));

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}