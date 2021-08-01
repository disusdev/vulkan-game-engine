
#include "vulkan/vulkan.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define VK_CHECK(command)\
{\
  VkResult result = command;\
  assert(result == VK_SUCCESS);\
}

struct
stVertex
{
  glm::vec2 Position = { 0.0f, 0.0f };
  glm::vec3 Color = { 0.0f, 0.0f, 0.0f };

  static VkVertexInputBindingDescription
  GetBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(stVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  GetAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(stVertex, Position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(stVertex, Color);

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
};

VkSurfaceKHR
CreateSurface(
  VkInstance instance,
  const stWindow& window
);

VkExtent2D
ChooseSwapExtent(
  const VkSurfaceCapabilitiesKHR& capabilities
);

#include "vulkan_initializers.h"

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
  Render();

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

  VkBuffer VertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;

  VkImage SwapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT];
  VkImageView SwapchainImageViews[MAX_SWAPCHAIN_IMAGE_COUNT];
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
  Instance = init::create_instance();

  Surface = CreateSurface(Instance, window);

  Deletion.PushFunction([=]{
    vkDestroySurfaceKHR(Instance, Surface, nullptr);
    vkDestroyInstance(Instance, nullptr);
  });

  VK_CHECK(vkEnumeratePhysicalDevices(
    Instance,
    &PhysicalDeviceCount,
    PhysicalDevices)
  );

  assert(PhysicalDeviceCount > 0);

  Device.PhysicalDevice = init::pick_device(PhysicalDevices, PhysicalDeviceCount);
  assert(Device.PhysicalDevice != VK_NULL_HANDLE);

  Device = init::create_device(Device.PhysicalDevice, Surface);

  Deletion.PushFunction([=]{
    vkDestroyDevice(Device.LogicalDevice, nullptr);
  });

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    AcquireSemaphores[i] = init::create_semaphore(Device);
    ReleaseSemaphores[i] = init::create_semaphore(Device);
    InFlightFence[i] = init::create_fence(Device);

    Deletion.PushFunction([=]{
      vkDestroySemaphore(Device.LogicalDevice, AcquireSemaphores[i], nullptr);
      vkDestroySemaphore(Device.LogicalDevice, ReleaseSemaphores[i], nullptr);
      vkDestroyFence(Device.LogicalDevice, InFlightFence[i], nullptr);
    });
  }

  CommandPool = init::create_command_pool(Device, 0);

  Deletion.PushFunction([=]{
    vkDestroyCommandPool(Device.LogicalDevice, CommandPool, nullptr);
  });

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
    SwapchainImageViews,
    SwapchainImageCount
  );

  SwapchainDeletion.PushFunction([=]{
    vkDestroySwapchainKHR(Device.LogicalDevice, Swapchain, nullptr);
  });

  ForwardRenderPass = init::create_render_pass(Device, SwapchainImageFormat);

  SwapchainDeletion.PushFunction([=]{
    vkDestroyRenderPass(Device.LogicalDevice, ForwardRenderPass, nullptr);
  });

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    Framebuffers[i] = init::create_framebuffer(Device, ForwardRenderPass, SwapchainExtent, SwapchainImageViews[i]);

    SwapchainDeletion.PushFunction([=]{
      vkDestroyFramebuffer(Device.LogicalDevice, Framebuffers[i], nullptr);
      vkDestroyImageView(Device.LogicalDevice, SwapchainImageViews[i], nullptr);
    });
  }

  GraphicsPipeline = init::create_pipeline(Device, SwapchainExtent, ForwardRenderPass);

  SwapchainDeletion.PushFunction([=]{
    vkDestroyPipeline(Device.LogicalDevice, GraphicsPipeline.Pipeline, nullptr);
    vkDestroyPipelineLayout(Device.LogicalDevice, GraphicsPipeline.Layout, nullptr);
  });

  stVertex vertices[] = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  };

  VertexBuffer = init::create_vertex_buffer<stVertex>(Device, ArrayCount(vertices));

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(Device.LogicalDevice, VertexBuffer, &memRequirements);
  
  VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = init::find_memory_type(Device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VK_CHECK(vkAllocateMemory(Device.LogicalDevice, &allocInfo, nullptr, &VertexBufferMemory));

  VK_CHECK(vkBindBufferMemory(Device.LogicalDevice, VertexBuffer, VertexBufferMemory, 0));

  SwapchainDeletion.PushFunction([=]{
    vkDestroyBuffer(Device.LogicalDevice, VertexBuffer, nullptr);
    vkFreeMemory(Device.LogicalDevice, VertexBufferMemory, nullptr);
  });

  void* data;
  vkMapMemory(Device.LogicalDevice, VertexBufferMemory, 0, ArrayCount(vertices) * sizeof(stVertex), 0, &data);
    memcpy(data, vertices, ArrayCount(vertices) * sizeof(stVertex));
  vkUnmapMemory(Device.LogicalDevice, VertexBufferMemory);

  for (size_t i = 0; i < SwapchainImageCount; i++)
  {
    CommandBuffers[i] = init::create_command_buffer(Device, CommandPool, ForwardRenderPass, Framebuffers[i], SwapchainExtent, GraphicsPipeline,
    [=](VkCommandBuffer cb, VkPipeline gp)
    {
      vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, gp);

      VkBuffer vertexBuffers[] = { VertexBuffer };
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);

      vkCmdDraw(cb, ArrayCount(vertices), 1, 0, 0);
    });
    
    SwapchainDeletion.PushFunction([=]{
      vkFreeCommandBuffers(Device.LogicalDevice, CommandPool, 1, &CommandBuffers[i]);
    });
  }
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
stRenderer::Render()
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