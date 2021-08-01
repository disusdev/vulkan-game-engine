
#include "vulkan/vulkan.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define VK_CHECK(command)\
{\
  VkResult result = command;\
  assert(result == VK_SUCCESS);\
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

    for (size_t i = 0; i < SwapchainImageCount; i++)
    {
      CommandBuffers[i] = init::create_command_buffer(Device, CommandPool, ForwardRenderPass, Framebuffers[i], SwapchainExtent);
      
      SwapchainDeletion.PushFunction([=]{
        vkFreeCommandBuffers(Device.LogicalDevice, CommandPool, 1, &CommandBuffers[i]);
      });
    }
  }

  void
  RecreateSwapchain()
  {
    if (Swapchain == VK_NULL_HANDLE)
    {
      return;
    }

    VK_CHECK(vkDeviceWaitIdle(Device.LogicalDevice));
    SwapchainDeletion.Flush();

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

    for (size_t i = 0; i < SwapchainImageCount; i++)
    {
      CommandBuffers[i] = init::create_command_buffer(Device, CommandPool, ForwardRenderPass, Framebuffers[i], SwapchainExtent);
      
      SwapchainDeletion.PushFunction([=]{
        vkFreeCommandBuffers(Device.LogicalDevice, CommandPool, 1, &CommandBuffers[i]);
      });
    }
  }

  void
  Term()
  {
    VK_CHECK(vkDeviceWaitIdle(Device.LogicalDevice));
    SwapchainDeletion.Flush();
    Deletion.Flush();
  }

  void
  Render()
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

  VkInstance Instance = VK_NULL_HANDLE;
  VkSurfaceKHR Surface = VK_NULL_HANDLE;

  VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

  uint32_t CurrentFrame = 0;

  VkFormat SwapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D SwapchainExtent = { 0, 0 };

  VkCommandPool CommandPool = VK_NULL_HANDLE;
  VkCommandBuffer CommandBuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

  VkRenderPass ForwardRenderPass = VK_NULL_HANDLE;

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