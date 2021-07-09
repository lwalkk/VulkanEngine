#pragma once
#include "vulkan_headers.h"
#include "buffer.h"
#include <vector>

class VertexBuffer : public Buffer {
public:
  VertexBuffer();
  VertexBuffer(const InitData& init, const RenderData& render, const std::vector<Vertex>& vertices);
  void Bind() override;
};
