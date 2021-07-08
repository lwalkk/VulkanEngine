#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "vulkan_headers.h"
#include <string>
#include <vector>

/* STEPS FOR LOADING A MODEL:
 * 1. Get Library set up. I'm using assimp.
 * 2. Find a model / texture
 * 3. Load vertices and indices.
 */

/* Things I need to do:
 * Implement mesh and texture classes
 */




class Model {
public:
  Model(const char* path);
private:

  std::vector<Mesh> meshes;
  std::string directory;
};
