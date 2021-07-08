#pragma once
#include "vulkan_headers.h"
#include "buffer.h"
#include <vector>

class IndexBuffer: public Buffer {
public:
  IndexBuffer(const InitData& init, const RenderData& render, const std::vector<Vertex>& vertices);
  void Bind() override;
};
