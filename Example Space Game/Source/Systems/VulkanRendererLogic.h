// The rendering system is responsible for drawing all objects
#ifndef VULKANRENDERERLOGIC_H
#define VULKANRENDERERLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"
#include "../../Level-Import/h2bParser.h"
#include "../../Level-Import/LevelDataLoad.h"// idk why I need to add this?(DD 1/15/24)
#include "vkDescriptorSetsHandler.h"
#include "DebugCameraControl.h"

// example space game (avoid name collisions)
namespace USG
{
	class VulkanRendererLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// handle to our running ECS systems
		flecs::system startDraw;
		flecs::system updateDraw;
		flecs::system updateDrawInstructions;
		flecs::system findPointLights;
		flecs::system completeDraw;
		// Used to query screen dimensions
		GW::SYSTEM::GWindow win;
		// Level Data handle added 1/11/24 by DD
		LEVEL_DATA levelData;
		LEVEL_DATA dynamicModelData;
		std::vector<GW::MATH::GVECTORF> pointLightVec;// added 3/24/24 by DD
		// Vulkan resources used for rendering
		GW::GRAPHICS::GVulkanSurface vlk;
		VkDevice device = nullptr;// added 1/11/24 by DD
		VkPhysicalDevice physicalDevice = nullptr;
		VkRenderPass renderPass;

		VkBuffer vertexHandle = nullptr;
		VkDeviceMemory vertexData = nullptr;
		VkBuffer dynamicVertexBuffer = nullptr;// added 1/29/24 by DD
		VkDeviceMemory dynamicVertexData = nullptr;
		VkBuffer indexHandle = nullptr;// added index buffer 1/15/24 by DD
		VkDeviceMemory indexData = nullptr;
		VkBuffer dynamicIndexBuffer = nullptr;// added 1/29/24 by DD
		VkDeviceMemory dynamicIndexData = nullptr;

		VkShaderModule vertexShader = nullptr;
		VkShaderModule fragmentShader = nullptr;// same as pixel shader 
		VkDescriptorPool descriptorPool = nullptr;
		VkPipeline pipeline = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		unsigned int windowWidth, windowHeight;
		//Uniform buffer
		std::vector<VkBuffer> uniformBufferHandle;
		std::vector<VkDeviceMemory> uniformBufferData;
		std::vector<VkDescriptorSet> uniformBufferDescriptorSet;
		VkDescriptorSetLayout uniformBuffDescriptorLayout = nullptr;
		//storage buffer changed by DD on 1/11/24
		std::vector<VkBuffer> storageBufferHandle;
		std::vector<VkDeviceMemory> storageBufferData;
		std::vector<VkDescriptorSet> storageBufferDescriptorSet;
		VkDescriptorSetLayout storageBuffDescriptorLayout = nullptr;
		// used to trigger clean up of vulkan resources
		GW::CORE::GEventReceiver shutdown;
		// Gateware Proxys added by DD 1/14/24
		GW::MATH::GMatrix matrixProxy;
		GW::MATH::GVector vecProxy;
		// light and camera variables added by DD 1/14/24
		GW::MATH::GVECTORF lightDirVec;
		GW::MATH::GVECTORF lightColorVec;
		GW::MATH::GVECTORF ambientColorVec;
		GW::MATH::GMATRIXF cameraPosMatrix;
		GW::MATH::GVECTORF cameraPosVec;
		// added by DD 3/14/24
		DebugCameraControl debugCamera;
		VKDescriptorSetHandler descriptorHandler;
		// determines the end position for the world matrix array with the static level data
		size_t worldMatrixStaticLevelDataEndPosition;
		size_t materialDataStaticLevelDataEndPosition;
		unsigned pointLightCount = 0;
		bool lineRenderFilter;
		bool shutdownStarted = false;
	public:
		std::vector<GW::MATH::GMATRIXF> levelDataWorldMats;
		// attach the required logic to the ECS 
		bool Init(std::shared_ptr<flecs::world> _game,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::GRAPHICS::GVulkanSurface _vulkan,
			GW::SYSTEM::GWindow _window, LEVEL_DATA _levelData,
			LEVEL_DATA _dynamicModelData, std::vector<GW::MATH::GVECTORF>_pointLightVec, GW::INPUT::GInput _input, bool _lineRender);
		// control if the system is actively running
		bool Activate(bool runSystem);
		// release any resources allocated by the system
		bool Shutdown();
		// added by DD 1/14/24
		// gives people outside to update the camera pos to the view matrix
		void UpdateCameraPos();
		GW::MATH::GVECTORF GetCameraPos() { return cameraPosVec; }
		void UpdateViewMatrix(GW::MATH::GMATRIXF newCamPos);
		GW::MATH::GMATRIXF GetViewMatrix() { return SCENE_DATA.viewMatrix[0]; }// added 3/25/24 by DD
		GW::MATH::GMATRIXF GetProjectionMatrix() { return SCENE_DATA.projMatrix[0]; }
		unsigned GetWindowWidth() { return windowWidth; }
		unsigned GetWindowHeight() { return windowHeight; }
		void DeleteRendererMemory();
	private:
		//constructor helper functions
		void UpdateWindowDimensions();
		// Matrix Initialization Funcs added 1/14/24 by DD
		void InitializeWorldMatrix();
		void InitializeViewMatrix();
		void InitializeCameraPosMatrix();
		void InitializeProjMatrix();
		void InitializeMatrices();


		// Light Initialization
		void InitializeLightDir();
		void InitializeLightColor();
		void InitializeAmbientLight();
		void InitializeMaterialData();
		void InitializePointLightPos();
		void InitializePointLightColor();
		void InitializePointLights();
		void InitializeSpotLightPos();
		void InitializeSpotLightColor();
		void InitializeSpotLightAngle();
		void InitializeSpotLights();
		void InitializeLight();

		void GetHandlesFromSurface();// get handles from vlk added 1/15/24

		// added by DD 1/14/24
		// used in UpdateCameraPos
		void UpdateCameraPosVec();
		// Loading funcs
		bool LoadShaders();
		bool LoadDescriptorBuffers();
		bool LoadGeometry();
		// 2 LoadGeometry Helper Funcs added by DD 1/15/24
		bool LoadVertexBuffer(const void* data, unsigned int sizeInBytes, VkBuffer* buffer, VkDeviceMemory* deviceMem);
		bool LoadIndexBuffer(const void* data, unsigned int sizeInBytes, VkBuffer* buffer, VkDeviceMemory* deviceMem);
		bool SetupPipeline();
		// Graphics Pipeline Helper Functions
		VkPipelineInputAssemblyStateCreateInfo CreateVkPipelineInputAssemblyStateCreateInfo();
		VkVertexInputBindingDescription CreateVkVertexInputBindingDescription();
		VkPipelineVertexInputStateCreateInfo CreateVkPipelineVertexInputStateCreateInfo(
			VkVertexInputBindingDescription* bindingDescriptions, uint32_t bindingCount,
			VkVertexInputAttributeDescription* attributeDescriptions, uint32_t attributeCount);
		VkViewport CreateViewportFromWindowDimensions();
		VkRect2D CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo CreateVkPipelineViewportStateCreateInfo(VkViewport* viewports, uint32_t viewportCount, VkRect2D* scissors, uint32_t scissorCount);
		VkPipelineRasterizationStateCreateInfo CreateVkPipelineRasterizationStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo CreateVkPipelineDepthStencilStateCreateInfo();
		VkPipelineColorBlendAttachmentState CreateVkPipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo CreateVkPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState* attachmentStates, uint32_t attachmentCount);
		VkPipelineDynamicStateCreateInfo CreateVkPipelineDynamicStateCreateInfo(VkDynamicState* dynamicStates, uint32_t dynamicStateCount);
		void CreatePipelineLayout();
		void SetUpPushConstantData(VkPushConstantRange* _pushConstantRange);

		bool SetupDrawcalls();
		// Drawing Helper Functions
		void UpdateStorageBuffer();
		VkCommandBuffer GetCurrentCommandBuffer();
		void SetUpPipeline(VkCommandBuffer& commandBuffer);
		void SetViewport(const VkCommandBuffer& commandBuffer);
		void SetScissor(const VkCommandBuffer& commandBuffer);
		void BindVertexBuffers(VkCommandBuffer& commandBuffer, unsigned buffNum);
		void BindIndexBuffers(VkCommandBuffer& commandBuffer, unsigned buffNum);
		// Unloading funcs
		bool FreeVulkanResources();
		// Utility funcs
		std::string ShaderAsString(const char* shaderFilePath);
	private:
		// Uniform Data Definitions
		static constexpr unsigned int Instance_Max = 240;
		struct SceneData {// Uniform Buffer
			GW::MATH::GMATRIXF viewMatrix[2], projMatrix[2];
			// directional lighting
			GW::MATH::GVECTORF lightDir, lightColor, cameraPos, ambientColor;
			// point lighting
			GW::MATH::GVECTORF pointLightPos[MAX_NUM_LIGHT_SOURCES];
			GW::MATH::GVECTORF pointLightColor[MAX_NUM_LIGHT_SOURCES];// w is intensity
			// spot lighting
			GW::MATH::GVECTORF spotLightPos[MAX_NUM_LIGHT_SOURCES];
			GW::MATH::GVECTORF spotLightColor[MAX_NUM_LIGHT_SOURCES];// w is intensity
			// removed for time reasons
			/*GW::MATH::GVECTORF spotLightDir[MAX_NUM_LIGHT_SOURCES];*/
			float spotLightAngle;
		}SCENE_DATA;
		struct MeshData {// Storage Buffer
			//unsigned int transformStart;
			H2B::ATTRIBUTES materialData[MAX_MATRIX_SIZE];
			GW::MATH::GMATRIXF worldMatrices[MAX_MATRIX_SIZE];
		}MESH_DATA;
		struct PushConstantData {// 
			unsigned int materialIndex;
			unsigned int wMatrixInstanceOffset;
			unsigned int greyscaleFilter = 0;
			unsigned int invertedColorsFilter = 0;
			unsigned int isOrthogonal;
		}PUSH_CONSTANT_DATA;
		// how many instances will be drawn this frame(probably won't be used)
		int draw_counter = 0;
		struct DrawInstructions {
			std::vector<GW::MATH::GMATRIXF> worldMatrices;// also used to determine the instance count
			std::vector<H2B::MESH> modelMeshes;// sub-meshes
			LEVEL_DATA::MODEL_INSTANCES modelInstancesData;
			LEVEL_DATA::MODEL_DATA model;
		};
		std::vector<DrawInstructions> staticDrawInstructions;
		std::vector<DrawInstructions> dynamicDrawInstructions;

	};
};

#endif


