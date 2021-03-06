#include "index_buffer.h"

IndexBuffer::IndexBuffer() : Buffer()
{
}

IndexBuffer::IndexBuffer(const InitData& init, const RenderData& render, const std::vector<uint32_t>& vertices)
  : Buffer(init, render, sizeof(vertices.at(0))* vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    (void*)vertices.data()) {
}

void IndexBuffer::Bind() {
  return;
}
