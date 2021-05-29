#pragma once


struct InitData {

  GLFWwindow* window;

  VkInstance instance;
  VkSurfaceKHR surface;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkDevice device;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;

};