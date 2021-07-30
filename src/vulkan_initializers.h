
namespace
init
{

VkInstance
create_instance(
  uint32_t apiVersion = VK_API_VERSION_1_1,
  const char* appName = "default_app",
  uint32_t appVersion = VK_MAKE_VERSION(0, 0, 1),
  const char* engineName = "default_engine",
  uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 1) )
{
  VkInstance instance = VK_NULL_HANDLE;

  VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
  app_info.pApplicationName = appName;
  app_info.applicationVersion = appVersion;
  app_info.pEngineName = engineName;
  app_info.engineVersion = engineVersion;
  app_info.apiVersion = apiVersion;

  VkInstanceCreateInfo inst_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
  inst_info.pApplicationInfo = &app_info;

#if defined(DEBUG)
  const char* layers[] =
  {
    "VK_LAYER_KHRONOS_validation"
  };

  inst_info.enabledLayerCount = ArrayCount(layers);
  inst_info.ppEnabledLayerNames = layers;
#endif

  const char* extentions[] =
  {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
  };

  inst_info.enabledExtensionCount = ArrayCount(extentions);
  inst_info.ppEnabledExtensionNames = extentions;

  VK_CHECK(vkCreateInstance(&inst_info, nullptr, &instance));

  return instance;
}

VkPhysicalDevice
pick_device(
  VkPhysicalDevice* devices,
  uint32_t deviceCount )
{
  VkPhysicalDeviceProperties props;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  for (size_t i = 0; i < deviceCount; i++)
  {
    physicalDevice = devices[i];
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      break;
  }

  return physicalDevice;
}

stQueueFamilyIndices
find_queue_families(
  VkPhysicalDevice device,
  VkSurfaceKHR surface)
{
  stQueueFamilyIndices familyIndices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    device,
    &queueFamilyCount,
    nullptr
  );

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
    device,
    &queueFamilyCount,
    queueFamilies.data()
  );

  for (uint32_t i = 0; i < queueFamilies.size(); i++)
  {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      familyIndices.GraphicsFamily = i;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      familyIndices.ComputeFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport)
    {
      familyIndices.PresentFamily = i;
    }

    if (familyIndices.IsComplete())
    {
      break;
    }
  }

  return familyIndices;
}

stDevice
create_device(
  VkPhysicalDevice physicalDevice,
  VkSurfaceKHR surface
)
{
  stDevice device = {};

  device.PhysicalDevice = physicalDevice;

  float queueProps[] = { 1.0f };

  stQueueFamilyIndices indices = find_queue_families(physicalDevice, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies =
  {
    indices.GraphicsFamily.value(),
    indices.PresentFamily.value()
  };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo = { 
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
    };
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.push_back(queueCreateInfo);
  };

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  deviceFeatures.sampleRateShading = VK_TRUE;

  const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  VkDeviceCreateInfo deviceCreateInfo = { 
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
  };
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(
    queueCreateInfos.size()
  );
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.enabledExtensionCount = ArrayCount(deviceExtensions);
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  
  VK_CHECK(vkCreateDevice(
    physicalDevice,
    &deviceCreateInfo,
    0, &device.LogicalDevice)
  );

  vkGetDeviceQueue(
    device.LogicalDevice,
    indices.GraphicsFamily.value(),
    0, &device.GraphicsQueue.Queue
  );

  device.GraphicsQueue.Index = indices.GraphicsFamily.value();

  vkGetDeviceQueue(
    device.LogicalDevice,
    indices.ComputeFamily.value(),
    0, &device.ComputeQueue.Queue
  );

  device.ComputeQueue.Index = indices.ComputeFamily.value();

  vkGetDeviceQueue(
    device.LogicalDevice,
    indices.PresentFamily.value(),
    0, &device.PresentQueue.Queue
  );

  device.PresentQueue.Index = indices.PresentFamily.value();

  return device;
}

VkSwapchainKHR
create_swapchain(
  const stDevice& device,
  VkSurfaceKHR surface,
  VkExtent2D imageExtend = { 1264, 681 },
  uint32_t minImageCount = 2,
  VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
  VkPresentModeKHR mode = VK_PRESENT_MODE_MAILBOX_KHR)
{
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;

  uint32_t formatCount = 32;
  VkSurfaceFormatKHR formats[32];

  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
    device.PhysicalDevice,
    surface,
    &formatCount,
    formats)
  );

  VkSwapchainCreateInfoKHR createInfo = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
  };
  createInfo.surface = surface;
  createInfo.minImageCount = minImageCount;
  createInfo.imageFormat = formats[0].format;
  createInfo.imageColorSpace = formats[0].colorSpace;
  // 1264,681
  createInfo.imageExtent = imageExtend;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = imageUsage;
  uint32_t queueIndices[] = {
    device.GraphicsQueue.Index,
    device.ComputeQueue.Index,
    device.PresentQueue.Index
  };
  createInfo.queueFamilyIndexCount = ArrayCount(queueIndices);
  createInfo.pQueueFamilyIndices = queueIndices;
  createInfo.presentMode = mode;

  createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  VK_CHECK(vkCreateSwapchainKHR(
    device.LogicalDevice,
    &createInfo,
    nullptr,
    &swapchain)
  );

  return swapchain;
}

VkSemaphore
create_semaphore(
  const stDevice& device)
{
  VkSemaphore semaphore = VK_NULL_HANDLE;

  VkSemaphoreCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };
  // createInfo.flags = 

  VK_CHECK(vkCreateSemaphore(
    device.LogicalDevice,
    &createInfo,
    nullptr,
    &semaphore)
  );

  return semaphore;
}

VkCommandPool
create_command_pool(
  stDevice device,
  VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
{
  VkCommandPool pool;

  VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  createInfo.flags = flags;
  createInfo.queueFamilyIndex = device.GraphicsQueue.Index;

  VK_CHECK(vkCreateCommandPool(device.LogicalDevice, &createInfo, nullptr, &pool));

  return pool;
}

}