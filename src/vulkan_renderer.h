
//#include "vulkan/vulkan.h"
#include <volk.c>


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define VK_CHECK(command)\
{\
  VkResult result = command;\
  assert(result == VK_SUCCESS);\
}

struct
stDeletionQueue
{
  std::deque<std::function<void()>> Deletors;

  void PushFunction(std::function<void()>&& function)
  {
    Deletors.push_back(function);
  }

  void Flush()
  {
    for (auto it = Deletors.rbegin(); it != Deletors.rend(); it++)
    {
      (*it)();
    }

    Deletors.clear();
  }
};

struct
stVertexInputDescription
{
  std::vector<VkVertexInputBindingDescription> Bindings;
  std::vector<VkVertexInputAttributeDescription> Attributes;

  VkPipelineVertexInputStateCreateFlags Flags = 0;
};

struct
stVertex
{
	alignas(16) glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	alignas(16) glm::vec3 Normal = { 0.0f, 0.0f, 0.0f };
	alignas(8)  glm::vec2 TexCoord = { 0.0f, 0.0f };

  static VkVertexInputBindingDescription
  GetBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(stVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3>
  GetAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(stVertex, Position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(stVertex, Normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(stVertex, TexCoord);

    return attributeDescriptions;
  }
};

enum
enQueueType : int
{
  QUEUE_TYPE_GRAPHICS = 0,
  QUEUE_TYPE_PRESENT = 0,
  QUEUE_TYPE_COMPUTE = 1
};

struct
stQueue
{
  enQueueType Type = QUEUE_TYPE_GRAPHICS;
  VkQueue Queue = VK_NULL_HANDLE;
  uint32_t Index = 0;
};

struct
stDevice
{
  VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
  VkDevice LogicalDevice = VK_NULL_HANDLE;
  stQueue Queues[2] = {};
};

struct
stPipeline
{
  VkPipeline Pipeline = VK_NULL_HANDLE;
  VkPipelineLayout Layout = VK_NULL_HANDLE;
  VkPipelineLayout MeshLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout DescriptorSet = VK_NULL_HANDLE;
};

struct
stUniformBufferObject
{
  alignas(16) glm::mat4 Model;
  alignas(16) glm::mat4 View;
  alignas(16) glm::mat4 Proj;
};

struct
stMeshPushConstants
{
  glm::vec4 Data;
  glm::mat4 Model;
};

struct
stBuffer
{
  VkBuffer Buffer = VK_NULL_HANDLE;
  VkDeviceMemory Memory = VK_NULL_HANDLE;
};

struct
stImage
{
  VkImage Src = VK_NULL_HANDLE;
  VkImageView View = VK_NULL_HANDLE;
  VkDeviceMemory Memory = VK_NULL_HANDLE;
};

struct
stTexture
{
  stImage Image = {};
  VkSampler Sampler = VK_NULL_HANDLE;
  uint32_t MipLevels;
};

struct
stMesh
{
  std::vector<stVertex> Vertices;
  std::vector<uint32_t> Indices;
};

VkSurfaceKHR
CreateSurface(
  VkInstance instance,
  const stWindow& window,
  stDeletionQueue* deletionQueue
);

VkExtent2D
ChooseSwapExtent(
  const VkSurfaceCapabilitiesKHR& capabilities
);

#include "vulkan_initializers.h"
#include "vulkan_shaders.h"

#define MAX_PHYSICAL_DEVICE_COUNT 16
#define MAX_SWAPCHAIN_IMAGE_COUNT 16

struct
stRenderer
{
  void
  Init(const stWindow& window);

  void
  CreateSwapchain();

  void
  RecreateSwapchain();

  void
  Term();

  void
  Render(stCamera& camera, double delta = 0.0f);

  VkInstance Instance = VK_NULL_HANDLE;
  VkSurfaceKHR Surface = VK_NULL_HANDLE;

  VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

  uint32_t CurrentFrame = 0;

  VkFormat SwapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D SwapchainExtent = { 0, 0 };

  VkCommandPool CommandPool = VK_NULL_HANDLE;
  VkCommandBuffer CommandBuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

  VkRenderPass ForwardRenderPass = VK_NULL_HANDLE;
  stPipeline GraphicsPipeline = {};

  stBuffer VertexBuffer = {};

  stBuffer IndexBuffer = {};

  stBuffer UniformBuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

  VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet DescriptorSets[MAX_SWAPCHAIN_IMAGE_COUNT];

  stTexture TexImage = {};

  VkSampleCountFlagBits SamplesFlag = VK_SAMPLE_COUNT_1_BIT;

  stImage SwapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT];
  stImage DepthImage = {};
  stImage ColorImage = {};
  VkFramebuffer Framebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];
  uint32_t SwapchainImageCount = ArrayCount(SwapchainImages);

  VkSemaphore AcquireSemaphores[MAX_SWAPCHAIN_IMAGE_COUNT];
  VkSemaphore ReleaseSemaphores[MAX_SWAPCHAIN_IMAGE_COUNT];
  VkFence InFlightFence[MAX_SWAPCHAIN_IMAGE_COUNT];
  VkFence ImagesInFlight[MAX_SWAPCHAIN_IMAGE_COUNT];

  VkPhysicalDevice PhysicalDevices[MAX_PHYSICAL_DEVICE_COUNT];
  uint32_t PhysicalDeviceCount = ArrayCount(PhysicalDevices);

  stDevice Device;

  stDeletionQueue Deletion;
  stDeletionQueue SwapchainDeletion;
};

void
stRenderer::Init(
  const stWindow& window)
{
  VK_CHECK(volkInitialize());

  Instance = init::create_instance(
    VK_API_VERSION_1_1,
    "default_app",
    VK_MAKE_VERSION(0, 0, 1),
    "default_engine",
    VK_MAKE_VERSION(0, 0, 1),
    &Deletion
  );

  volkLoadInstance(Instance);

  Surface = CreateSurface(Instance, window, &Deletion);

  VK_CHECK(vkEnumeratePhysicalDevices(
    Instance,
    &PhysicalDeviceCount,
    PhysicalDevices)
  );
  assert(PhysicalDeviceCount > 0);

  Device.PhysicalDevice = init::pick_device(PhysicalDevices, PhysicalDeviceCount);
  assert(Device.PhysicalDevice != VK_NULL_HANDLE);

  SamplesFlag = init::get_max_usable_sample_count(Device);

  Device = init::create_device(Device.PhysicalDevice, Surface, &Deletion);

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    AcquireSemaphores[i] = init::create_semaphore(Device, &Deletion);
    ReleaseSemaphores[i] = init::create_semaphore(Device, &Deletion);
    InFlightFence[i] = init::create_fence(Device, &Deletion);
  }

  CommandPool = init::create_command_pool(Device, 0, &Deletion);

  TexImage = init::create_texture(Device, CommandPool, "./data/models/cube/default.png", &Deletion);

  CreateSwapchain();
}

void
stRenderer::CreateSwapchain()
{
  Swapchain = init::create_swapchain(
    Device,
    Surface,
    SwapchainImageFormat,
    SwapchainExtent,
    SwapchainImages,
    SwapchainImageCount,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    VK_PRESENT_MODE_MAILBOX_KHR,
    &SwapchainDeletion
  );

  ForwardRenderPass = init::create_render_pass(Device, SwapchainImageFormat, SamplesFlag, &SwapchainDeletion);

  ColorImage = init::create_color_resources(Device, SwapchainExtent, SwapchainImageFormat, SamplesFlag, CommandPool, &SwapchainDeletion);

  DepthImage = init::create_depth_resources(Device, SamplesFlag, SwapchainExtent, CommandPool, &SwapchainDeletion);

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    Framebuffers[i] = init::create_framebuffer(Device, ForwardRenderPass, SwapchainExtent, SwapchainImages[i].View, DepthImage.View, ColorImage.View, &SwapchainDeletion);
  }

  GraphicsPipeline = init::create_pipeline(Device, SwapchainExtent, ForwardRenderPass, SamplesFlag, &SwapchainDeletion);

  stMesh mesh;

  // init::load_mesh(mesh, "./data/models/pirate/pirate.obj");
  init::load_mesh(mesh, "./data/models/cube/cube.obj");
  // init::load_mesh(mesh, "./data/models/the_nobel/the_nobel.obj");
  // init::load_mesh(mesh, "./data/models/bunny/bunny.obj");

  { // CREATE VERTEX BUFFER
    VkDeviceSize bufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();

    stBuffer stagingBuffer = {};
    init::create_buffer(
      Device,
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, nullptr
    );

    void* data;
    vkMapMemory(Device.LogicalDevice, stagingBuffer.Memory, 0, bufferSize, 0, &data);
      memcpy(data, mesh.Vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(Device.LogicalDevice, stagingBuffer.Memory);

    init::create_buffer(
      Device,
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer,
      &SwapchainDeletion
    );

    init::copy_buffer(Device, stagingBuffer.Buffer, VertexBuffer.Buffer, bufferSize, CommandPool);

    vkDestroyBuffer(Device.LogicalDevice, stagingBuffer.Buffer, nullptr);
    vkFreeMemory(Device.LogicalDevice, stagingBuffer.Memory, nullptr);
  }

  { // CREATE INDEX BUFFER
    VkDeviceSize bufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();

    stBuffer stagingBuffer = {};
    init::create_buffer(
      Device,
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, nullptr
    );

    void* data;
    vkMapMemory(Device.LogicalDevice, stagingBuffer.Memory, 0, bufferSize, 0, &data);
      memcpy(data, mesh.Indices.data(), (size_t) bufferSize);
    vkUnmapMemory(Device.LogicalDevice, stagingBuffer.Memory);

    init::create_buffer(
      Device,
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, IndexBuffer, &SwapchainDeletion
    );

    init::copy_buffer(Device, stagingBuffer.Buffer, IndexBuffer.Buffer, bufferSize, CommandPool);

    vkDestroyBuffer(Device.LogicalDevice, stagingBuffer.Buffer, nullptr);
    vkFreeMemory(Device.LogicalDevice, stagingBuffer.Memory, nullptr);
  }

  //init::create_uniform_buffer(Device, UniformBuffers, SwapchainImageCount);

  VkDeviceSize bufferSize = sizeof(stUniformBufferObject);

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    init::create_buffer(
      Device,
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      UniformBuffers[i], &SwapchainDeletion
    );
  }

  std::array<VkDescriptorPoolSize, 2> poolSizes = {};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = SwapchainImageCount;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = SwapchainImageCount;

  DescriptorPool = init::create_descriptor_pools(Device, poolSizes.data(), (uint32_t)poolSizes.size(), SwapchainImageCount, &SwapchainDeletion);

  init::create_descriptor_sets(Device, GraphicsPipeline, DescriptorPool, DescriptorSets, UniformBuffers, SwapchainImageCount, TexImage);

  init::create_command_buffers(Device, CommandPool,
    {
      [=](VkCommandBuffer cb, uint32_t index)
      {
        VkClearValue clearValues[2] =
        {
          {{ 0.3f, 0.3f, 0.3f, 1.0f }},
          { 1.0f, 0 }
        };

        VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassBeginInfo.renderPass = ForwardRenderPass;
        renderPassBeginInfo.framebuffer = Framebuffers[index];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = SwapchainExtent;
        renderPassBeginInfo.clearValueCount = ArrayCount(clearValues);
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(cb, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // draw call
        {
          vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline.Pipeline);

          VkBuffer vertexBuffers[] = { VertexBuffer.Buffer };
          VkDeviceSize offsets[] = { 0 };
          vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
          vkCmdBindIndexBuffer(cb, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

          vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline.Layout, 0, 1, &DescriptorSets[index], 0, nullptr);
          vkCmdDrawIndexed(cb, (uint32_t) mesh.Indices.size(), 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(cb);
      }
    }, CommandBuffers, SwapchainImageCount, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 0, &SwapchainDeletion
  );
}

void
stRenderer::RecreateSwapchain()
{
  if (Swapchain == VK_NULL_HANDLE)
  {
    return;
  }

  VK_CHECK(vkDeviceWaitIdle(Device.LogicalDevice));
  SwapchainDeletion.Flush();

  CreateSwapchain();
}

void
stRenderer::Term()
{
  VK_CHECK(vkDeviceWaitIdle(Device.LogicalDevice));
  SwapchainDeletion.Flush();
  Deletion.Flush();
}

void
stRenderer::Render(stCamera& camera, double delta)
{
  vkWaitForFences(Device.LogicalDevice, 1, &InFlightFence[CurrentFrame], VK_TRUE, ~0ull);

  uint32_t imageIndex = 0;
  VK_CHECK(vkAcquireNextImageKHR(Device.LogicalDevice, Swapchain, ~0ull, AcquireSemaphores[CurrentFrame], VK_NULL_HANDLE, &imageIndex));

  if (ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
  {
    vkWaitForFences(Device.LogicalDevice, 1, &ImagesInFlight[imageIndex], VK_TRUE, ~0ull);
  }
  ImagesInFlight[imageIndex] = InFlightFence[CurrentFrame];

  vkResetFences(Device.LogicalDevice, 1, &InFlightFence[CurrentFrame]);

  VkPipelineStageFlags submitStageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

  //

  static float time = 0.0f;
  time += (float) delta * 0.1f;

  stUniformBufferObject ubo = {};
  ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  // ubo.Model *= glm::scale( ubo.Model, glm::vec3(0.5f) );
  ubo.View = camera.get_view_matrix();
  ubo.Proj = camera.get_projection_matrix({SwapchainExtent.width, SwapchainExtent.height});

  void* data;
  vkMapMemory(Device.LogicalDevice, UniformBuffers[imageIndex].Memory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(Device.LogicalDevice, UniformBuffers[imageIndex].Memory);

  //

  VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &AcquireSemaphores[CurrentFrame];
  submitInfo.pWaitDstStageMask = submitStageFlags;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &CommandBuffers[imageIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &ReleaseSemaphores[CurrentFrame];

  VK_CHECK(vkQueueSubmit(Device.Queues[QUEUE_TYPE_GRAPHICS].Queue, 1, &submitInfo, InFlightFence[CurrentFrame]));

  VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &ReleaseSemaphores[CurrentFrame];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &Swapchain;
  presentInfo.pImageIndices = &imageIndex;

  VK_CHECK(vkQueuePresentKHR(Device.Queues[QUEUE_TYPE_GRAPHICS].Queue, &presentInfo));

  CurrentFrame = (CurrentFrame + 1) % SwapchainImageCount;
}