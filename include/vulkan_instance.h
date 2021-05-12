#pragma once

#include "headers.h"

class VulkanInstance {
public:

  VulkanInstance();

  const VkInstance& get() { return instance; }


private:

  bool CheckValidationLayerSupport();
  VkInstance instance;
};
