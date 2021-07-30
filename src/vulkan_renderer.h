
#include "vulkan/vulkan.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define VK_CHECK(command)\
{\
  VkResult result = command;\
  assert(result == VK_SUCCESS);\
}

struct
stQueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
  std::optional<uint32_t> ComputeFamily;
	std::optional<uint32_t> PresentFamily;

	bool IsComplete()
	{
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};

struct
stQueue
{
  VkQueue Queue = VK_NULL_HANDLE;
  uint32_t Index = 0;
};

struct
stDevice
{
  VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
  VkDevice LogicalDevice = VK_NULL_HANDLE;
  stQueue GraphicsQueue = {};
  stQueue ComputeQueue = {};
  stQueue PresentQueue = {};
};

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


VkSurfaceKHR
CreateSurface(
  VkInstance instance,
  const stWindow& window
);

#define MAX_PHYSICAL_DEVICE_COUNT 16
#define MAX_SWAPCHAIN_IMAGE_COUNT 16

struct
stRenderer
{

  void
  Init(const stWindow& window)
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

    PhysicalDevice = init::pick_device(PhysicalDevices, PhysicalDeviceCount);
    assert(PhysicalDevice != VK_NULL_HANDLE);

    Device = init::create_device(PhysicalDevice, Surface);

    Deletion.PushFunction([=]{
      vkDestroyDevice(Device.LogicalDevice, nullptr);
    });

    Swapchain = init::create_swapchain(Device, Surface);

    Deletion.PushFunction([=]{
      vkDestroySwapchainKHR(Device.LogicalDevice, Swapchain, nullptr);
    });

    acquireSemaphore = init::create_semaphore(Device);
    releaseSemaphore = init::create_semaphore(Device);

    Deletion.PushFunction([=]{
      vkDestroySemaphore(Device.LogicalDevice, acquireSemaphore, nullptr);
      vkDestroySemaphore(Device.LogicalDevice, releaseSemaphore, nullptr);
    });

    VK_CHECK(vkGetSwapchainImagesKHR(Device.LogicalDevice, Swapchain, &SwapchainImageCount, SwapchainImages));

    CommandPool = init::create_command_pool(Device);

    Deletion.PushFunction([=]{
      vkDestroyCommandPool(Device.LogicalDevice, CommandPool, nullptr);
    });

    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(Device.LogicalDevice, &allocInfo, &CommandBuffer));
  }

  void
  Term()
  {
    Deletion.Flush();
  }

  void
  Render()
  {
    uint32_t imageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(Device.LogicalDevice, Swapchain, ~0ull, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

    VK_CHECK(vkResetCommandPool(Device.LogicalDevice, CommandPool, 0));
  
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &beginInfo));

    VkClearColorValue color = { 1.0f, 0.0f, 0.0f, 1.0f };
    VkImageSubresourceRange renge = {};
    renge.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    renge.levelCount = 1;
    renge.layerCount = 1;

    vkCmdClearColorImage(CommandBuffer, SwapchainImages[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &renge);

    VK_CHECK(vkEndCommandBuffer(CommandBuffer));

    VkPipelineStageFlags submitStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    submitInfo.pWaitDstStageMask = &submitStageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &CommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphore;

    vkQueueSubmit(Device.PresentQueue.Queue, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &Swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(Device.PresentQueue.Queue, &presentInfo));

    VK_CHECK(vkDeviceWaitIdle(Device.LogicalDevice));
  }

  VkInstance Instance = VK_NULL_HANDLE;
  VkSurfaceKHR Surface = VK_NULL_HANDLE;

  VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

  VkCommandPool CommandPool = VK_NULL_HANDLE;
  VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

  VkImage SwapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT];
  uint32_t SwapchainImageCount = ArrayCount(SwapchainImages);

  VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
  VkSemaphore releaseSemaphore = VK_NULL_HANDLE;

  VkPhysicalDevice PhysicalDevices[MAX_PHYSICAL_DEVICE_COUNT];
  uint32_t PhysicalDeviceCount = ArrayCount(PhysicalDevices);

  VkPhysicalDevice PhysicalDevice;
  stDevice Device;

  stDeletionQueue Deletion;
};