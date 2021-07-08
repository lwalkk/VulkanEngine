#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <vector>

class Texture;


struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  static VkVertexInputBindingDescription GetBindingDescription() {
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
  }

  // need to specify format of position and color of attribute hence the array
  static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescription() {
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

    return attribute_descriptions;
  }
};

struct Mesh {
  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
  // can a model have multiple textures?
  std::vector<Texture> textures_;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct InitData {

  GLFWwindow* window;

  VkInstance instance;
  VkSurfaceKHR surface;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkDevice device;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;

  VkQueue graphics_queue;
  VkQueue presentation_queue;

};


// TO BE EXPANDED
struct RenderData {
  VkCommandPool command_pool;
};
