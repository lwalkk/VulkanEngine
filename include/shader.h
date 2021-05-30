#pragma once
#include "vulkan_headers.h"
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
  Shader(const char* file_name, const char* entry_name, ShaderType type , const InitData& init);
  ~Shader();

  VkShaderModule GetModule() const { return module_; }
  VkPipelineShaderStageCreateInfo GetInfo() const { return info_; }

private:
  VkShaderModule module_;
  VkPipelineShaderStageCreateInfo info_{};
  const InitData& init_;
  std::string entry_;
};