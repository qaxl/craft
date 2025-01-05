#include "mesh.hpp"

#include "renderer.hpp"

namespace craft::vk {
MeshBuffers UploadMesh(Renderer *renderer, VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                       std::span<Vtx> vertices) {
  size_t vertex_size = vertices.size_bytes();
  size_t index_size = indices.size_bytes();

  MeshBuffers mesh;
  mesh.vertex = AllocateBuffer(allocator, vertex_size,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo addr_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  addr_info.buffer = mesh.vertex.buffer;

  mesh.vertex_addr = vkGetBufferDeviceAddress(device, &addr_info);
  mesh.index =
      AllocateBuffer(allocator, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging =
      AllocateBuffer(allocator, vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void *data;
  VK_CHECK(vmaMapMemory(allocator, staging.allocation, &data));

  memcpy(data, vertices.data(), vertex_size);
  memcpy(static_cast<char *>(data) + vertex_size, indices.data(), index_size);

  vmaUnmapMemory(allocator, staging.allocation);

  // TODO: fucking optimize
  renderer->SubmitNow([&](VkCommandBuffer cmd) {
    VkBufferCopy vertex_copy{};
    vertex_copy.size = vertex_size;

    vkCmdCopyBuffer(cmd, staging.buffer, mesh.vertex.buffer, 1, &vertex_copy);

    vertex_copy.srcOffset = vertex_size;
    vertex_copy.size = index_size;

    vkCmdCopyBuffer(cmd, staging.buffer, mesh.index.buffer, 1, &vertex_copy);
  });

  DestroyBuffer(allocator, std::move(staging));
  return mesh;
}
} // namespace craft::vk
