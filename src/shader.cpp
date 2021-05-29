#include "shader.h" 

std::string ReadFile(const char* filepath) {
  std::ifstream file;

  if (file.fail()) {
    throw std::runtime_error("could not open " + *filepath);
  }

  file.open(filepath);
  std::stringstream sstream;
  sstream << file.rdbuf();
  file.close();

  std::string code = sstream.str();
  return code;
}

std::vector<uint32_t> CompileShader(const std::string& source_name, shaderc_shader_kind shader_kind,
  const std::string& source, bool optimize = false) {

  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  if (optimize) {
    options.SetOptimizationLevel(shaderc_optimization_level_size);
  }

  shaderc::SpvCompilationResult module =
    compiler.CompileGlslToSpv(source, shader_kind, source_name.c_str(), options);

  if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << module.GetErrorMessage() << std::endl;
    return std::vector<uint32_t>();
  }

  return { module.cbegin(), module.cend() };
}

VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code, const InitData& init) {
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = sizeof(uint32_t) * code.size();
  create_info.pCode = code.data();

  VkShaderModule shader_module;
  if (vkCreateShaderModule(init.device, &create_info, nullptr, &shader_module) != VK_SUCCESS){ 
    throw std::runtime_error("failed to create shader module!");
  }

  return shader_module;
}

Shader::Shader(std::string shader_name, std::string file_name, ShaderType type, const InitData& init) {

  std::string shader_source = ReadFile(shader_name.c_str());

  shaderc_shader_kind kind;
  VkShaderStageFlagBits type_flag;

  if (type == ShaderType::VERTEX_SHADER) {
    kind = shaderc_glsl_vertex_shader;
    type_flag = VK_SHADER_STAGE_VERTEX_BIT;
  }
  else {
    kind = shaderc_glsl_fragment_shader;
    type_flag = VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  std::vector<uint32_t> shader_bin = CompileShader(shader_name, kind, shader_source);

  module_ = CreateShaderModule(shader_bin, init);

  info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info_.stage = type_flag;
  info_.module = module_;
  info_.pName = shader_name.c_str();
}

