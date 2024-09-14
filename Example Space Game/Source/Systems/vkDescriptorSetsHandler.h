// The rendering system is responsible for drawing all objects
#ifndef VKDESCRIPTORSETHANDLER_H
#define VKDESCRIPTORSETHANDLER_H

// Contains our global game settings

// example space game (avoid name collisions)
namespace USG
{
	class VKDescriptorSetHandler
	{
		VkDevice device;
		// vk descriptor set layout functions
		VkDescriptorSetLayoutBinding SetLayoutBinding(unsigned _binding, VkDescriptorType _bufferType, VkShaderStageFlagBits _allocatedShaders);
		VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo(VkDescriptorSetLayoutBinding* _layoutBinding);
		void AllocateDescriptorSets(std::vector<VkBuffer> _buffVec, std::vector<VkDescriptorSet>* _descriptorSets,
			VkDescriptorPool _descriptorPool, VkDescriptorSetLayout* _descriptorSetLayout);
		void WriteDescriptorSets(unsigned _binding, VkDescriptorType _bufferType, std::vector<VkDescriptorSet> _descriptorSets, std::vector<VkBuffer> _buffVec);
	public:
		void Init(VkDevice _device);
		VkDescriptorPool CreateDescriptorPool(unsigned _dataSize, unsigned _poolCount, VkDescriptorPoolSize* _descriptorPoolComponentArr);
		void DescriptorSetSetupPipeline(unsigned _binding, VkDescriptorType _bufferType, VkShaderStageFlagBits _allocatedShaders, VkDescriptorSetLayout* _descriptorSetLayout,
			VkDescriptorPool _descriptorPool, std::vector<VkDescriptorSet>* _descriptorSets, std::vector<VkBuffer> _buffVec);
	};
};

#endif


