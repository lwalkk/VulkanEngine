#pragma once
#include "vulkan_headers.h"
#include "buffer.h"
#include <vector>

class IndexBuffer: public Buffer {
public:
  IndexBuffer();
  IndexBuffer(const InitData& init, const RenderData& render, const std::vector<uint32_t>& vertices);
  void Bind() override;
};
