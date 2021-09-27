
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

std::array<VkVertexInputAttributeDescription, 4>
GetAttributeDescriptions()
{
  std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

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
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(stVertex, Color);

  attributeDescriptions[3].binding = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[3].offset = offsetof(stVertex, TexCoord);

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
stGfxPipeline
{
  VkPipeline Pipeline = VK_NULL_HANDLE;
  VkPipelineLayout Layout = VK_NULL_HANDLE;
  //VkPipelineLayout MeshLayout = VK_NULL_HANDLE;

  VkDescriptorSetLayout SamplerLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout ObjectLayout = VK_NULL_HANDLE;
};

#include "vulkan_pipeline.h"

struct
stMaterial
{
	VkPipeline Pipeline;
	VkPipelineLayout PipelineLayout;
};

namespace material
{

#define MAX_MATERIAL_COUNT 1024
stMaterial Materials[MAX_MATERIAL_COUNT];
uint32_t MaterialsCount = 0;

std::unordered_map<std::string, stMaterial*> CachedMaterials;

stMaterial* create_material(
  VkPipeline pipeline,
  VkPipelineLayout layout,
  const char* name)
{
  stMaterial* mat;
  if (CachedMaterials.find(name) != CachedMaterials.end())
  {
    mat = CachedMaterials[name];
  }
  else
  {
    mat = &Materials[MaterialsCount++];
    CachedMaterials[name] = mat;
  }
  mat->Pipeline = pipeline;
  mat->PipelineLayout = layout;
  return mat;
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
stIndirectBatch
{
  stMesh* Mesh;
  stMaterial* Material;
  glm::mat4* Transform;
  //
  uint32_t First;
  uint32_t Count;
};

struct
stPerObjectDataGPU
{
  alignas(16) glm::mat4 Model;
};

struct
stGlobalDataGPU
{
  alignas(16) glm::mat4 ViewProj;
  alignas(16) glm::vec3 DirectionalLight;
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
  uint32_t DescriptorSetIndex = 0;
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

struct
stRenderer
{
  void
  Init(const stWindow& window);

  void
  AddRenderingObjectsFromEntities(
    stScene& scene,
    const char* materialName = "default");


  void
  CreateSwapchain();

  void
  RecreateSwapchain();

  void
  Term();

  void
  Render(double delta = 0.0f);

  std::vector<stIndirectBatch>
  CompactDraws(
    stRenderObject* objects,
    uint32_t count)
  {
    std::vector<stIndirectBatch> draws;

    auto addToDraws =
      [&draws](stMesh* mesh,
               stMaterial* material)
    {
      stIndirectBatch firstDraw;
      firstDraw.Mesh = mesh;
      firstDraw.Material = material;
      firstDraw.First = 0;
      firstDraw.Count = 1;

      draws.push_back(firstDraw);
    };

    addToDraws(objects[0].Mesh, objects[0].Material);

    for (uint32_t i = 0; i < count; i++)
    {
      bool sameMesh = objects->Mesh == draws.back().Mesh;
      bool sameMaterial = objects->Material == draws.back().Material;

      if (sameMesh && sameMaterial)
      {
        draws.back().Count++;
      }
      else
      {
        addToDraws(objects[i].Mesh, objects[i].Material);
      }
    }

    return draws;
  }

  void
  DrawObjects(
    VkCommandBuffer cmd,
    uint32_t targetIndex,
    VkDescriptorSet descriptorSets[MAX_TEXTURE_COUNT][MAX_SWAPCHAIN_IMAGE_COUNT],
    uint32_t descriptorSetCount,
    stRenderObject* first,
    uint32_t count);

  VkInstance Instance = VK_NULL_HANDLE;
  VkSurfaceKHR Surface = VK_NULL_HANDLE;

  VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

  uint32_t CurrentFrame = 0;

  VkFormat SwapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D SwapchainExtent = { 0, 0 };

  VkCommandPool CommandPool = VK_NULL_HANDLE;
  VkCommandBuffer CommandBuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

  VkRenderPass ForwardRenderPass = VK_NULL_HANDLE;

  stGfxPipeline GraphicsPipeline = {};

  struct
  stRenderMeshData
  {
    stBuffer VertexBuffer = {};
    stBuffer IndexBuffer = {};
    stTexture TexImage = {};
  };

  stRenderMeshData RenderMeshes[MAX_MESH_COUNT];
  //stRenderMeshData *RenderMeshes = (stRenderMeshData*) calloc(MAX_MESH_COUNT, sizeof(stRenderMeshData));

  std::unordered_map<stMesh*, stRenderMeshData*> RenderMeshesCache;

  VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet TextureSets[MAX_TEXTURE_COUNT][MAX_SWAPCHAIN_IMAGE_COUNT]; // TODO: define image count

  stBuffer ObjectBuffers[MAX_SWAPCHAIN_IMAGE_COUNT];
  VkDescriptorSet ObjectDescriptors[MAX_SWAPCHAIN_IMAGE_COUNT];

  stTexture DefaultTexImage = {};

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
  stSun* Sun;

  uint64_t RenderObjectCount = 0;
  std::vector<stRenderObject> RenderObjects;
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

  RenderObjects.reserve(1000000);

  DefaultTexImage = init::create_texture(Device, CommandPool, "./data/models/cube/default.png", &Deletion);

  CreateSwapchain();
}

void
stRenderer::AddRenderingObjectsFromEntities(
  stScene& scene,
  const char* materialName /*= "default"*/)
{
  
  for (size_t i = 0; i < scene.Entities.size(); i++)
  {
    for (size_t j = 0; j < scene.Entities[i].Entity->MeshCount; j++)
    {
      *scene.Entities[i].Entity->Transform.Tramsform *= scene.Entities[i].Entity->Mesh[j]->RootMatrix;

      RenderObjects.push_back({ scene.Entities[i].Entity->Mesh[j], material::get_material(materialName), scene.Entities[i].Entity->Transform.Tramsform });
      // RenderObjects[RenderObjectCount] = { base_entities[i].Entity->Mesh[j], material::get_material(materialName), base_entities[i].Entity->Transform.Tramsform };

      RenderObjectCount += 1;
    }
  }

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    void* objectData;
    vkMapMemory(Device.LogicalDevice, ObjectBuffers[i].Memory, 0, sizeof(stPerObjectDataGPU) * MAX_OBJECTS_COUNT, 0, &objectData);
      stPerObjectDataGPU* objectSSBO = (stPerObjectDataGPU*)objectData;

      for (int i = 0; i < RenderObjectCount; i++)
      {
      	stRenderObject& object = RenderObjects[i];
      	objectSSBO[i].Model = *object.Transform;
      }
    vkUnmapMemory(Device.LogicalDevice, ObjectBuffers[i].Memory);
  }

  {
    for (size_t i = 0; i < mesh::MesheCounter; i++)
    {
      { // CREATE VERTEX BUFFER
        RenderMeshes[i].VertexBuffer = init::create_vertex_buffer(Device, CommandPool, mesh::Meshes[i], &Deletion);
      }

      { // CREATE INDEX BUFFER
        RenderMeshes[i].IndexBuffer = init::create_index_buffer(Device, CommandPool, mesh::Meshes[i], &Deletion);
      }

      std::string load_texture = mesh::Meshes[i].TexturePath.empty()
        ? "./data/models/cube/default.png"
        : mesh::Meshes[i].TexturePath;

      stTexture* texture = init::get_texture(load_texture);

      if (texture == nullptr)
      {
        RenderMeshes[i].TexImage = init::create_texture(Device, CommandPool, load_texture.c_str(), &Deletion);

        for (size_t i = 0; i < init::TextureCounter; i++)
        {
          for (size_t j = 0; j < SwapchainImageCount; j++)
          {
            init::update_descriptor_set(Device, TextureSets[i][j], init::Textures[i]);
          }
        }
      }
      else
        RenderMeshes[i].TexImage = *texture;

      RenderMeshesCache.insert({&mesh::Meshes[i], &RenderMeshes[i]});
    }
  }
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
    VkImageView attachments[3] = 
    {
      ColorImage.View,
      DepthImage.View,
      SwapchainImages[i].View
    };

    Framebuffers[i] = init::create_framebuffer(Device, ForwardRenderPass, SwapchainExtent, attachments, 3, &SwapchainDeletion);

    init::create_buffer(Device, MAX_OBJECTS_COUNT * sizeof(stPerObjectDataGPU), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ObjectBuffers[i], &SwapchainDeletion);
  }

  GraphicsPipeline = init::create_gfx_pipeline(Device, SwapchainExtent, ForwardRenderPass, SamplesFlag, &SwapchainDeletion);

  stMaterial* mat = material::create_material(GraphicsPipeline.Pipeline, GraphicsPipeline.Layout, "default");

  VkDescriptorPoolSize poolSizes[] =
  { 
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
  };

  DescriptorPool = init::create_descriptor_pools(Device, poolSizes, ArrayCount(poolSizes), (MAX_TEXTURE_COUNT + 1) * SwapchainImageCount * ArrayCount(poolSizes), &SwapchainDeletion);

  for (size_t tc = 0; tc < MAX_TEXTURE_COUNT; tc++)
  {
    VkDescriptorSetLayout layouts[] = { GraphicsPipeline.SamplerLayout, GraphicsPipeline.SamplerLayout, GraphicsPipeline.SamplerLayout };
    
    VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	  allocInfo.descriptorPool = DescriptorPool;
	  allocInfo.descriptorSetCount = ArrayCount(layouts);
	  allocInfo.pSetLayouts = layouts;

	  VK_CHECK(vkAllocateDescriptorSets(Device.LogicalDevice, &allocInfo, &TextureSets[tc][0]));

    for (size_t i = 0; i < SwapchainImageCount; i++)
    {
	    VkDescriptorImageInfo imageBufferInfo;
	    imageBufferInfo.sampler = DefaultTexImage.Sampler;
	    imageBufferInfo.imageView = DefaultTexImage.Image.View;
	    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	    VkWriteDescriptorSet texture1 = init::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TextureSets[tc][i], &imageBufferInfo, 0);

	    vkUpdateDescriptorSets(Device.LogicalDevice, 1, &texture1, 0, nullptr);
    }
  }

  {
    VkDescriptorSetLayout layouts[] = { GraphicsPipeline.ObjectLayout, GraphicsPipeline.ObjectLayout, GraphicsPipeline.ObjectLayout};

    VkDescriptorSetAllocateInfo objectSetAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		objectSetAlloc.descriptorPool = DescriptorPool;
		objectSetAlloc.descriptorSetCount = ArrayCount(layouts);
		objectSetAlloc.pSetLayouts = layouts;

		VK_CHECK(vkAllocateDescriptorSets(Device.LogicalDevice, &objectSetAlloc, &ObjectDescriptors[0]));

    for (size_t i = 0; i < SwapchainImageCount; i++)
    {
		  VkDescriptorBufferInfo objectBufferInfo;
		  objectBufferInfo.buffer = ObjectBuffers[i].Buffer;
		  objectBufferInfo.offset = 0;
		  objectBufferInfo.range = sizeof(stPerObjectDataGPU) * MAX_OBJECTS_COUNT;

		  VkWriteDescriptorSet objectWrite = init::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, ObjectDescriptors[i], &objectBufferInfo, 0);

		  vkUpdateDescriptorSets(Device.LogicalDevice, 1, &objectWrite, 0, nullptr);
    }
  }

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

  for (size_t i = 0; i < init::TextureCounter; i++)
  {
    for (size_t j = 0; j < SwapchainImageCount; j++)
    {
      init::update_descriptor_set(Device, TextureSets[i][j], init::Textures[i]);
    }
  }
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

  // update descriptor sets if you wish, may be resonable gona be
  // to make more descriptor sets ?
  // or to make uniform ?

  // VkDescriptorSet* descriptorSets = DescriptorSets[i];

  //for (size_t i = 0; i < init::TextureCounter; i++)
  //{
    // stRenderMeshData* renderData = RenderMeshesCache[RenderObjects[i].Mesh];
    // init::update_descriptor_set(Device, DescriptorSets1[i][imageIndex], init::Textures[i]);
  //}
  uint32_t descriptorsCount = init::TextureCounter;

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
    DrawObjects(CommandBuffers[imageIndex], imageIndex, TextureSets, descriptorsCount, RenderObjects.data(), RenderObjectCount);
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

void
stRenderer::DrawObjects(
  VkCommandBuffer cmd,
  uint32_t targetIndex,
  VkDescriptorSet descriptorSets[MAX_TEXTURE_COUNT][MAX_SWAPCHAIN_IMAGE_COUNT],
  uint32_t descriptorSetCount,
  stRenderObject* first,
  uint32_t count)
{
  if (count == 0) return;
  
  auto bindDescriptors =
  [cmd, descriptorSets, targetIndex](
    stRenderObject& draw,
    VkDescriptorSet* objectDescriptors,
    stRenderMeshData* renderData)
  {
    //* (*draw.Transform);

    vkCmdBindDescriptorSets(
      cmd,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      draw.Material->PipelineLayout,
      0,
      1,
      &descriptorSets[renderData->TexImage.DescriptorSetIndex][targetIndex],
      0,
      nullptr
    );

    vkCmdBindDescriptorSets(
      cmd,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      draw.Material->PipelineLayout,
      1,
      1,
      &objectDescriptors[targetIndex],
      0,
      nullptr);
  };

  auto bindMeshes = [cmd](
    stRenderMeshData* renderData)
  {
    VkBuffer vertexBuffers[] = { renderData->VertexBuffer.Buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, renderData->IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
  };

  auto pushConstants = [=](
    stRenderObject& draw)
  {
    stGlobalDataGPU constants = {};
    constants.ViewProj =
      Camera->get_projection_matrix({ SwapchainExtent.width, SwapchainExtent.height }) 
      * Camera->get_view_matrix();
    constants.DirectionalLight = Sun->LightDirection;
    
    vkCmdPushConstants(cmd, draw.Material->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(stGlobalDataGPU), &constants);
  };

  //std::vector<stIndirectBatch> draws = CompactDraws(first, count);

  //for (stIndirectBatch& draw : draws)
  //{
  //  stRenderMeshData* renderData = RenderMeshesCache[draw.Mesh];

  //  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.Material->Pipeline);

  //  bindDescriptors(draw, renderData);

  //  bindMeshes(renderData);

  //  pushConstants(draw);

  //  for(int i = draw.First ;i < draw.Count;i++)
  //  {
  //    vkCmdDrawIndexed(cmd, (uint32_t) draw.Mesh->Indices.size(), 1, 0, 0, 0);
  //  }
  //}


  for (size_t i = 0; i < count; i++)
  {
    stRenderObject& object = first[i];
    stRenderMeshData* renderData = RenderMeshesCache[object.Mesh];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.Material->Pipeline);
    
    bindDescriptors(object, ObjectDescriptors, renderData);
    
    bindMeshes(renderData);
    
    pushConstants(object);    

    vkCmdDrawIndexed(cmd, (uint32_t) object.Mesh->Indices.size(), 1, 0, 0, 0);
  }
}