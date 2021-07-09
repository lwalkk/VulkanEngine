#pragma once 
#include "headers.h"
#include "tiny_obj_loader.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const std::vector<const char*> validation_layers = {
  "VK_LAYER_KHRONOS_validation"
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
  const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
    "vkCreateDebugUtilsMessengerEXT");

  if (func != nullptr) {
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  }
  else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
  const VkAllocationCallbacks* p_allocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
    "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debug_messenger, p_allocator);
  }
}

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool IsComplete() {
    return graphics_family.has_value() && present_family.has_value();
  }
};

// struct to get necessary swap chain information
struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

class VulkanEngine {
public:
  void run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
  }

private:

  void MainLoop() {
    while (!glfwWindowShouldClose(instance.window)) {
      glfwPollEvents();
      DrawFrame();
    }
    vkDeviceWaitIdle(instance.device);
  }
  void Cleanup() {

    CleanupSwapChain();

    vkDestroySampler(instance.device, texture_sampler, nullptr);
    vkDestroyImageView(instance.device, texture_image_view, nullptr);
    vkDestroyImage(instance.device, texture_image, nullptr);
    vkFreeMemory(instance.device, texture_image_memory, nullptr);

    vkDestroyDescriptorSetLayout(instance.device, descriptor_set_layout, nullptr);

    vkDestroyBuffer(instance.device, vert_buffer.GetBuffer(), nullptr);
    vkFreeMemory(instance.device, vert_buffer.GetBufferMemory(), nullptr);

    vkDestroyBuffer(instance.device, ind_buffer.GetBuffer(), nullptr);
    vkFreeMemory(instance.device, ind_buffer.GetBufferMemory(), nullptr);

    for (size_t ii = 0; ii < MAX_FRAMES_IN_FLIGHT; ii++) {
      vkDestroySemaphore(instance.device, render_finished_semaphores[ii], nullptr);
      vkDestroySemaphore(instance.device, image_available_semaphores[ii], nullptr);
      vkDestroyFence(instance.device, in_flight_fences[ii], nullptr);
    }

    vkDestroyCommandPool(instance.device, command_pool, nullptr);
    vkDestroyDevice(instance.device, nullptr);

    if (enable_validation_layers) {
      DestroyDebugUtilsMessengerEXT(instance.instance, instance.debug_messenger, nullptr);
    }

    vkDestroySurfaceKHR(instance.instance, instance.surface, nullptr);
    vkDestroyInstance(instance.instance, nullptr);
    glfwDestroyWindow(instance.window);
    glfwTerminate();
  }

  void InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    instance.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateDepthResources();
    CreateFrameBuffers();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
    LoadModel();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
  }

  static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->frame_buffer_resized = true;
  }

  // something interesting that could be implemented is when we loop through
  // each devicein the physical devices list, choose the GPU which 'scores' 
  // the highest based on some criteria -- eg. Discrete GPU vs Integrated

  bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensions_supported = CheckDeviceExtensionSupport(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
      SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(device);
      swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);


    return indices.IsComplete() && extensions_supported && swap_chain_adequate &&
      supported_features.samplerAnisotropy;
  }

  bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    // get available extensions
    std::vector<VkExtensionProperties> availiable_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
      availiable_extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : availiable_extensions) {
      required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();

  }

  void PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance.instance, &device_count, nullptr);

    if (device_count == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance.instance, &device_count, devices.data());

    for (const auto& device_ : devices) {
      if (isDeviceSuitable(device_)) {
        instance.physical_device = device_;
        break;
      }
    }

    if (instance.physical_device == VK_NULL_HANDLE) {
      throw std::runtime_error("failed to find a suitable GPU!");
    }

  }

  QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
    // logic to find queue families
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    // get the physical device queue properties
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int ii = 0;
    // iterate through the queue family and check if at least one queue family 
    // supports the queue graphics bit?
    for (const auto& queue_family : queue_families) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphics_family = ii;
      }

      // look for a queue family that has capability of presenting to our window surface
      // ii is the queue family index
      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, ii, instance.surface, &present_support);

      if (present_support) {
        indices.present_family = ii;
      }

      if (indices.IsComplete()) {
        break;
      }
      ii++;
    }

    return indices;
  }

  void CreateLogicalDevice() {
    // create infos for logical devices
    QueueFamilyIndices indices = FindQueueFamilies(instance.physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
      VkDeviceQueueCreateInfo queue_create_info{};
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queue_family;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &queue_priority;
      queue_create_infos.push_back(queue_create_info);

    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

    // add validation layer info
    if (enable_validation_layers) {
      create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
      create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else {
      create_info.enabledLayerCount = 0;
    }

    // create logical device
    if (vkCreateDevice(instance.physical_device, &create_info, nullptr, &instance.device) != VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device!");
    }
    // retrieve queue handles for each queue family
    vkGetDeviceQueue(instance.device, indices.graphics_family.value(), 0, &instance.graphics_queue);
    vkGetDeviceQueue(instance.device, indices.graphics_family.value(), 0, &presentation_queue);
  }

  // get the swap chain support details
  SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, instance.surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, instance.surface, &format_count, nullptr);

    if (format_count != 0) {
      details.formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, instance.surface, &format_count, details.formats.data());
    }

    uint32_t presentation_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, instance.surface, &presentation_mode_count, nullptr);
    if (presentation_mode_count != 0) {
      details.present_modes.resize(presentation_mode_count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, instance.surface, &presentation_mode_count, details.present_modes.data());
    }

    return details;
  }

  // Function to choose the swap chain surface format
  // REMINDER : The swap chain is a queue of images that are waiting to be presented to the screen
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
      if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return available_format;
      }
    }
    return available_formats[0];
  }
  // choose presentation mode - ie the conditions for how images get shown to the screen
  // mailbox mode -- rendered images are submitted to a queue and the ones at the back get replaced when its full
  VkPresentModeKHR ChooseSwapPresentMode(std::vector<VkPresentModeKHR>& availiable_present_modes) {
    for (const auto& available_present_mode : availiable_present_modes) {
      if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return available_present_mode;
      }
    }
    // default queue
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  // the swap extent is the resolution of the swap chain images and is (almost) 
  // always equal to the resolution (in pixels) of the window we're rendering to 
  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // 
    if (capabilities.currentExtent.width != UINT32_MAX) {
      return capabilities.currentExtent;
    }
    else {

      int width, height;
      glfwGetFramebufferSize(instance.window, &width, &height);

      VkExtent2D actual_extent{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
      };

      actual_extent.width = std::max(capabilities.minImageExtent.width,
        std::min(capabilities.minImageExtent.width, actual_extent.width));

      actual_extent.height = std::max(capabilities.minImageExtent.height,
        std::min(capabilities.minImageExtent.height, actual_extent.height));

      return actual_extent;
    }
  }

  // use all the helper functions I created to create the swap chain 
  void CreateSwapChain()
  {
    SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(instance.physical_device);
    VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count >
      swap_chain_support.capabilities.maxImageCount) {
      image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = instance.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    // amount of layers each image consists of
    // always one unless 3D stereoscopic ? 
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(instance.physical_device);
    uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

    if (indices.graphics_family != indices.present_family) {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(instance.device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
      throw std::runtime_error("failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(instance.device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(instance.device, swap_chain, &image_count, swap_chain_images.data());

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
  }

  void CreateImageViews() {
    swap_chain_image_views.resize(swap_chain_images.size());

    for (size_t ii = 0; ii < swap_chain_images.size(); ii++) {
      swap_chain_image_views[ii] = CreateImageView(swap_chain_images[ii], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
  }

  void CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // one uniform buffer, could pass in an array of ubos and we would need to
    // put the number of elements in the array
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.pImmutableSamplers = nullptr; // optional
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(instance.device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }

  }

  void CreateGraphicsPipeline() {

    Shader vert_shader("shaders/vert.glsl", "main", ShaderType::VERTEX_SHADER, instance);
    Shader frag_shader("shaders/frag.glsl", "main", ShaderType::FRAGMENT_SHADER, instance);

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader.GetInfo(), frag_shader.GetInfo() };

    auto binding_description = Vertex::GetBindingDescription();
    auto attribute_descriptions = Vertex::GetAttributeDescription();
    // vertex input stage
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // input assembly stage
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // viewports and scissors
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain_extent.width;
    viewport.height = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    // rasterizer 
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional


    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(instance.device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline layout!");
    }

    // we'll use this to enable depth testing in the graphics pipeline
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f; // optional
    depth_stencil.maxDepthBounds = 1.0f; // optional
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {}; // optional 
    depth_stencil.back = {}; // optional

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(instance.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) !=
      VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }
  }


  void CreateRenderPass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = FindDepthFormat();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(instance.device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass!");
    }
  }

  void CreateFrameBuffers() {
    swap_chain_framebuffers.resize(swap_chain_images.size());

    for (size_t ii = 0; ii < swap_chain_image_views.size(); ii++) {
      std::array<VkImageView, 2> attachments = { swap_chain_image_views[ii], depth_image_view };
      VkFramebufferCreateInfo framebuffer_info{};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_pass;
      framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
      framebuffer_info.pAttachments = attachments.data();
      framebuffer_info.width = swap_chain_extent.width;
      framebuffer_info.height = swap_chain_extent.height;
      framebuffer_info.layers = 1;

      if (vkCreateFramebuffer(instance.device, &framebuffer_info, nullptr, &swap_chain_framebuffers[ii]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer");
      }

    }

  }

  void CreateCommandPool() {
    QueueFamilyIndices queue_family_indices = FindQueueFamilies(instance.physical_device);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
    pool_info.flags = 0;

    if (vkCreateCommandPool(instance.device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }

    render_data.command_pool = command_pool;
  }

  void CreateDepthResources() {
    VkFormat depth_format = FindDepthFormat();

    CreateImage(swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      depth_image, depth_image_memory);

    depth_image_view = CreateImageView(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

  }


  VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(instance.physical_device, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      }
      else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
        return format;
      }

      else {
        throw std::runtime_error("failed to find supported format!");
      }
    }
  }
  
  bool HasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

  VkFormat FindDepthFormat() {

    return FindSupportedFormat(
      { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
  }

  VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(instance.device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view");
    }
    return image_view;
  }


  void CreateTextureImage() {
    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = tex_width * tex_height * 4;

    std::cout << "image size is:" << image_size << std::endl;

    if (!pixels) {
      throw std::runtime_error("failed to load texture!");
    }

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

    void* data;
    vkMapMemory(instance.device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<uint32_t>(image_size));
    vkUnmapMemory(instance.device, staging_buffer_memory);

    stbi_image_free(pixels);

    CreateImage(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

    TransitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(staging_buffer, texture_image, static_cast<uint32_t>(tex_width),
      static_cast<uint32_t>(tex_height));

    TransitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(instance.device, staging_buffer, nullptr);
    vkFreeMemory(instance.device, staging_buffer_memory, nullptr);
  }


  void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
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
    alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(instance.device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(instance.device, image, image_memory, 0);
  }

  void CreateTextureImageView() {
    texture_image_view = CreateImageView(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  void CreateTextureSampler() {
    VkSamplerCreateInfo sampler_info{};

    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // How to interpolate texels that are magnified or minified
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    // what to do when go beyond the image dimensions
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    sampler_info.anisotropyEnable = VK_TRUE;

    // TODO move this somewhere better so physical device isn't in here
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(instance.physical_device, &properties);

    sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    if (vkCreateSampler(instance.device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler!");
    }
  }

  void LoadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
      throw std::runtime_error(warn + err);
    }

    for(const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
        };

        vertex.tex_coord = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
        };

        vertex.color = { 1.0f, 1.0f, 1.0f };
        vertices.push_back(vertex);
        indices.push_back(indices.size());
      }
    }
  }

  void CreateVertexBuffer() {
    vert_buffer = VertexBuffer(instance, render_data, vertices);
  }

  uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(instance.physical_device, &memory_properties);

    for (uint32_t ii = 0; ii < memory_properties.memoryTypeCount; ii++) {
      if ((type_filter & (1 << ii)) &&
        (memory_properties.memoryTypes[ii].propertyFlags & properties) == properties) {
        return ii;
      }
    }
    throw std::runtime_error("failed to find suitable memory type!");
  }

  void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(instance.device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to create buffer");
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(instance.device, buffer, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(instance.device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate memory");
    }

    vkBindBufferMemory(instance.device, buffer, buffer_memory, 0);
  }

  void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {

    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    EndSingleTimeCommands(command_buffer);

  }

  void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

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

    EndSingleTimeCommands(command_buffer);
  }

  void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

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

    EndSingleTimeCommands(command_buffer);
  }

  void CreateIndexBuffer() {
    ind_buffer = IndexBuffer(instance, render_data, indices);
  }

  void CreateUniformBuffers()
  {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    uniform_buffers.resize(swap_chain_images.size());
    uniform_buffers_memory.resize(swap_chain_images.size());

    for (size_t ii = 0; ii < swap_chain_images.size(); ii++) {
      CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers[ii], uniform_buffers_memory[ii]);
    }
  }

  void CreateDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(swap_chain_images.size());
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(swap_chain_images.size());

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = static_cast<uint32_t>(swap_chain_images.size());

    if (vkCreateDescriptorPool(instance.device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor pool");
    }

  }

  void CreateDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(swap_chain_images.size(), descriptor_set_layout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(swap_chain_images.size());
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(swap_chain_images.size());

    if (vkAllocateDescriptorSets(instance.device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor sets!");
    }

    for (size_t ii = 0; ii < swap_chain_images.size(); ii++) {
      VkDescriptorBufferInfo buffer_info{};
      buffer_info.buffer = uniform_buffers[ii];
      buffer_info.offset = 0;
      buffer_info.range = sizeof(UniformBufferObject);

      VkDescriptorImageInfo image_info{};
      image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      image_info.imageView = texture_image_view;
      image_info.sampler = texture_sampler;

      std::array<VkWriteDescriptorSet, 2> descriptor_writes{};

      descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_writes[0].dstSet = descriptor_sets[ii];
      descriptor_writes[0].dstBinding = 0;
      descriptor_writes[0].dstArrayElement = 0;
      descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptor_writes[0].descriptorCount = 1;
      descriptor_writes[0].pBufferInfo = &buffer_info;

      descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_writes[1].dstSet = descriptor_sets[ii];
      descriptor_writes[1].dstBinding = 1;
      descriptor_writes[1].dstArrayElement = 0;
      descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_writes[1].descriptorCount = 1;
      descriptor_writes[1].pImageInfo = &image_info;

      vkUpdateDescriptorSets(instance.device, static_cast<uint32_t>(descriptor_writes.size()),
        descriptor_writes.data(), 0, nullptr);
    }
  }

  void UpdateUniformBuffer(uint32_t current_image) {
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
      glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain_extent.width
      / (float)swap_chain_extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(instance.device, uniform_buffers_memory[current_image], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(instance.device, uniform_buffers_memory[current_image]);

  }

  void CreateCommandBuffers() {
    command_buffers.resize(swap_chain_framebuffers.size());

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t)command_buffers.size();

    if (vkAllocateCommandBuffers(instance.device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers");
    }

    for (size_t ii = 0; ii < command_buffers.size(); ii++) {
      VkCommandBufferBeginInfo begin_info{};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
      begin_info.pInheritanceInfo = nullptr;

      if (vkBeginCommandBuffer(command_buffers[ii], &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
      }

      std::array<VkClearValue, 2> clear_values{};
      clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
      // range of values in the depth buffer 
      clear_values[1].depthStencil = { 1.0f, 0 };

      VkRenderPassBeginInfo render_pass_info{};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_info.renderPass = render_pass;
      render_pass_info.framebuffer = swap_chain_framebuffers[ii];
      render_pass_info.renderArea.offset = { 0,0 };
      render_pass_info.renderArea.extent = swap_chain_extent;

      render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
      render_pass_info.pClearValues = clear_values.data();

      vkCmdBeginRenderPass(command_buffers[ii], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(command_buffers[ii], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

      VkBuffer vertex_buffers[] = { vert_buffer.GetBuffer()};

      VkDeviceSize offsets[] = { 0 };

      vkCmdBindVertexBuffers(command_buffers[ii], 0, 1, vertex_buffers, offsets);

      vkCmdBindIndexBuffer(command_buffers[ii], ind_buffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

      vkCmdBindDescriptorSets(command_buffers[ii], VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, 0, 1, &descriptor_sets[ii], 0, nullptr);

      vkCmdDrawIndexed(command_buffers[ii], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

      vkCmdEndRenderPass(command_buffers[ii]);

      if (vkEndCommandBuffer(command_buffers[ii]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
      }
    }
  }

  VkCommandBuffer BeginSingleTimeCommands() {
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

  void EndSingleTimeCommands(VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(instance.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(instance.graphics_queue);

    vkFreeCommandBuffers(instance.device, command_pool, 1, &command_buffer);
  }

  VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = sizeof(uint32_t) * code.size();
    create_info.pCode = code.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(instance.device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
      throw std::runtime_error("failed to create shader module!");
    }

    return shader_module;
  }


  void SetupDebugMessenger() {
    if (!enable_validation_layers) return;

    // Data structure to give information about the debug call back
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    populateDebugMessengerCreateInfo(create_info);

    if (CreateDebugUtilsMessengerEXT(instance.instance, &create_info, nullptr, &instance.debug_messenger)
      != VK_SUCCESS) {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }

  void CreateInstance() {
    if (enable_validation_layers && !CheckValidationLayerSupport()) {
      throw std::runtime_error("validation layers requested, but not available");
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    auto extensions = GetRequiredExtensions();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (enable_validation_layers) {
      create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
      create_info.ppEnabledLayerNames = validation_layers.data();

      populateDebugMessengerCreateInfo(debug_create_info);
      create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    }
    else {
      create_info.enabledLayerCount = 0;
      create_info.pNext = nullptr;
    }


    if (vkCreateInstance(&create_info, nullptr, &instance.instance) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create instance!");
    }

  }

  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;
    create_info.pUserData = nullptr;
  }

  // debug callback function
  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }

  bool CheckValidationLayerSupport() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> availiable_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, availiable_layers.data());

    for (const char* layer_name : validation_layers) {
      bool layer_found = false;

      for (const auto& layer_properties : availiable_layers) {
        if (strcmp(layer_name, layer_properties.layerName) == 0) {
          layer_found = true;
          break;
        }
      }

      if (!layer_found) {
        return false;
      }

    }

    return true;
  }

  // will return the list of required extensions based on whether
// validation layers are enabled or not
  std::vector<const char* > GetRequiredExtensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }

  void CreateSurface() {
    if (glfwCreateWindowSurface(instance.instance, instance.window, nullptr, &instance.surface)) {
      throw std::runtime_error("failed to create window surface");
    }
  }

  void CreateSyncObjects() {
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight.resize(swap_chain_images.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t ii = 0; ii < MAX_FRAMES_IN_FLIGHT; ii++) {
      if (vkCreateSemaphore(instance.device, &semaphore_info, nullptr, &image_available_semaphores[ii]) != VK_SUCCESS ||
        vkCreateSemaphore(instance.device, &semaphore_info, nullptr, &render_finished_semaphores[ii]) != VK_SUCCESS ||
        vkCreateFence(instance.device, &fence_info, nullptr, &in_flight_fences[ii]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create sync objects for a frame");
      }
    }

  }

  //
  void CleanupSwapChain() {

    vkDestroyImageView(instance.device, depth_image_view, nullptr);
    vkDestroyImage(instance.device, depth_image, nullptr);
    vkFreeMemory(instance.device, depth_image_memory, nullptr);

    for (auto framebuffer : swap_chain_framebuffers) {
      vkDestroyFramebuffer(instance.device, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(instance.device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
    vkDestroyPipeline(instance.device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(instance.device, pipeline_layout, nullptr);
    vkDestroyRenderPass(instance.device, render_pass, nullptr);

    for (auto image_view : swap_chain_image_views) {
      vkDestroyImageView(instance.device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(instance.device, swap_chain, nullptr);

    for (size_t ii = 0; ii < swap_chain_images.size(); ii++) {
      vkDestroyBuffer(instance.device, uniform_buffers[ii], nullptr);
      vkFreeMemory(instance.device, uniform_buffers_memory[ii], nullptr);
    }

    vkDestroyDescriptorPool(instance.device, descriptor_pool, nullptr);
  }
  // we need to recreate the swap chain for when the window surface is no
  // longer compatible with the swap chain (window resizing)
  void RecreateSwapChain() {
    // don't want to touch resources that are still in use
    int width = 0, height = 0;
    glfwGetFramebufferSize(instance.window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(instance.window, &width, &height);
      glfwWaitEvents();
    }
    vkDeviceWaitIdle(instance.device);
    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateDepthResources();
    CreateFrameBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();

    images_in_flight.resize(swap_chain_images.size(), VK_NULL_HANDLE);
  }

  // acquire an image from the swap chain
  // execute the command buffer with that image as an attachment
  // in the frame buffer
  // return the image to the swap chain for presentation

  void DrawFrame() {
    vkWaitForFences(instance.device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(instance.device, swap_chain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      RecreateSwapChain();
      return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire a swap chain image");
    }

    UpdateUniformBuffer(image_index);
    // check if a previous frame is using this image
    if (images_in_flight[image_index] != VK_NULL_HANDLE) {
      vkWaitForFences(instance.device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }

    // mark this image as now being in use by this frame
    images_in_flight[image_index] = in_flight_fences[current_frame];


    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { image_available_semaphores[current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[image_index];

    VkSemaphore signal_semaphores[] = { render_finished_semaphores[current_frame] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(instance.device, 1, &in_flight_fences[current_frame]);

    if (vkQueueSubmit(instance.graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = { swap_chain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(presentation_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frame_buffer_resized) {
      frame_buffer_resized = false;
      RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  // ATTRIBUTES 

  VkQueue graphics_queue;
  VkQueue presentation_queue;

  VkSwapchainKHR swap_chain;
  std::vector<VkImage> swap_chain_images;
  std::vector<VkImageView> swap_chain_image_views;
  VkFormat swap_chain_image_format;
  VkExtent2D swap_chain_extent;

  VkRenderPass render_pass;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  std::vector<VkFramebuffer> swap_chain_framebuffers;

  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffers;

  std::vector<VkSemaphore> image_available_semaphores;
  std::vector<VkSemaphore> render_finished_semaphores;
  std::vector<VkFence> in_flight_fences;
  std::vector<VkFence> images_in_flight;
  size_t current_frame = 0;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VertexBuffer vert_buffer;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;
  IndexBuffer ind_buffer;

  std::vector<VkBuffer> uniform_buffers;
  std::vector<VkDeviceMemory> uniform_buffers_memory;

  VkDescriptorPool descriptor_pool;
  std::vector<VkDescriptorSet> descriptor_sets;


  VkImage texture_image;
  VkDeviceMemory texture_image_memory;

  VkImageView texture_image_view;
  VkSampler texture_sampler;

  VkImage depth_image;
  VkDeviceMemory depth_image_memory;
  VkImageView depth_image_view;

  bool frame_buffer_resized = false;

  InitData instance;
  RenderData render_data;
};
