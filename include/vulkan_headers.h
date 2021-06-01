#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct InitData {

  GLFWwindow* window;

  VkInstance instance;
  VkSurfaceKHR surface;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkDevice device;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;

};
