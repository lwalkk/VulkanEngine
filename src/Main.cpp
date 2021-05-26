#include "VulkanEngine.h"

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


