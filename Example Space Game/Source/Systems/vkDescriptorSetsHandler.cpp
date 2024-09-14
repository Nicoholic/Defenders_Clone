#include "vkDescriptorSetsHandler.h"


VkDescriptorSetLayoutBinding USG::VKDescriptorSetHandler::SetLayoutBinding(unsigned _binding, VkDescriptorType _bufferType, VkShaderStageFlagBits _allocatedShaders) {
	// Set Layout for storage buffer
	VkDescriptorSetLayoutBinding returnVal = {};
	returnVal.binding = _binding;
	returnVal.descriptorCount = 1;
	returnVal.descriptorType = _bufferType;
	returnVal.pImmutableSamplers = nullptr;
	returnVal.stageFlags = _allocatedShaders;
	return returnVal;
}

VkDescriptorSetLayoutCreateInfo USG::VKDescriptorSetHandler::SetLayoutCreateInfo(VkDescriptorSetLayoutBinding* _layoutBinding) {
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
	descriptorSetLayoutInfo.bindingCount = 1;
	descriptorSetLayoutInfo.flags = 0;
	descriptorSetLayoutInfo.pBindings = _layoutBinding;
	descriptorSetLayoutInfo.pNext = nullptr;
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	return descriptorSetLayoutInfo;
}

VkDescriptorPool USG::VKDescriptorSetHandler::CreateDescriptorPool(unsigned _dataSize, unsigned _poolCount, VkDescriptorPoolSize* _descriptorPoolComponentArr) {
	VkDescriptorPool returnVal;
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.flags = 0;
	descriptorPoolInfo.maxSets = _dataSize;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = _poolCount;
	descriptorPoolInfo.pPoolSizes = _descriptorPoolComponentArr;
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	{
		auto hr = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &returnVal);
	}
	return returnVal;
}

void USG::VKDescriptorSetHandler::AllocateDescriptorSets(std::vector<VkBuffer> _buffVec, std::vector<VkDescriptorSet>* _descriptorSets,
	VkDescriptorPool _descriptorPool, VkDescriptorSetLayout* _descriptorSetLayout) {
	_descriptorSets->resize(_buffVec.size());
	// STORAGE BUFFER
	VkDescriptorSetAllocateInfo descriptorAllocationInfo = {};
	descriptorAllocationInfo.descriptorSetCount = 1;
	descriptorAllocationInfo.pNext = nullptr;
	descriptorAllocationInfo.descriptorPool = _descriptorPool;
	descriptorAllocationInfo.pSetLayouts = _descriptorSetLayout;
	descriptorAllocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	// swapchain swaps buffer being used(makes 2 buffers)
	for (size_t i = 0; i < _descriptorSets->size(); i++) {
		auto hr = vkAllocateDescriptorSets(device, &descriptorAllocationInfo, &_descriptorSets->data()[i]);
	}
}

void USG::VKDescriptorSetHandler::WriteDescriptorSets(unsigned _binding, VkDescriptorType _bufferType, std::vector<VkDescriptorSet> _descriptorSets, std::vector<VkBuffer> _buffVec) {
	VkWriteDescriptorSet descriptorWrite = {};//8
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = _bufferType;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.dstBinding = _binding;
	descriptorWrite.pNext = nullptr;
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	for (size_t i = 0; i < _buffVec.size(); i++) {
		descriptorWrite.dstSet = _descriptorSets[i];
		VkDescriptorBufferInfo descriptorBufferInfo = { _buffVec[i], 0, VK_WHOLE_SIZE };
		descriptorWrite.pBufferInfo = &descriptorBufferInfo;
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}

void USG::VKDescriptorSetHandler::Init(VkDevice _device) {
	device = _device;
}

void USG::VKDescriptorSetHandler::DescriptorSetSetupPipeline(unsigned _binding, VkDescriptorType _bufferType, VkShaderStageFlagBits _allocatedShaders, VkDescriptorSetLayout* _descriptorSetLayout,
	VkDescriptorPool _descriptorPool, std::vector<VkDescriptorSet>* _descriptorSets, std::vector<VkBuffer> _buffVec) {
	VkDescriptorSetLayoutBinding layoutBinding = SetLayoutBinding(_binding, _bufferType, _allocatedShaders);
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = SetLayoutCreateInfo(&layoutBinding);
	auto hr = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, _descriptorSetLayout);
	AllocateDescriptorSets(_buffVec, _descriptorSets, _descriptorPool, _descriptorSetLayout);
	WriteDescriptorSets(_binding, _bufferType, *_descriptorSets, _buffVec);
}