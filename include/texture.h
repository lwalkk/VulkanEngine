#pragma once
#include "vulkan_headers.h"
#include <string>


class Texture {
public:
  Texture(const InitData& instance, const std::string& filepath, VkCommandPool command_pool);

  inline VkImage Image() const { return texture_image_; }
  inline VkDeviceMemory Memory() const { return texture_image_memory_; }
  inline VkImageView ImageView() const { return texture_image_view_; }
  inline VkSampler Sampler() const { return texture_sampler_; }

private:

  void CopyBufferToImage(const InitData& instance, VkBuffer buffer, VkImage image,
    uint32_t width, uint32_t height, VkCommandPool command_pool);

  void CreateImage(const InitData& instance, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);

  void TransitionImageLayout(const InitData& instance, VkImage image, VkFormat format, VkImageLayout old_layout,
    VkImageLayout new_layout, VkCommandPool command_pool);

  VkImage texture_image_;
  VkDeviceMemory texture_image_memory_;
  VkImageView texture_image_view_;
  VkSampler texture_sampler_;

  InitData instance_;
  VkCommandPool command_pool_;
};
