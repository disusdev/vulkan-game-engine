
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

template<typename T>
VkVertexInputBindingDescription
GetBindingDescription()
{
  VkVertexInputBindingDescription bindingDescription = {};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(T);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3>
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
stMaterial
{
	VkPipeline Pipeline;
	VkPipelineLayout PipelineLayout;
};

namespace material
{

#define MAX_MATERIAL_COUNT 10
stMaterial Materials[MAX_MATERIAL_COUNT];
uint32_t MaterialsCount = 0;

std::unordered_map<std::string, stMaterial*> CachedMaterials;

stMaterial* create_material(
  VkPipeline pipeline,
  VkPipelineLayout layout,
  const char* name)
{
  stMaterial mat;
  mat.Pipeline = pipeline;
  mat.PipelineLayout = layout;
  Materials[MaterialsCount] = mat;
  CachedMaterials.insert( { name, &Materials[MaterialsCount] } );
  return &Materials[MaterialsCount++];
}

stMaterial* get_material(
  const char* name)
{
  return CachedMaterials[name];
}

}

struct
stRenderObject
{
  stMesh* Mesh;
  stMaterial* Material;
  glm::mat4* Transform;
};

struct
stMeshPushConstants
{
  alignas(16) glm::mat4 Model;
  alignas(16) glm::mat4 View;
  alignas(16) glm::mat4 Proj;
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
#define MAX_RENDER_OBJECT_COUNT 128

struct
stRenderer
{
  void
  Init(const stWindow& window);

  void
  AddRenderingObjectsFromEntities(
    stEntity** entities,
    uint32_t entitiesCount,
    const char* materialName = "default")
  {
    RenderObjectCount = entitiesCount;
    for (size_t i = 0; i < entitiesCount; i++)
    {
      RenderObjects[i] = { entities[i]->Mesh, material::get_material(materialName), entities[i]->Transform.Tramsform };
    }

    // TODO: cached and loaded from every entity
    //{ // load meshes to GPU
    //  stMesh& mesh = *entities[0]->Mesh;

    //  { // CREATE VERTEX BUFFER
    //    VertexBuffer = init::create_vertex_buffer(Device, CommandPool, mesh, &SwapchainDeletion);
    //  }

    //  { // CREATE INDEX BUFFER
    //    IndexBuffer = init::create_index_buffer(Device, CommandPool, mesh, &SwapchainDeletion);
    //  }
    //}

    {
      for (size_t i = 0; i < mesh::MesheCounter; i++)
      {
        stMesh& mesh = mesh::Meshes[i];

        { // CREATE VERTEX BUFFER
          VertexBuffer = init::create_vertex_buffer(Device, CommandPool, mesh, &SwapchainDeletion);
        }

        { // CREATE INDEX BUFFER
          IndexBuffer = init::create_index_buffer(Device, CommandPool, mesh, &SwapchainDeletion);
        }
      }
    }
  }

  void
  CreateSwapchain();

  void
  RecreateSwapchain();

  void
  Term();

  void
  Render(double delta = 0.0f);

  void
  draw_objects(
    VkCommandBuffer cmd,
    VkDescriptorSet descriptorSet,
    stRenderObject* first,
    uint32_t count)
  {
    for (size_t i = 0; i < count; i++)
    {
      stRenderObject& object = first[i];

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.Material->Pipeline);
      
      VkBuffer vertexBuffers[] = { VertexBuffer.Buffer };
      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(cmd, IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
      
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.Material->PipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
      
      stMeshPushConstants constants = {};
      constants.Model = *object.Transform;
      constants.View = Camera->get_view_matrix();
      constants.Proj = Camera->get_projection_matrix({SwapchainExtent.width, SwapchainExtent.height});
      
      vkCmdPushConstants(cmd, object.Material->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(stMeshPushConstants), &constants);

      vkCmdDrawIndexed(cmd, (uint32_t) object.Mesh->Indices.size(), 1, 0, 0, 0);
    }
  }

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

  stCamera* Camera;

  uint32_t RenderObjectCount = 0;
  stRenderObject RenderObjects[MAX_RENDER_OBJECT_COUNT];
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

  CommandPool = init::create_command_pool(Device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &Deletion);

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

  stMaterial* mat = material::create_material(GraphicsPipeline.Pipeline, GraphicsPipeline.Layout, "default");

  std::array<VkDescriptorPoolSize, 1> poolSizes = {};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[0].descriptorCount = SwapchainImageCount;

  DescriptorPool = init::create_descriptor_pools(Device, poolSizes.data(), (uint32_t)poolSizes.size(), SwapchainImageCount, &SwapchainDeletion);

  init::create_descriptor_sets(Device, GraphicsPipeline, DescriptorPool, DescriptorSets, UniformBuffers, SwapchainImageCount, TexImage);

  init::create_command_buffers(Device, CommandPool, CommandBuffers, SwapchainImageCount, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &SwapchainDeletion);
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

void IRender()
{
  // stDeviceContext* ctx = device->createCommandBuffer();

  // stPassClear passClear;
  // passClear.Mask = Framebuffer::Mask_Color0;

  // ctx->BeginPass(fb, 0, Framebuffer::Mask_Color0, &passClear);

  // ctx->BindProgram(Program.Get());

  // ctx->BindBuffer(0, GlobalDataBuffer.Get());
  // ctx->BindBufferData(1, &Params, sizeof(Params));
  // ctx->BindTexture(0, LightMap, SamplerState::Filter_Linear);

  // ctx->Draw(Geometry, Geometry::Primitive_Triangles, 0, Count);

  // ctx->EndPass();

  // device->CommitCommandBuffer(ctx);
}

void
stRenderer::Render(double delta)
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

  VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(CommandBuffers[imageIndex], &beginInfo));

  VkClearValue clearValues[2] =
  {
    {{ 0.1f, 0.1f, 0.1f, 1.0f }},
    { 1.0f, 0 }
  };

  VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
  renderPassBeginInfo.renderPass = ForwardRenderPass;
  renderPassBeginInfo.framebuffer = Framebuffers[imageIndex];
  renderPassBeginInfo.renderArea.offset = { 0, 0 };
  renderPassBeginInfo.renderArea.extent = SwapchainExtent;
  renderPassBeginInfo.clearValueCount = ArrayCount(clearValues);
  renderPassBeginInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(CommandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  {
    draw_objects(CommandBuffers[imageIndex], DescriptorSets[imageIndex], RenderObjects, RenderObjectCount);
  }

  vkCmdEndRenderPass(CommandBuffers[imageIndex]);

  VK_CHECK(vkEndCommandBuffer(CommandBuffers[imageIndex]));
  
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