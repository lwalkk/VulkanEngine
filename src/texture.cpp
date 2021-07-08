#include "texture.h"
#include "buffer.h"
#include <stb_image.h>

Texture::Texture(const InitData& instance, const std::string& filepath, VkCommandPool command_pool) :
instance_(instance), command_pool_(command_pool) {

  int tex_width, tex_height, tex_channels;
  stbi_uc* pixels = stbi_load(filepath.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
  VkDeviceSize image_size = tex_width * tex_height * 4;

  if (!pixels) {
    throw std::runtime_error("failed to load texture!");
  }

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;

  Buffer::CreateBuffer(instance, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

  void* data;
  vkMapMemory(instance_.device, staging_buffer_memory, 0, image_size, 0, &data);
  memcpy(data, pixels, static_cast<uint32_t>(image_size));
  vkUnmapMemory(instance_.device, staging_buffer_memory);

  stbi_image_free(pixels);

  CreateImage(instance, tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image_, texture_image_memory_);

  TransitionImageLayout(instance_, texture_image_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command_pool_);

  CopyBufferToImage(instance_, staging_buffer, texture_image_, static_cast<uint32_t>(tex_width),
    static_cast<uint32_t>(tex_height), command_pool);

  TransitionImageLayout(instance_, texture_image_, VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_pool_);

  vkDestroyBuffer(instance_.device, staging_buffer, nullptr);
  vkFreeMemory(instance_.device, staging_buffer_memory, nullptr);
}

bool HasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Texture::CopyBufferToImage(const InitData& instance,  VkBuffer buffer, VkImage image, 
  uint32_t width, uint32_t height, VkCommandPool command_pool) {

  VkCommandBuffer command_buffer = Buffer::BeginSingleTimeCommands(instance, command_pool);

  // need to specify which part of the buffer will be copied to which part of the image
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = { width, height, 1 };

  vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &region);

  Buffer::EndSingleTimeCommands(instance, command_buffer, command_pool);
}

void Texture::TransitionImageLayout(const InitData& instance, VkImage image, VkFormat format, VkImageLayout old_layout,
  VkImageLayout new_layout, VkCommandPool command_pool)  {

  VkCommandBuffer command_buffer = Buffer::BeginSingleTimeCommands(instance, command_pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0; barrier.subresourceRange.layerCount = 1;


    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    // Need to deal with  two different types of layout transitions:
    // undefined -> transfer destination AND
    // Transfer destination -> shader reading 
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
      throw std::invalid_argument("unsupported layout transition");
    }

    if(new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

      if(HasStencilComponent(format)) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
    }
    else {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    Buffer::EndSingleTimeCommands(instance, command_buffer, command_pool);
  }


void Texture::CreateImage(const InitData& instance, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
  VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory) {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = tiling;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.flags = 0;

  if (vkCreateImage(instance.device, &image_info, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image!");
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(instance.device, image, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = Buffer::FindMemoryType(instance, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(instance.device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(instance.device, image, image_memory, 0);
}
