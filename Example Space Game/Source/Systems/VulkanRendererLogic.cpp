#include "VulkanRendererLogic.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"

using namespace USG; // Example Space Game

bool USG::VulkanRendererLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig, GW::GRAPHICS::GVulkanSurface _vulkan,
	GW::SYSTEM::GWindow _window, LEVEL_DATA _levelData,
	LEVEL_DATA _dynamicModelData, std::vector<GW::MATH::GVECTORF>_pointLightVec, GW::INPUT::GInput _input, bool _lineRender)// added level data to the Initialization function
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	vlk = _vulkan;
	win = _window;
	levelData = _levelData;
	lineRenderFilter = _lineRender;

	levelDataWorldMats = levelData.levelTransforms;

	dynamicModelData = _dynamicModelData;
	pointLightVec = _pointLightVec;

	UpdateWindowDimensions();
	InitializeMatrices();
	InitializeLight();
	GetHandlesFromSurface();
	debugCamera.Init(_input);

	// Setup all vulkan resources
	if (LoadShaders() == false)
		return false;
	if (LoadDescriptorBuffers() == false)// load in both uniform and storage buffers
		return false;
	if (LoadGeometry() == false)
		return false;
	if (SetupPipeline() == false)
		return false;
	// Setup drawing engine
	if (SetupDrawcalls() == false)
		return false;
	// GVulkanSurface will inform us when to release any allocated resources
	if (!shutdownStarted) {
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				FreeVulkanResources(); // unlike D3D we must be careful about destroy timing
			}
			});
		shutdownStarted = true;
	}
	return true;
}

bool USG::VulkanRendererLogic::Activate(bool runSystem)
{
	if (startDraw.is_alive() &&
		updateDraw.is_alive() &&
		completeDraw.is_alive()) {
		if (runSystem) {
			startDraw.enable();
			updateDraw.enable();
			completeDraw.enable();
		}
		else {
			startDraw.disable();
			updateDraw.disable();
			completeDraw.disable();
		}
		return true;
	}
	return false;
}

bool USG::VulkanRendererLogic::Shutdown()
{
	startDraw.destruct();
	updateDraw.destruct();
	completeDraw.destruct();
	return true; // vulkan resource shutdown handled via GEvent in Init()
}

// added by DD 1/14/24
// gives people outside to update the camera pos to the view matrix
void USG::VulkanRendererLogic::UpdateCameraPos() {
	InitializeCameraPosMatrix();
	UpdateCameraPosVec();
}
void USG::VulkanRendererLogic::UpdateViewMatrix(GW::MATH::GMATRIXF _camPos) {
	SCENE_DATA.viewMatrix[0] = _camPos;// may potentially need to be inverted
}

std::string USG::VulkanRendererLogic::ShaderAsString(const char* shaderFilePath)
{
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file; file.Create();
	file.GetFileSize(shaderFilePath, stringLength);
	if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	else
		std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
	return output;
}

// updates client
void USG::VulkanRendererLogic::UpdateWindowDimensions()
{
	win.GetClientWidth(windowWidth);
	win.GetClientHeight(windowHeight);
}

// region added by DD on 1/14/24
#pragma region MatrixSetup
/// <summary>
	/// sets up world matrices based inputted level data
	/// </summary>
void USG::VulkanRendererLogic::InitializeWorldMatrix() {
	for (size_t i = 0; i < levelData.levelTransforms.size(); i++) {
		MESH_DATA.worldMatrices[i] = levelData.levelTransforms[i];
	}
	worldMatrixStaticLevelDataEndPosition = levelData.levelTransforms.size();
	//matrixProxy.RotateXLocalF(MESH_DATA.worldMatrix, wMatrixRotSpeed, rotWorldMatrix);// rotation matrix
}
/// <summary>
/// sets up a view matrix with initial values
/// </summary>
void USG::VulkanRendererLogic::InitializeViewMatrix() {
	SCENE_DATA.viewMatrix;
	matrixProxy.IdentityF(SCENE_DATA.viewMatrix[0]);// no uninitialized mem
	GW::MATH::GVECTORF translationVec;
	translationVec.x = 0, translationVec.y = -1, translationVec.z = 0;
	/*matrixProxy.TranslateGlobalF(SCENE_DATA.viewMatrix, translationVec.xyz(), SCENE_DATA.viewMatrix);
	GW::MATH::GVECTORF lookAtVec;
	lookAtVec.x = 0.15, lookAtVec.y = 0.75;
	matrixProxy.LookAtLHF(translationVec, lookAtVec.xy(), UP_VEC, SCENE_DATA.viewMatrix);*/
	SCENE_DATA.viewMatrix[0] = levelData.cameraMatrix;// find the camera pos
	//matrixProxy.TranslateGlobalF(SCENE_DATA.viewMatrix, translationVec.xyz(), SCENE_DATA.viewMatrix);
	/*GW::MATH::GVECTORF lookAtPos;
	matrixProxy.GetTranslationF(MESH_DATA.worldMatrices[0], lookAtPos);
	matrixProxy.LookAtLHF(translationVec.xyz(), lookAtPos, UP_VEC, SCENE_DATA.viewMatrix);
	SCENE_DATA.viewMatrix = GW::MATH::GMATRIXF{ -0.99, 0.063, 0.021, 0,
												0.037, 0.795, -0.605, 0,
												-0.055, -0.603, -0.796, 0,
												21.989, 25.105, 576.31, 1 };*/
	matrixProxy.IdentityF(SCENE_DATA.viewMatrix[1]);

}
void USG::VulkanRendererLogic::InitializeCameraPosMatrix() {
	matrixProxy.InverseF(SCENE_DATA.viewMatrix[0], cameraPosMatrix);
}
/// <summary>
/// creates a projection matrix
/// </summary>
void USG::VulkanRendererLogic::InitializeProjMatrix() {
	SCENE_DATA.projMatrix;
	// crate normal projection matrix
	matrixProxy.IdentityF(SCENE_DATA.projMatrix[0]);// no uninitialized mem
	float yScale = 1 / tanf(0.5f * FOV);
	float aspectRatio;
	auto er = vlk.GetAspectRatio(aspectRatio);
	SCENE_DATA.projMatrix[0].row1.x = yScale * aspectRatio;// X scale
	SCENE_DATA.projMatrix[0].row2.y = yScale;// Y scale
	SCENE_DATA.projMatrix[0].row3.z = FAR_PLANE / (FAR_PLANE - NEAR_PLANE);
	SCENE_DATA.projMatrix[0].row3.w = 1;
	SCENE_DATA.projMatrix[0].row4.z = -(FAR_PLANE * NEAR_PLANE) / (FAR_PLANE - NEAR_PLANE);
	SCENE_DATA.projMatrix[0].row4.w = 0;
	// create orthographic matrix for UI
	matrixProxy.IdentityF(SCENE_DATA.projMatrix[1]);
}
/// <summary>
/// Initialize all needed matrices
/// </summary>
void USG::VulkanRendererLogic::InitializeMatrices() {
	InitializeWorldMatrix();
	InitializeViewMatrix();
	InitializeCameraPosMatrix();
	InitializeProjMatrix();
}


#pragma endregion
// region added by DD on 3/14/24
#pragma region LightingSetup

void USG::VulkanRendererLogic::InitializeLightDir() {
	lightDirVec.x = 0.25, lightDirVec.y = -1, lightDirVec.z = -0.75, lightDirVec.w = 0;
	vecProxy.NormalizeF(lightDirVec, lightDirVec);
	SCENE_DATA.lightDir = lightDirVec;
}
void USG::VulkanRendererLogic::InitializeLightColor() {
	lightColorVec.x = 0.8, lightColorVec.y = 0.2, lightColorVec.z = 0.1, lightColorVec.w = 1;
	SCENE_DATA.lightColor = lightColorVec;
}
void USG::VulkanRendererLogic::InitializeAmbientLight() {
	ambientColorVec.x = 0.05, ambientColorVec.y = 0.05, ambientColorVec.z = 0.1, ambientColorVec.w = 1;
	SCENE_DATA.ambientColor = ambientColorVec;
}
void USG::VulkanRendererLogic::InitializeMaterialData() {// creates array of material data
	for (size_t i = 0; i < levelData.levelMaterials.size(); i++) {
		MESH_DATA.materialData[i] = levelData.levelMaterials[i];
	}
	materialDataStaticLevelDataEndPosition = levelData.levelMaterials.size();
}
void USG::VulkanRendererLogic::InitializePointLightPos() {
	for (size_t i = 0; i < MAX_NUM_LIGHT_SOURCES; i++) {
		if (i >= levelData.pointLightTransforms.size()) {// light not found
			SCENE_DATA.pointLightPos[i] = { -1000, -1000, -1000, -1000 };// defualt value to check for in color initialization
		}
		else {// light found
			auto hr = matrixProxy.GetTranslationF(levelData.pointLightTransforms[i], SCENE_DATA.pointLightPos[i]);
		}
	}
}
void USG::VulkanRendererLogic::InitializePointLightColor() {
	for (size_t i = 0; i < MAX_NUM_LIGHT_SOURCES; i++) {
		if (SCENE_DATA.pointLightPos[i].w == -1000) {// no light found
			SCENE_DATA.pointLightColor[i] = { 0,0,0,0 };
		}
		else {// light found
			SCENE_DATA.pointLightColor[i] = { 0.8f, 1.0f, 0.85f, 0.8f };// green light inspired by Japanese Mercury Vapor Lighting
		}
	}
}
void USG::VulkanRendererLogic::InitializePointLights() {
	InitializePointLightPos();
	InitializePointLightColor();
}
void USG::VulkanRendererLogic::InitializeSpotLightPos() {
	for (size_t i = 0; i < MAX_NUM_LIGHT_SOURCES; i++) {
		if (i >= levelData.spotLightTransforms.size()) {// light not found
			SCENE_DATA.spotLightPos[i] = { -1000, -1000, -1000, -1000 };// defualt value to check for in color initialization
		}
		else {// light found
			auto hr = matrixProxy.GetTranslationF(levelData.spotLightTransforms[i], SCENE_DATA.spotLightPos[i]);
		}
	}
}
void USG::VulkanRendererLogic::InitializeSpotLightColor() {
	for (size_t i = 0; i < MAX_NUM_LIGHT_SOURCES; i++) {
		if (SCENE_DATA.spotLightPos[i].w == -1000) {// no light found
			SCENE_DATA.spotLightColor[i] = { 0,0,0,0 };
		}
		else {// light found
			SCENE_DATA.spotLightColor[i] = { 1.0f, 0.8f, 0.6f, 0.8f };// warm space ship spot lights
		}
	}
}
void USG::VulkanRendererLogic::InitializeSpotLightAngle() {
	SCENE_DATA.spotLightAngle = FOV;// change in future but basic setup for now
}
void USG::VulkanRendererLogic::InitializeSpotLights() {
	InitializeSpotLightPos();
	InitializeSpotLightColor();
	InitializeSpotLightAngle();
}
void USG::VulkanRendererLogic::InitializeLight() {
	InitializeLightDir();
	InitializeLightColor();
	InitializeAmbientLight();
	InitializeMaterialData();
	InitializePointLights();
	InitializeSpotLights();
}

#pragma endregion
// added 1/15/24 by DD
// gets called once instead of in every function
void USG::VulkanRendererLogic::GetHandlesFromSurface() {
	vlk.GetDevice((void**)&device);
	vlk.GetPhysicalDevice((void**)&physicalDevice);
	vlk.GetRenderPass((void**)&renderPass);
}

// added by DD 1/14/24
		// used in UpdateCameraPos
void USG::VulkanRendererLogic::UpdateCameraPosVec() {
	matrixProxy.VectorXMatrixF(cameraPosMatrix, GW::MATH::GVECTORF{ 0,0,0,1 }, SCENE_DATA.cameraPos);
}

bool USG::VulkanRendererLogic::LoadShaders()
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	std::string vertexShaderSource = (*readCfg).at("Shaders").at("vertex").as<std::string>();
	std::string fragmentShaderSource = (*readCfg).at("Shaders").at("pixel").as<std::string>();

	if (vertexShaderSource.empty() || fragmentShaderSource.empty())
		return false;

	vertexShaderSource = ShaderAsString(vertexShaderSource.c_str());
	fragmentShaderSource = ShaderAsString(fragmentShaderSource.c_str());

	if (vertexShaderSource.empty() || fragmentShaderSource.empty())
		return false;


	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();
	shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
	shaderc_compile_options_set_invert_y(options, true);
#ifndef NDEBUG
	shaderc_compile_options_set_generate_debug_info(options);
#endif
	// Create Vertex Shader
	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
		compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
		shaderc_vertex_shader, "main.vert", "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
		(char*)shaderc_result_get_bytes(result), &vertexShader);
	shaderc_result_release(result); // done
	// Create Pixel Shader
	result = shaderc_compile_into_spv( // compile
		compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
		shaderc_fragment_shader, "main.frag", "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
	GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
		(char*)shaderc_result_get_bytes(result), &fragmentShader);
	shaderc_result_release(result); // done
	// Free runtime shader compiler resources
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
	return true;
}

bool USG::VulkanRendererLogic::LoadDescriptorBuffers()
{

	unsigned max_frames = 0;
	// to avoid per-frame resource sharing issues we give each "in-flight" frame its own buffer
	vlk.GetSwapchainImageCount(max_frames);
	uniformBufferHandle.resize(max_frames);
	uniformBufferData.resize(max_frames);
	storageBufferHandle.resize(max_frames);// resizing storage buffer vector as well
	storageBufferData.resize(max_frames);
	for (int i = 0; i < max_frames; ++i) {

		if (VK_SUCCESS != GvkHelper::create_buffer(physicalDevice, device,
			sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBufferHandle[i], &uniformBufferData[i]))
			return false;

		if (VK_SUCCESS != GvkHelper::write_to_buffer(device, uniformBufferData[i],
			&SCENE_DATA, sizeof(SceneData)))
			return false;
		// added storage buffer initialization 1/15/24
		if (VK_SUCCESS != GvkHelper::create_buffer(physicalDevice, device,
			sizeof(MeshData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&storageBufferHandle[i], &storageBufferData[i]))
			return false;

		if (VK_SUCCESS != GvkHelper::write_to_buffer(device, storageBufferData[i],
			&MESH_DATA, sizeof(MeshData)))
			return false;
	}
	// uniform buffers and storage buffers created
	return true;
}

bool USG::VulkanRendererLogic::LoadGeometry()
{

	float verts[] = {
		-0.5f, -0.5f,
		0, 0.5f,
		0, -0.25f,
		0.5f, -0.5f
	};
	// Transfer triangle data to the vertex buffer. (staging buffer would be prefered here)
	if (LoadVertexBuffer(levelData.levelVerts.data(), sizeof(H2B::VERTEX) * levelData.levelVerts.size(), &vertexHandle, &vertexData)) {
		if (LoadIndexBuffer(levelData.levelIndices.data(), sizeof(unsigned int) * levelData.levelIndices.size(), &indexHandle, &indexData)) {
			if (LoadVertexBuffer(dynamicModelData.levelVerts.data(), sizeof(H2B::VERTEX) * dynamicModelData.levelVerts.size(), &dynamicVertexBuffer, &dynamicVertexData)) {
				if (LoadIndexBuffer(dynamicModelData.levelIndices.data(), sizeof(unsigned int) * dynamicModelData.levelIndices.size(), &dynamicIndexBuffer, &dynamicIndexData))
					return true;
			}
		}
	}
	return false;
}

bool USG::VulkanRendererLogic::LoadVertexBuffer(const void* data, unsigned int sizeInBytes, VkBuffer* buffer, VkDeviceMemory* deviceMem) {
	GvkHelper::create_buffer(physicalDevice, device, sizeInBytes,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, deviceMem);
	// Transfer triangle data to the vertex buffer. (staging would be prefered here)
	if (VK_SUCCESS == GvkHelper::write_to_buffer(device, *deviceMem, data, sizeInBytes)) {
		return true;
	}
	return false;
}
bool USG::VulkanRendererLogic::LoadIndexBuffer(const void* data, unsigned int sizeInBytes, VkBuffer* buffer, VkDeviceMemory* deviceMem) {
	GvkHelper::create_buffer(physicalDevice, device, sizeInBytes,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, deviceMem);
	// Transfer integer data to the index buffer. (staging would be prefered here)
	if (VK_SUCCESS == GvkHelper::write_to_buffer(device, *deviceMem, data, sizeInBytes)) {
		return true;
	}
	return false;
}

bool USG::VulkanRendererLogic::SetupPipeline()
{

	VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
	// Create Stage Info for Vertex Shader
	stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stage_create_info[0].module = vertexShader;
	stage_create_info[0].pName = "main";
	// Create Stage Info for Fragment Shader
	stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stage_create_info[1].module = fragmentShader;
	stage_create_info[1].pName = "main";
	// Assembly State
	VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();
	// Vertex Input State
	VkVertexInputBindingDescription vertex_binding_description = CreateVkVertexInputBindingDescription();
	// tell the vertex shader what the input means
	VkVertexInputAttributeDescription vertex_attribute_description[3];
	vertex_attribute_description[0].binding = 0;// pos
	vertex_attribute_description[0].location = 0;
	vertex_attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_description[0].offset = 0;
	vertex_attribute_description[1].binding = 0;// uvw
	vertex_attribute_description[1].location = 1;
	vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_description[1].offset = sizeof(H2B::VERTEX::pos);
	vertex_attribute_description[2].binding = 0;// nrm
	vertex_attribute_description[2].location = 2;
	vertex_attribute_description[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_description[2].offset = sizeof(H2B::VERTEX::pos) * 2;

	VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(&vertex_binding_description, 1, vertex_attribute_description, 3);

	VkViewport viewport = CreateViewportFromWindowDimensions();
	VkRect2D scissor = CreateScissorFromWindowDimensions(); // we will overwrite this in Draw
	VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
	// Rasterizer State
	VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkPipelineRasterizationStateCreateInfo();
	// Multisampling State
	VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
	// Depth-Stencil State
	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
	// Color Blending Attachment & State
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo color_blend_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);
	// Dynamic State 
	VkDynamicState dynamic_state[2] = {
		// By setting these we do not need to re-create the pipeline on Resize
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_state, 2);
#pragma region Descriptors
	// Descriptor Setup
	// added by DD 3/15-17 
	descriptorHandler.Init(device);
	VkDescriptorPoolSize descriptorPoolSize[NUM_DESCRIPTOR_POOLS] = { {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferData.size()},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferData.size()} };
	descriptorPool = descriptorHandler.CreateDescriptorPool(storageBufferData.size() + uniformBufferData.size(), NUM_DESCRIPTOR_POOLS, descriptorPoolSize);
	// uniform buffer
	descriptorHandler.DescriptorSetSetupPipeline(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		&uniformBuffDescriptorLayout, descriptorPool, &uniformBufferDescriptorSet, uniformBufferHandle);
	// storage buffer
	descriptorHandler.DescriptorSetSetupPipeline(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		&storageBuffDescriptorLayout, descriptorPool, &storageBufferDescriptorSet, storageBufferHandle);
#pragma endregion
	// Descriptor pipeline layout
	CreatePipelineLayout();

	// Pipeline State... (FINALLY) 
	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;
	pipeline_create_info.pStages = stage_create_info;
	pipeline_create_info.pInputAssemblyState = &assembly_create_info;
	pipeline_create_info.pVertexInputState = &input_vertex_info;
	pipeline_create_info.pViewportState = &viewport_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_create_info;
	pipeline_create_info.pMultisampleState = &multisample_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
	pipeline_create_info.pColorBlendState = &color_blend_create_info;
	pipeline_create_info.pDynamicState = &dynamic_create_info;
	pipeline_create_info.layout = pipelineLayout;
	pipeline_create_info.renderPass = renderPass;
	pipeline_create_info.subpass = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

	if (VK_SUCCESS != vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
		&pipeline_create_info, nullptr, &pipeline))
		return false; // something went wrong

	return true;
}

#pragma region GraphicsPipelineHelperFunctions
// code moved for single responsibility
VkPipelineInputAssemblyStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineInputAssemblyStateCreateInfo()
{
	VkPipelineInputAssemblyStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	retval.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	retval.primitiveRestartEnable = false;
	return retval;
}
// description for size of vertex informatoin
VkVertexInputBindingDescription USG::VulkanRendererLogic::CreateVkVertexInputBindingDescription()
{
	VkVertexInputBindingDescription retval = {};
	retval.binding = 0;
	retval.stride = sizeof(H2B::VERTEX);
	retval.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return retval;
}
// compilation of other data for vulkan
VkPipelineVertexInputStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineVertexInputStateCreateInfo(
	VkVertexInputBindingDescription* bindingDescriptions, uint32_t bindingCount,
	VkVertexInputAttributeDescription* attributeDescriptions, uint32_t attributeCount)
{
	VkPipelineVertexInputStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	retval.vertexBindingDescriptionCount = bindingCount;
	retval.pVertexBindingDescriptions = bindingDescriptions;
	retval.vertexAttributeDescriptionCount = attributeCount;
	retval.pVertexAttributeDescriptions = attributeDescriptions;
	return retval;
}

VkViewport USG::VulkanRendererLogic::CreateViewportFromWindowDimensions()
{
	VkViewport retval = {};
	retval.x = 0;
	retval.y = 0;
	retval.width = static_cast<float>(windowWidth);
	retval.height = static_cast<float>(windowHeight);
	retval.minDepth = 0;
	retval.maxDepth = 1;
	return retval;
}

VkRect2D USG::VulkanRendererLogic::CreateScissorFromWindowDimensions()
{
	VkRect2D retval = {};
	retval.offset.x = 0;
	retval.offset.y = 0;
	retval.extent.width = windowWidth;
	retval.extent.height = windowHeight;
	return retval;
}

VkPipelineViewportStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineViewportStateCreateInfo(VkViewport* viewports, uint32_t viewportCount, VkRect2D* scissors, uint32_t scissorCount)
{
	VkPipelineViewportStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	retval.viewportCount = viewportCount;
	retval.pViewports = viewports;
	retval.scissorCount = scissorCount;
	retval.pScissors = scissors;
	return retval;
}

VkPipelineRasterizationStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineRasterizationStateCreateInfo()
{
	VkPipelineRasterizationStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	retval.rasterizerDiscardEnable = VK_FALSE;
	if (lineRenderFilter)
		retval.polygonMode = VK_POLYGON_MODE_LINE;
	else
		retval.polygonMode = VK_POLYGON_MODE_FILL;
	retval.lineWidth = 1.0f;
	retval.cullMode = VK_CULL_MODE_BACK_BIT;
	retval.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	retval.depthClampEnable = VK_FALSE;
	retval.depthBiasEnable = VK_FALSE;
	retval.depthBiasClamp = 0.0f;
	retval.depthBiasConstantFactor = 0.0f;
	retval.depthBiasSlopeFactor = 0.0f;
	return retval;
}

VkPipelineMultisampleStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineMultisampleStateCreateInfo()
{
	VkPipelineMultisampleStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	retval.sampleShadingEnable = VK_FALSE;
	retval.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	retval.minSampleShading = 1.0f;
	retval.pSampleMask = VK_NULL_HANDLE;
	retval.alphaToCoverageEnable = VK_FALSE;
	retval.alphaToOneEnable = VK_FALSE;
	return retval;
}

VkPipelineDepthStencilStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineDepthStencilStateCreateInfo()
{
	VkPipelineDepthStencilStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	retval.depthTestEnable = VK_TRUE;
	retval.depthWriteEnable = VK_TRUE;
	retval.depthCompareOp = VK_COMPARE_OP_LESS;
	retval.depthBoundsTestEnable = VK_FALSE;
	retval.minDepthBounds = 0.0f;
	retval.maxDepthBounds = 1.0f;
	retval.stencilTestEnable = VK_FALSE;
	return retval;
}

VkPipelineColorBlendAttachmentState USG::VulkanRendererLogic::CreateVkPipelineColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState retval = {};
	retval.colorWriteMask = 0xF;
	retval.blendEnable = VK_FALSE;
	retval.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	retval.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
	retval.colorBlendOp = VK_BLEND_OP_ADD;
	retval.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	retval.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	retval.alphaBlendOp = VK_BLEND_OP_ADD;
	return retval;
}

VkPipelineColorBlendStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState* attachmentStates, uint32_t attachmentCount)
{
	VkPipelineColorBlendStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	retval.logicOpEnable = VK_FALSE;
	retval.logicOp = VK_LOGIC_OP_COPY;
	retval.attachmentCount = attachmentCount;
	retval.pAttachments = attachmentStates;
	retval.blendConstants[0] = 0.0f;
	retval.blendConstants[1] = 0.0f;
	retval.blendConstants[2] = 0.0f;
	retval.blendConstants[3] = 0.0f;
	return retval;
}

VkPipelineDynamicStateCreateInfo USG::VulkanRendererLogic::CreateVkPipelineDynamicStateCreateInfo(VkDynamicState* dynamicStates, uint32_t dynamicStateCount)
{
	VkPipelineDynamicStateCreateInfo retval = {};
	retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	retval.dynamicStateCount = dynamicStateCount;
	retval.pDynamicStates = dynamicStates;
	return retval;
}
// post-descriptor set setup
void USG::VulkanRendererLogic::CreatePipelineLayout()
{
	// Descriptor pipeline layout
	// TODO: Part 2e
	// TODO: Part 3c
	VkPushConstantRange pushConstantRange;
	SetUpPushConstantData(&pushConstantRange);
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 2;
	VkDescriptorSetLayout layouts[2] = { storageBuffDescriptorLayout, uniformBuffDescriptorLayout };
	pipeline_layout_create_info.pSetLayouts = layouts;
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges = &pushConstantRange;

	auto hr = vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipelineLayout);
}
void USG::VulkanRendererLogic::SetUpPushConstantData(VkPushConstantRange* _pushConstantRange) {
	_pushConstantRange->size = sizeof(PushConstantData);
	_pushConstantRange->offset = 0;
	_pushConstantRange->stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

}
#pragma endregion

bool USG::VulkanRendererLogic::SetupDrawcalls() // I SCREWED THIS UP MAKES SO MUCH SENSE NOW
{
	// create a unique entity for the renderer (just a Tag)
	// this only exists to ensure we can create systems that will run only once per frame. 
	struct RenderingSystem {}; // local definition so we control iteration counts
	if (game->entity("Rendering System").has<RenderingSystem>())
		return true;

	game->entity("Rendering System").add<RenderingSystem>();
	// an instanced renderer is complex and needs to run additional system code once per frame
	// to do this I create 3 systems:
	// A pre-update system, that runs only once (using our Tag above)
	// An update system that iterates over all renderable components (may run multiple times)
	// A post-update system that also runs only once rendering all collected data

	// only happens once per frame
	startDraw = game->system<RenderingSystem>().kind(flecs::PreUpdate)
		.each([this](flecs::entity e, RenderingSystem& s) {
		// reset the draw counter only once per frame
		dynamicDrawInstructions.clear();
		for (size_t i = 0; i < pointLightCount; i++) {
			SCENE_DATA.pointLightPos[i] = {0,0,0,0};
			SCENE_DATA.pointLightColor[i] = {0,0,0,0};
		}
		pointLightCount = 0;

		bool temp = lineRenderFilter;
		// added by DD 3/14/2024
		debugCamera.GetInput(&SCENE_DATA.viewMatrix[0], e, &lineRenderFilter, &PUSH_CONSTANT_DATA.greyscaleFilter, &PUSH_CONSTANT_DATA.invertedColorsFilter);
		if (temp != lineRenderFilter)
			e.add<ToggleLineRender>();
			});
	// may run multiple times per frame, will run after startDraw
	updateDraw = game->system<ModelData>().kind(flecs::OnUpdate)
		.each([this](flecs::entity e, ModelData& md) {

		// copy all data to our instancing array
		//int i = draw_counter;
		//
		//// increment the shared draw counter but don't go over (branchless) 
		//int v = static_cast<int>(Instance_Max) - static_cast<int>(draw_counter + 2);
		//// if v < 0 then 0, else 1, https://graphics.stanford.edu/~seander/bithacks.html
		//int sign = 1 ^ ((unsigned int)v >> (sizeof(int) * CHAR_BIT - 1));
		//draw_counter += sign;

		if (e.get<ModelData>()->modelIndex < dynamicModelData.levelTransforms.size()) {// check if there's an entity that can be drawn

			int drawInstructionsIndex = -1;
			for (size_t ndx = 0; ndx < dynamicDrawInstructions.size(); ndx++) {// loop through to see if there's already an instance of that object
				if (dynamicDrawInstructions[ndx].model.filename == dynamicModelData.levelModels[e.get<ModelData>()->modelIndex].filename) {
					drawInstructionsIndex = ndx;
					break;
				}
			}

			if (drawInstructionsIndex >= 0) {// if draw instructions index exists
				dynamicDrawInstructions[drawInstructionsIndex].worldMatrices.push_back(e.get<ModelData>()->positionMatrix);
				dynamicDrawInstructions[drawInstructionsIndex].modelInstancesData.transformCount++;
			}
			else {// drawInstructionsIndex = -1
				DrawInstructions temp{};
				temp.model = dynamicModelData.levelModels[e.get<ModelData>()->modelIndex];
				temp.worldMatrices.push_back(e.get<ModelData>()->positionMatrix);
				temp.modelMeshes = dynamicModelData.levelMeshes;
				temp.modelInstancesData.transformCount = 1;
				dynamicDrawInstructions.push_back(temp);

			}
		}

			});


	updateDrawInstructions = game->system<RenderingSystem>().kind(flecs::OnUpdate)
		.each([this](flecs::entity e, RenderingSystem& s) {
		// write into the draw instructions only once per frame
		size_t currTransform = 0;
		for (size_t i = 0; i < dynamicDrawInstructions.size(); i++) {
			dynamicDrawInstructions[i].modelInstancesData.transformStart = currTransform;
			currTransform += dynamicDrawInstructions[i].modelInstancesData.transformCount;
		}
			});
	// added 3/24/24 by DD
	findPointLights = game->system<LightData>().kind(flecs::OnUpdate)
		.each([this](flecs::entity e, LightData& ld) {

		if (pointLightCount < MAX_NUM_LIGHT_SOURCES) {// check to see if there's a spot available in the array
			SCENE_DATA.pointLightPos[pointLightCount] = ld.lightPos.row4;
			SCENE_DATA.pointLightColor[pointLightCount] = pointLightVec[ld.lightID];
			pointLightCount++;
		}

			});

	// runs once per frame after updateDraw
	completeDraw = game->system<RenderingSystem>().kind(flecs::PostUpdate)
		.each([this](flecs::entity e, RenderingSystem& s) {
		// run the rendering code just once!
		// Copy data to this frame's buffer

		unsigned int activeBuffer;// could be simplified
		vlk.GetSwapchainCurrentImage(activeBuffer);

		// grab the current Vulkan commandBuffer
		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();
		SetUpPipeline(commandBuffer);

		for (size_t i = 0; i < levelDataWorldMats.size(); i++) {
			MESH_DATA.worldMatrices[i] = levelDataWorldMats[i];
		}
		matrixProxy.InverseF(SCENE_DATA.viewMatrix[0], cameraPosMatrix);
		matrixProxy.GetTranslationF(cameraPosMatrix, cameraPosVec);
		// Write and update buffers
		GvkHelper::write_to_buffer(device, uniformBufferData[activeBuffer], &SCENE_DATA, sizeof(SceneData));
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &uniformBufferDescriptorSet[0], 0, nullptr);

		UpdateStorageBuffer();

		auto r = GvkHelper::write_to_buffer(device, storageBufferData[activeBuffer], &MESH_DATA, sizeof(MeshData));
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &storageBufferDescriptorSet[0], 0, nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		PUSH_CONSTANT_DATA.isOrthogonal = 0;
		for (size_t i = 0; i < levelData.levelModels.size(); i++) {// loop through each model
			auto drawData = levelData.levelModels[i];

			// loop through each mesh of the model
			for (size_t ndx = levelData.levelModels[i].meshStart; ndx < levelData.levelModels[i].meshCount + levelData.levelModels[i].meshStart; ndx++) {
				// Push constant for material to draw
				PUSH_CONSTANT_DATA.materialIndex = drawData.materialStart + levelData.levelMeshes[ndx].materialIndex;
				PUSH_CONSTANT_DATA.wMatrixInstanceOffset = 0;
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &PUSH_CONSTANT_DATA);

				// draw all submeshes with that material
				vkCmdDrawIndexed(commandBuffer, levelData.levelMeshes[ndx].drawInfo.indexCount, levelData.levelInstancesData[i].transformCount,
					levelData.levelMeshes[ndx].drawInfo.indexOffset + drawData.indexStart,
					drawData.vertStart, levelData.levelInstancesData[i].transformStart);
			}
		}

		BindVertexBuffers(commandBuffer, dynamicStepBinding);
		BindIndexBuffers(commandBuffer, dynamicStepBinding);

		for (size_t i = 0; i < dynamicDrawInstructions.size(); i++) {// loop through each model
			// loop through each mesh of the model
			for (size_t ndx = dynamicDrawInstructions[i].model.meshStart; ndx < dynamicDrawInstructions[i].model.meshCount + dynamicDrawInstructions[i].model.meshStart; ndx++) {
				// Push constant for material to draw
				PUSH_CONSTANT_DATA.materialIndex = materialDataStaticLevelDataEndPosition + dynamicDrawInstructions[i].model.materialStart
					+ dynamicDrawInstructions[i].modelMeshes[ndx].materialIndex;
				PUSH_CONSTANT_DATA.wMatrixInstanceOffset = worldMatrixStaticLevelDataEndPosition;
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &PUSH_CONSTANT_DATA);

				// draw all submeshes with that material

				vkCmdDrawIndexed(commandBuffer, dynamicDrawInstructions[i].modelMeshes[ndx].drawInfo.indexCount, dynamicDrawInstructions[i].modelInstancesData.transformCount,
					dynamicDrawInstructions[i].modelMeshes[ndx].drawInfo.indexOffset + dynamicDrawInstructions[i].model.indexStart,
					dynamicDrawInstructions[i].model.vertStart, dynamicDrawInstructions[i].modelInstancesData.transformStart);
			}
		}
		PUSH_CONSTANT_DATA.isOrthogonal = 1;
		//rebind index/vertex buffers here and loop through UI
			});
	// NOTE: I went with multi-system approach for the ease of passing lambdas with "this"
	// There is a built-in solution for this problem referred to as a "custom runner":
	// https://github.com/SanderMertens/flecs/blob/master/examples/cpp/systems/custom_runner/src/main.cpp
	// The negative is that it requires the use of a C callback which is less flexibe vs the lambda
	// you could embed what you need in the ecs and use a lookup to get it but I think that is less clean

	// all drawing operations have been setup
	return true;
}

#pragma region RenderFuncHelpers

void USG::VulkanRendererLogic::UpdateStorageBuffer() {
	size_t endPosTrack = worldMatrixStaticLevelDataEndPosition;
	size_t endPosTrackMat = materialDataStaticLevelDataEndPosition;
	for (size_t i = 0; i < dynamicDrawInstructions.size(); i++) {
		for (size_t ndx = 0; ndx < dynamicDrawInstructions[i].worldMatrices.size(); ndx++) {
			MESH_DATA.worldMatrices[endPosTrack + ndx] = dynamicDrawInstructions[i].worldMatrices[ndx];
		}
		endPosTrack += dynamicDrawInstructions[i].worldMatrices.size();
	}
	for (size_t i = 0; i < dynamicModelData.levelMaterials.size(); i++) {
		MESH_DATA.materialData[endPosTrackMat + i] = dynamicModelData.levelMaterials[i];
	}
}

VkCommandBuffer USG::VulkanRendererLogic::GetCurrentCommandBuffer()
{
	VkCommandBuffer retval;
	unsigned int currentBuffer;
	vlk.GetSwapchainCurrentImage(currentBuffer);
	vlk.GetCommandBuffer(currentBuffer, (void**)&retval);
	return retval;
}

void USG::VulkanRendererLogic::SetUpPipeline(VkCommandBuffer& commandBuffer)
{
	UpdateWindowDimensions(); // what is the current client area dimensions?
	SetViewport(commandBuffer);
	SetScissor(commandBuffer);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	BindVertexBuffers(commandBuffer, staticStepBinding);
	BindIndexBuffers(commandBuffer, staticStepBinding);
}

void USG::VulkanRendererLogic::SetViewport(const VkCommandBuffer& commandBuffer)
{
	VkViewport viewport = CreateViewportFromWindowDimensions();
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void USG::VulkanRendererLogic::SetScissor(const VkCommandBuffer& commandBuffer)
{
	VkRect2D scissor = CreateScissorFromWindowDimensions();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void USG::VulkanRendererLogic::BindVertexBuffers(VkCommandBuffer& commandBuffer, unsigned buffNum)
{
	VkBuffer vertexBuffers[] = { vertexHandle, dynamicVertexBuffer };
	VkDeviceSize offsets[] = { 0 , 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[buffNum], offsets);
}

void USG::VulkanRendererLogic::BindIndexBuffers(VkCommandBuffer& commandBuffer, unsigned buffNum) {
	VkBuffer indexBuffers[] = { indexHandle, dynamicIndexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindIndexBuffer(commandBuffer, indexBuffers[buffNum], offsets[0], VK_INDEX_TYPE_UINT32);
}
#pragma endregion

bool USG::VulkanRendererLogic::FreeVulkanResources()
{
	// wait till everything has completed
	DeleteRendererMemory();

	return true;
}

void USG::VulkanRendererLogic::DeleteRendererMemory() {
	// free the uniform buffer and its handle
	vkDeviceWaitIdle(device);
	for (int i = 0; i < uniformBufferData.size(); ++i) {
		vkDestroyBuffer(device, uniformBufferHandle[i], nullptr);
		vkFreeMemory(device, uniformBufferData[i], nullptr);
	}
	uniformBufferData.clear();
	uniformBufferHandle.clear();
	// do the same for the storage buffer
	for (int i = 0; i < storageBufferData.size(); ++i) {
		vkDestroyBuffer(device, storageBufferHandle[i], nullptr);
		vkFreeMemory(device, storageBufferData[i], nullptr);
	}
	storageBufferData.clear();
	storageBufferHandle.clear();
	// don't need the descriptors anymore
	vkDestroyDescriptorSetLayout(device, uniformBuffDescriptorLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, storageBuffDescriptorLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	// Release allocated buffers, shaders & pipeline
	vkDestroyBuffer(device, vertexHandle, nullptr);
	vkFreeMemory(device, vertexData, nullptr);
	vkDestroyBuffer(device, dynamicVertexBuffer, nullptr);
	vkFreeMemory(device, dynamicVertexData, nullptr);

	vkDestroyBuffer(device, indexHandle, nullptr);
	vkFreeMemory(device, indexData, nullptr);
	vkDestroyBuffer(device, dynamicIndexBuffer, nullptr);
	vkFreeMemory(device, dynamicIndexData, nullptr);

	vkDestroyShaderModule(device, vertexShader, nullptr);
	vkDestroyShaderModule(device, fragmentShader, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
}
