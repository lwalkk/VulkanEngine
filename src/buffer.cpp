#include "buffer.h"
#include <iostream>


Buffer::Buffer()
{
}

Buffer::Buffer(const InitData& init, const RenderData& ren_dat, VkDeviceSize buffer_size, VkBufferUsageFlags usage, void* buffer_data) {

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  
  CreateBuffer(init, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

  void* data;
  vkMapMemory(init.device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, buffer_data, (size_t)buffer_size);
  vkUnmapMemory(init.device, staging_buffer_memory);

  CreateBuffer(init, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer_, buffer_memory_);

  CopyBuffer(init, ren_dat.command_pool, staging_buffer, buffer_, buffer_size);

  vkDestroyBuffer(init.device, staging_buffer, nullptr);
  vkFreeMemory(init.device, staging_buffer_memory, nullptr);
}

void Buffer::CreateBuffer(const InitData& init, VkDeviceSize size, VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(init.device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer");
  }

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(init.device, buffer, &memory_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = memory_requirements.size;
  alloc_info.memoryTypeIndex = FindMemoryType(init, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(init.device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate memory");
  }

  vkBindBufferMemory(init.device, buffer, buffer_memory, 0);
}

void Buffer::CopyBuffer(const InitData& init, VkCommandPool command_pool, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
  VkCommandBuffer command_buffer = BeginSingleTimeCommands(init, command_pool);

  VkBufferCopy copy_region{};
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  EndSingleTimeCommands(init, command_buffer, command_pool);
}

uint32_t Buffer::FindMemoryType(const InitData& init, uint32_t type_filter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(init.physical_device, &memory_properties);

  for (uint32_t ii = 0; ii < memory_properties.memoryTypeCount; ii++) {
    if ((type_filter & (1 << ii)) &&
      (memory_properties.memoryTypes[ii].propertyFlags & properties) == properties) {
      return ii;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer Buffer::BeginSingleTimeCommands(const InitData& instance, VkCommandPool command_pool) {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(instance.device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

void Buffer::EndSingleTimeCommands(const InitData& instance, VkCommandBuffer command_buffer, VkCommandPool command_pool) {
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(instance.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(instance.graphics_queue);
  vkFreeCommandBuffers(instance.device, command_pool, 1, &command_buffer);
}

