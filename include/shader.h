#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <shaderc/shaderc.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
enum class ShaderType{
  VERTEX_SHADER, 
  FRAGMENT_SHADER 
};

class Shader {
public:
  Shader(std::string shader_name, std::string file_name, ShaderType type);
  ~Shader();

  VkShaderModule GetModule() { return module_; }
  VkPipelineShaderStageCreateInfo GetInfo() { return info_; }

private:
  VkShaderModule module_;
  VkPipelineShaderStageCreateInfo info_{};
};