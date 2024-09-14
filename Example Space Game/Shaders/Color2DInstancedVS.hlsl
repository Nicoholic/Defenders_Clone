// Pulled directly from the "VulkanDescriptorSets" sample.
// Removing the arrays & using HLSL StructuredBuffer<> would be better.
#pragma pack_matrix(row_major)
#define MAX_MATRIX_SIZE 1000
#define MAX_NUM_LIGHT_SOURCES 20
#define VIEW_MAT_SIZE 2
struct VS_IN
{
    float3 inputVertex : POSITION;
    float3 uvw : TEXCOORD;
    float3 normal : NORMAL;
};

[[vk::push_constant]]
cbuffer SHADER_VARS
{
    uint materialIndex;
    uint wMatrixInstanceOffset; // added 1/27/24 by DD
    uint greyscaleFilter;
    uint invertedColorsFilter;
    uint isOrthogonal;
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
    // spot lighting
    float3 spotLightPos[MAX_NUM_LIGHT_SOURCES];
    float4 spotLightColor[MAX_NUM_LIGHT_SOURCES]; // w is intensity
    float spotLightAngle;
};
// TODO: Part 2i
struct OUT_DATA
{
    float4 pos : SV_Position;
    float4 normVal : NORMAL;
    float4 surfacePos : POSITION1;
};
// TODO: Part 3e
// TODO: Part 4a
// TODO: Part 4b
// TODO: Part 4g
OUT_DATA main(VS_IN inputData, uint instanceID : SV_InstanceID)
{
    OUT_DATA output;
    output.surfacePos = mul(float4(inputData.inputVertex, 1), MeshData[0].worldMatrices[wMatrixInstanceOffset + instanceID]);
    output.pos = mul(output.surfacePos, viewMatrix[isOrthogonal]);
    output.pos = mul(output.pos, projMatrix[isOrthogonal]);
    output.normVal = float4(mul(float4(inputData.normal, 0), MeshData[0].worldMatrices[wMatrixInstanceOffset + instanceID]));
	// TODO: Part 1h
	// TODO: Part 2i
	// TODO: Part 4b
	// TODO: Part 4e
    return output;
}