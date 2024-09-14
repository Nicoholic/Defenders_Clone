// Pulled directly from the "VulkanDescriptorSets" sample.
// Removing the arrays & using HLSL StructuredBuffer<> would be better.
#pragma pack_matrix(row_major)
#define MAX_MATRIX_SIZE 1000
#define MAX_NUM_LIGHT_SOURCES 20
#define VIEW_MAT_SIZE 2
#pragma pack

[[vk::push_constant]]
cbuffer SHADER_VARS
{
    uint materialIndex;
    uint wMatrixInstanceOffset; // added 1/27/24 by DD
    uint greyscaleFilter;
    uint invertedColorsFilter;
};

struct OBJ_ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};
struct STORAGE_BUFFER_DATA
{
    //uint transformStart;
    OBJ_ATTRIBUTES materialData[MAX_MATRIX_SIZE];
    matrix worldMatrices[MAX_MATRIX_SIZE];
};
[[vk::binding(0, 0)]]
StructuredBuffer<STORAGE_BUFFER_DATA> MeshData;

[[vk::binding(1, 1)]]
cbuffer SCENE_DATA
{
    matrix viewMatrix[VIEW_MAT_SIZE], projMatrix[VIEW_MAT_SIZE];
    // directional lighting
    float3 lightDir, lightColor, cameraPos,
    ambientColor;
    // point lighting
    float3 pointLightPos[MAX_NUM_LIGHT_SOURCES];
    float4 pointLightColor[MAX_NUM_LIGHT_SOURCES]; // w is intensity
    //spot lighting
    float3 spotLightPos[MAX_NUM_LIGHT_SOURCES];
    float4 spotLightColor[MAX_NUM_LIGHT_SOURCES]; // w is intensity
    float spotLightAngle;
};

struct PS_IN
{
    float4 pos : SV_Position;
    float4 normVal : NORMAL;
    float4 surfacePos : POSITION1;
};

float4 main(PS_IN inVal) : SV_TARGET
{
    //float3 modifiedLightDir = normalize(float3(1.0f, -1.0f, 1.0f));
    // -----LIGHT RATIO-----
    inVal.normVal.xyz = normalize(inVal.normVal.xyz);
    float lightRatio = max(dot(inVal.normVal.xyz, -lightDir), 0.0);
    // -----SPECULAR-----
    float3 viewDir = normalize(cameraPos - inVal.surfacePos.xyz);
    float3 halfVec = normalize((-lightDir) + viewDir);
    float specIntensity = pow(max(clamp(dot(inVal.normVal.xyz, halfVec), 0, 1), 0), MeshData[0].materialData[materialIndex].Ns);
    float3 specReflections = lightColor * (specIntensity * MeshData[0].materialData[materialIndex].Ks);
    // -----DIR LIGHT-----
    float3 directionalLightOutVal = lightColor * MeshData[0].materialData[materialIndex].Kd * lightRatio + ambientColor + specReflections;
    
    float3 pointLightOutVal = float3(0, 0, 0);
    float3 spotLightOutVal = float3(0, 0, 0);
    for (int i = 0; i < MAX_NUM_LIGHT_SOURCES; i++)
    {
        // -----POINT LIGHT-----
        if (pointLightColor[i].w != 0)// if light exists
        {
            //point light base
            float3 pointLightDir = normalize(pointLightPos[i] - inVal.surfacePos.xyz);
            float3 pointLightDistance = pointLightPos[i] - inVal.surfacePos.xyz;
            float pointLightFallOffVal = 1.0 / dot(pointLightDistance, pointLightDistance);
            float pointLightRatio = max(dot(inVal.normVal.xyz, pointLightDir), 0);
            //point light specular
            float3 pointLightHalfVec = normalize(pointLightDir - viewDir);
            float pointLightSpecIntensity = pow(max(clamp(dot(inVal.normVal.xyz, pointLightHalfVec), 0, 1), 0), MeshData[0].materialData[materialIndex].Ns);
            float3 pointLightSpecReflections = pointLightColor[i].xyz * (pointLightSpecIntensity * MeshData[0].materialData[materialIndex].Ks) * pointLightFallOffVal;
            // final calc
            pointLightOutVal += ((pointLightColor[i].xyz * pointLightFallOffVal * pointLightColor[i].w) * MeshData[0].materialData[materialIndex].Kd
            * pointLightRatio) + pointLightSpecReflections;
        }
        // -----SPOT LIGHT-----
        if (spotLightColor[i].w != 0)// if light exists
        {
            // spot light base
            float3 spotLightDir = normalize(spotLightPos[i] - inVal.surfacePos.xyz);
            float spotLightRatio = max(dot(inVal.normVal.xyz, spotLightDir), 0);
            float spotLightTheta = dot(-spotLightDir, spotLightPos[i]);
            float spotFactor = smoothstep(spotLightAngle, spotLightAngle + 0.1, spotLightTheta);
            spotLightOutVal += spotLightColor[i].xyz * spotFactor * MeshData[0].materialData[materialIndex].Kd * spotLightRatio;

        }
    }
    if (invertedColorsFilter == 1)
    {
        return float4((1, 1, 1) - clamp(pointLightOutVal + directionalLightOutVal + spotLightOutVal, float3(0, 0, 0), float3(1, 1, 1)), 1);
    }
    if (greyscaleFilter == 1)
    {
        float combinedVal = (pointLightOutVal.x + pointLightOutVal.y + pointLightOutVal.z +
                        directionalLightOutVal.x + directionalLightOutVal.y + directionalLightOutVal.z +
                        spotLightOutVal.x + spotLightOutVal.y + spotLightOutVal.z) / 3;
        return float4(clamp(float3(combinedVal, combinedVal, combinedVal), float3(0, 0, 0), float3(1, 1, 1)), 1);
    }

    // -----DRAW COLOR-----
    return float4(clamp(pointLightOutVal + directionalLightOutVal + spotLightOutVal + MeshData[0].materialData[materialIndex].Ke, float3(0, 0, 0), float3(1, 1, 1)), 1);
	
}