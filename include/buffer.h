#pragma once
#include "vulkan_headers.h"
#include <stdexcept>

class Buffer {
public:

  Buffer();
  Buffer(const InitData& instance, const RenderData& ren_dat, VkDeviceSize buffer_size, VkBufferUsageFlags usage, void* data);

  VkBuffer GetBuffer() const { return buffer_; }
  VkDeviceMemory GetBufferMemory() const { return buffer_memory_; }

  virtual void Bind() = 0;

  // right now this is an abstract class - need to make derived classes
  // which implement vertex buffers, uniform buffers etc

private:
  static void CreateBuffer(const InitData& init, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);

  static void CopyBuffer(const InitData& init, VkCommandPool command_pool, 
    VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

  static VkCommandBuffer BeginSingleTimeCommands(const InitData& init,
    VkCommandPool command_pool);

  static void EndSingleTimeCommands(const InitData& instance, VkCommandBuffer command_buffer,  VkCommandPool command_pool);

  static uint32_t FindMemoryType(const InitData& init, uint32_t type_filter, VkMemoryPropertyFlags properties);

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory buffer_memory_ = VK_NULL_HANDLE;

  friend class Texture;
};
