#include "VulkanEngine.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main() {
  VulkanEngine app;

  try {
    app.run();
  }
  catch (const std::exception& e) {
    std::cerr << "EXCEPTION" << std::endl;
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}


