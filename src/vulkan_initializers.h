
namespace
init
{

VkInstance
create_instance(
  uint32_t apiVersion = VK_API_VERSION_1_1,
  const char* appName = "default_app",
  uint32_t appVersion = VK_MAKE_VERSION(0, 0, 1),
  const char* engineName = "default_engine",
  uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 1),
  stDeletionQueue* deleteionQueue = nullptr)
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

  if(deleteionQueue)
  deleteionQueue->PushFunction([=]{
    vkDestroyInstance(instance, nullptr);
  });

  return instance;
}

VkSampleCountFlagBits
get_max_usable_sample_count(
  const stDevice& device)
{
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(device.PhysicalDevice, &physicalDeviceProperties);

  VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
  if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
  if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
  if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
  if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
  if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

  return VK_SAMPLE_COUNT_1_BIT;
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

VkFormat
find_supported_format(
  const stDevice& device,
  const std::vector<VkFormat>& candidates,
  VkImageTiling tiling,
  VkFormatFeatureFlags features
)
{
  for (VkFormat format : candidates)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device.PhysicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
    {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }

  assert(false);
  return VK_FORMAT_UNDEFINED;
}

VkFormat
find_depth_format(
  const stDevice& device)
{
  return find_supported_format(
    device,
    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

bool
has_stencil_component(
  VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

stDevice
create_device(
  VkPhysicalDevice physicalDevice,
  VkSurfaceKHR surface,
  stDeletionQueue* deletionQueue
)
{
  stDevice device = {};

  device.PhysicalDevice = physicalDevice;

  float queueProps[] = { 1.0f };

  VkQueueFamilyProperties queueFamilies[32];
  uint32_t queueFamilyCount = ArrayCount(queueFamilies);
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice,
    &queueFamilyCount,
    queueFamilies
  );

  for (uint32_t i = 0; i < queueFamilyCount; i++)
  {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      device.Queues[QUEUE_TYPE_GRAPHICS].Index = i;
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device.PhysicalDevice, i, surface, &presentSupport);
      assert(presentSupport);
    }
    else if(queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
       device.Queues[QUEUE_TYPE_COMPUTE].Index = i;
    }
  }

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  float queuePriority = 1.0f;

  VkDeviceQueueCreateInfo queueCreateInfo = { 
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
  };
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;
  queueCreateInfo.queueFamilyIndex = device.Queues[QUEUE_TYPE_GRAPHICS].Index;
  queueCreateInfos.push_back(queueCreateInfo);
  queueCreateInfo.queueFamilyIndex = device.Queues[QUEUE_TYPE_COMPUTE].Index;
  queueCreateInfos.push_back(queueCreateInfo);

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
    device.Queues[QUEUE_TYPE_GRAPHICS].Index,
    0, &device.Queues[QUEUE_TYPE_GRAPHICS].Queue
  );

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyDevice(device.LogicalDevice, nullptr);
  });

  return device;
}

VkSurfaceFormatKHR
choose_swap_surface_format(
  const VkSurfaceFormatKHR* availableFormats,
  uint32_t availableFormatCount)
{
  for (uint32_t i = 0; i < availableFormatCount; i++)
  {
    if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB 
      && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
    {
      return availableFormats[i];
    }
  }

  return availableFormats[0];
}

void
create_image_view(
  const stDevice& device,
  stImage& image,
  VkFormat format,
  VkImageAspectFlags aspectFlags,
  uint32_t mipLevels)
{
  VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
  viewInfo.image = image.Src;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  // viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  // viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  // viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  // viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  VK_CHECK(vkCreateImageView(device.LogicalDevice, &viewInfo, nullptr, &image.View));
}

VkSwapchainKHR
create_swapchain(
  const stDevice& device,
  VkSurfaceKHR surface,
  VkFormat& swapChainImageFormat,
  VkExtent2D& swapChainExtent,
  stImage* swapchainImages,
  uint32_t& swapchainImageCount,
  VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  VkPresentModeKHR mode = VK_PRESENT_MODE_MAILBOX_KHR,
  stDeletionQueue* deleteionQueue = nullptr)
{
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.PhysicalDevice, surface, &capabilities);

  uint32_t formatCount = 32;
  VkSurfaceFormatKHR formats[32];

  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
    device.PhysicalDevice,
    surface,
    &formatCount,
    formats)
  );

  VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(formats, ArrayCount(formats));

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = ChooseSwapExtent(capabilities);

  VkSwapchainCreateInfoKHR createInfo = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
  };
  createInfo.surface = surface;
  createInfo.minImageCount = capabilities.minImageCount + 1;
  createInfo.imageFormat = swapChainImageFormat;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = swapChainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = imageUsage;


  uint32_t queueIndices[] = {
    device.Queues[QUEUE_TYPE_GRAPHICS].Index,
    device.Queues[QUEUE_TYPE_COMPUTE].Index
  };

  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

  createInfo.queueFamilyIndexCount = ArrayCount(queueIndices);
  createInfo.pQueueFamilyIndices = queueIndices;
  createInfo.presentMode = mode;

  createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.clipped = VK_TRUE;

  VK_CHECK(vkCreateSwapchainKHR(
    device.LogicalDevice,
    &createInfo,
    nullptr,
    &swapchain)
  );

  std::vector<VkImage> images(swapchainImageCount);
  for (size_t i = 0; i < swapchainImageCount; i++)
  {
    images[i] = swapchainImages[i].Src;
  }

  VK_CHECK(vkGetSwapchainImagesKHR(device.LogicalDevice, swapchain, &swapchainImageCount, images.data()));

  for (uint32_t i = 0; i < swapchainImageCount; i++)
  {
    swapchainImages[i].Src = images[i];
    create_image_view(device, swapchainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
  }

  if (deleteionQueue)
  deleteionQueue->PushFunction([=]{
    vkDestroySwapchainKHR(device.LogicalDevice, swapchain, nullptr);
  });

  return swapchain;
}

VkSemaphore
create_semaphore(
  const stDevice& device,
  stDeletionQueue* deletionQueue)
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

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroySemaphore(device.LogicalDevice, semaphore, nullptr);
  });

  return semaphore;
}

VkCommandPool
create_command_pool(
  const stDevice& device,
  VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
  stDeletionQueue* deletionQueue = nullptr)
{
  VkCommandPool pool;

  VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  createInfo.flags = flags;
  createInfo.queueFamilyIndex = device.Queues[QUEUE_TYPE_GRAPHICS].Index;

  VK_CHECK(vkCreateCommandPool(device.LogicalDevice, &createInfo, nullptr, &pool));

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyCommandPool(device.LogicalDevice, pool, nullptr);
  });

  return pool;
}

VkRenderPass
create_depth_render_pass(
  const stDevice& device,
  VkFormat renderFormat,
  stDeletionQueue* deletionQueue
)
{
  VkRenderPass renderPass = VK_NULL_HANDLE;

	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = renderFormat;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &attachmentDescription;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
	render_pass_info.pDependencies = dependencies.data();

	VK_CHECK(vkCreateRenderPass(device.LogicalDevice, &render_pass_info, nullptr, &renderPass));

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyRenderPass(device.LogicalDevice, renderPass, nullptr);
  });

  return renderPass;
}

VkRenderPass
create_render_pass(
  const stDevice& device,
  VkFormat renderFormat,
  VkSampleCountFlagBits msaaSamples,
  stDeletionQueue* deletionQueue
)
{
  VkRenderPass renderPass = VK_NULL_HANDLE;

  VkAttachmentDescription color_attachment = {};
	color_attachment.format = renderFormat;
	color_attachment.samples = msaaSamples;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL VK_IMAGE_LAYOUT_PRESENT_SRC_KHR

  VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = init::find_depth_format(device);
	depth_attachment.samples = msaaSamples;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription color_attachment_resolve = {};
  color_attachment_resolve.format = renderFormat;
  color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_resolve_ref = {};
  color_attachment_resolve_ref.attachment = 2;
  color_attachment_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
  subpass.pResolveAttachments = &color_attachment_resolve_ref;

	//1 dependency, which is from "outside" into the subpass. And we can read or write color
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcAccessMask = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  //array of 2 attachments, one for the color, and other for depth
	VkAttachmentDescription attachments[3] = { color_attachment, depth_attachment, color_attachment_resolve };

	VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	render_pass_info.attachmentCount = ArrayCount(attachments);
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(device.LogicalDevice, &render_pass_info, nullptr, &renderPass));

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyRenderPass(device.LogicalDevice, renderPass, nullptr);
  });

  return renderPass;
}

VkFramebuffer
create_framebuffer(
  const stDevice& device,
  VkRenderPass renderPass,
  VkExtent2D bufferSize,
  VkImageView* attachments,
  uint32_t attachmentCount,
  //VkImageView swapChainImageView,
  //VkImageView depthImageView,
  //VkImageView colorImageView,
  stDeletionQueue* deletionQueue
  )
{
  VkFramebuffer framebuffer = VK_NULL_HANDLE;

  //VkImageView attachments[3] = 
  //{
  //  colorImageView,
  //  depthImageView,
  //  swapChainImageView
  //};

  VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
  createInfo.renderPass = renderPass;
  createInfo.width = bufferSize.width;
  createInfo.height = bufferSize.height;
  createInfo.attachmentCount = attachmentCount;
  createInfo.pAttachments = attachments;
  createInfo.layers = 1;

  VK_CHECK(vkCreateFramebuffer(device.LogicalDevice, &createInfo, nullptr, &framebuffer));

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyFramebuffer(device.LogicalDevice, framebuffer, nullptr);
    // vkDestroyImageView(device.LogicalDevice, swapChainImageView, nullptr);
  });

  return framebuffer;
}

VkFence
create_fence(
  const stDevice& device,
  stDeletionQueue* deletionQueue)
{
  VkFence fence = VK_NULL_HANDLE;

  VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
  createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VK_CHECK(vkCreateFence(device.LogicalDevice, &createInfo, nullptr, &fence));

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyFence(device.LogicalDevice, fence, nullptr);
  });

  return fence;
}

void
create_command_buffers(
  const stDevice& device,
  VkCommandPool commandPool,
  VkCommandBuffer* commandBuffers = nullptr,
  uint32_t count = 0,
  VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
  stDeletionQueue* deletionQueue = nullptr)
{
  VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocInfo.commandPool = commandPool;
  allocInfo.level = level;
  allocInfo.commandBufferCount = count;

  VK_CHECK(vkAllocateCommandBuffers(device.LogicalDevice, &allocInfo, commandBuffers));

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkFreeCommandBuffers(device.LogicalDevice, commandPool, count, commandBuffers);
  });
}

VkShaderModule
load_shader_module(
  const stDevice& device,
  const char* filePath)
{
  FILE* file = fopen(filePath, "rb");

  assert(file);
  
  fseek(file, 0L, SEEK_END);
  uint32_t fileSize = ftell(file);
  fseek(file, 0L, SEEK_SET);

  static uint32_t buffer[4 * 256 * 256];
  
  fread(buffer, fileSize, 1, file);
  fclose(file);

  VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  createInfo.pNext = nullptr;
  createInfo.codeSize = fileSize; 
  createInfo.pCode = buffer;
  
  VkShaderModule shaderModule;
  VK_CHECK(vkCreateShaderModule(device.LogicalDevice, &createInfo, nullptr, &shaderModule));

  return shaderModule;
}

VkPipelineVertexInputStateCreateInfo
vertex_input_state_create_info()
{
  VkPipelineVertexInputStateCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  info.pNext = nullptr;
  info.vertexBindingDescriptionCount = 0;
  info.vertexAttributeDescriptionCount = 0;
  return info;
}

VkPipelineShaderStageCreateInfo
pipeline_shader_stage_create_info(
  VkShaderStageFlagBits stage,
  VkShaderModule shaderModule)
{
  VkPipelineShaderStageCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  info.stage = stage;
  info.module = shaderModule;
  info.pName = "main";
  return info;
}

VkPipelineLayoutCreateInfo
pipeline_layout_create_info()
{
  VkPipelineLayoutCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  info.flags = 0;
  info.setLayoutCount = 0;
  info.pSetLayouts = nullptr;
  info.pushConstantRangeCount = 0;
  info.pPushConstantRanges = nullptr;
  return info;
}

// pipeline_buiilder
// VertexInputDescription VertexDescription
// VkPipelineVertexInputStateCreateInfo VertexInputInfo
// VkPipelineInputAssemblyStateCreateInfo InputAssembly
// InputAssembler // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
// Rasterizer // VK_POLYGONE_MODE_FILL
// Rasterizer.cullMode = VK_CULL_MODE_NONE;//BACK_BIT;
// Multisampling
// colorBlendAttachemnt
// DepthStencil

VkPipeline
build_pipeline(
  const stDevice& device,
  VkExtent2D windowExtend,
  VkRenderPass renderPass,
  VkPipelineLayout pipelineLayout,
  stVertexInputDescription vertexDescription,
  VkPipelineColorBlendAttachmentState colorBlendAttachment,
  VkPipelineInputAssemblyStateCreateInfo inputAssembly,
  VkPipelineRasterizationStateCreateInfo rasterizer,
  VkPipelineMultisampleStateCreateInfo multisampling,
  VkPipelineDepthStencilStateCreateInfo depthStencil,
  VkPipelineShaderStageCreateInfo* shaderStages,
  uint32_t shaderStageCount,
  stDeletionQueue* deletionQueue)
{
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = vertex_input_state_create_info();
  vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.Attributes.data();
  vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.Attributes.size();
  vertexInputInfo.pVertexBindingDescriptions = vertexDescription.Bindings.data();
  vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDescription.Bindings.size();

  VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) windowExtend.width;
  viewport.height = (float) windowExtend.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = windowExtend;

  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

  pipelineInfo.stageCount = shaderStageCount;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

  VkDynamicState dynamicStates[] = 
  {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_DEPTH_BIAS
  };

  dynamicState.pDynamicStates = dynamicStates;
  dynamicState.dynamicStateCount = ArrayCount(dynamicStates);

  pipelineInfo.pDynamicState = &dynamicState;

  VkPipeline pipeline = VK_NULL_HANDLE;

  VK_CHECK(vkCreateGraphicsPipelines(device.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

  return  pipeline;
}

stPipeline
create_pipeline(
  const stDevice& device,
  VkExtent2D windowExtend,
  VkRenderPass renderPass,
  VkSampleCountFlagBits msaaSamples,
  stDeletionQueue* deletionQueue)
{
  stPipeline pipeline = {};

  VkShaderModule vertShaderModule = load_shader_module(device, "./data/shaders/shader.vert.spv");
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkShaderModule fragShaderModule = load_shader_module(device, "./data/shaders/shader.frag.spv");
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  auto bindingDescription = GetBindingDescription<stVertex>();
  auto attributeDescriptions = GetAttributeDescriptions();

  // bindings
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) windowExtend.width;
  viewport.height = (float) windowExtend.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = windowExtend;

  VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;// VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  multisampling.sampleShadingEnable = VK_TRUE;
  multisampling.rasterizationSamples = msaaSamples;
  multisampling.minSampleShading = .2f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  //colorBlendAttachment.blendEnable = VK_TRUE;
  //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  //colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  //colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  //colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  //colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f ;
  colorBlending.blendConstants[1] = 0.0f ;
  colorBlending.blendConstants[2] = 0.0f ;
  colorBlending.blendConstants[3] = 0.0f ;

  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
  dynamicState.dynamicStateCount = ArrayCount(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;

  // ubo
  //VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  //uboLayoutBinding.binding = 0;
  //uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  //uboLayoutBinding.descriptorCount = 1;
  //uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  //uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

  // sampler
  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  
  std::array<VkDescriptorSetLayoutBinding, 1> bindings = { /*uboLayoutBinding,*/ samplerLayoutBinding };
  VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VK_CHECK(vkCreateDescriptorSetLayout(device.LogicalDevice, &layoutInfo, nullptr, &pipeline.DescriptorSet));

  VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(stMeshPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &pipeline.DescriptorSet;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &push_constant;

  VK_CHECK(vkCreatePipelineLayout(device.LogicalDevice, &pipelineLayoutInfo, nullptr, &pipeline.Layout));

  VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f; // Optional
  depthStencil.maxDepthBounds = 1.0f; // Optional
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {}; // Optional
  depthStencil.back = {}; // Optional

  VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  pipelineInfo.stageCount = ArrayCount(shaderStages);
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = pipeline.Layout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(device.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.Pipeline));

  vkDestroyShaderModule(device.LogicalDevice, vertShaderModule, nullptr);
  vkDestroyShaderModule(device.LogicalDevice, fragShaderModule, nullptr);

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyDescriptorSetLayout(device.LogicalDevice, pipeline.DescriptorSet, nullptr);
    vkDestroyPipeline(device.LogicalDevice, pipeline.Pipeline, nullptr);
    vkDestroyPipelineLayout(device.LogicalDevice, pipeline.Layout, nullptr);
    vkDestroyPipelineLayout(device.LogicalDevice, pipeline.MeshLayout, nullptr);
  });

  return pipeline;
}

uint32_t
find_memory_type(
  stDevice device,
  uint32_t typeFilter,
  VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(device.PhysicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
      }
  }

  assert(false);
  return 0;
}

void
create_buffer(
  stDevice device,
  VkDeviceSize size,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties,
  stBuffer& buffer,
  stDeletionQueue* deletionQueue)
{
  VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK(vkCreateBuffer(device.LogicalDevice, &bufferInfo, nullptr, &buffer.Buffer));

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device.LogicalDevice, buffer.Buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, properties);

  VK_CHECK(vkAllocateMemory(device.LogicalDevice, &allocInfo, nullptr, &buffer.Memory));

  vkBindBufferMemory(device.LogicalDevice, buffer.Buffer, buffer.Memory, 0);

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyBuffer(device.LogicalDevice, buffer.Buffer, nullptr);
    vkFreeMemory(device.LogicalDevice, buffer.Memory, nullptr);
  });
}

VkCommandBuffer
begin_single_time_commands(
  const stDevice& device,
  VkCommandPool commandPool)
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device.LogicalDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void
end_single_time_command(
  const stDevice& device,
  VkCommandPool commandPool,
  VkCommandBuffer commandBuffer
)
{
  vkEndCommandBuffer(commandBuffer);
  
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  
  vkQueueSubmit(device.Queues[QUEUE_TYPE_GRAPHICS].Queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(device.Queues[QUEUE_TYPE_GRAPHICS].Queue);
  
  vkFreeCommandBuffers(device.LogicalDevice, commandPool, 1, &commandBuffer);
}

void
copy_buffer(
  const stDevice& device,
  VkBuffer srcBuffer,
  VkBuffer dstBuffer,
  VkDeviceSize size,
  VkCommandPool commandPool)
{
  VkCommandBuffer commandBuffer = begin_single_time_commands(device, commandPool);

  VkBufferCopy copyRegion = {};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  end_single_time_command(device, commandPool, commandBuffer);
}

//void
//create_uniform_buffer(
//  const stDevice& device,
//  stBuffer* buffers,
//  uint32_t count
//)
//{
//  VkDeviceSize bufferSize = sizeof(stUniformBufferObject);
//
//  for (size_t i = 0; i < count; i++)
//  {
//    create_buffer(
//      device,
//      bufferSize,
//      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//      buffers[i]
//    );
//  }
//}

VkDescriptorPool
create_descriptor_pools(
  const stDevice& device,
  VkDescriptorPoolSize* poolSizes, 
  uint32_t poolCount,
  uint32_t swapchainImageCount,
  stDeletionQueue* deletionQueue
)
{
  VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  poolInfo.poolSizeCount = poolCount;
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = swapchainImageCount * MAX_TEXTURE_COUNT;

  VkDescriptorPool descriptorPool;

  vkCreateDescriptorPool(device.LogicalDevice, &poolInfo, nullptr, &descriptorPool);

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyDescriptorPool(device.LogicalDevice, descriptorPool, nullptr);
  });

  return descriptorPool;
}

void
update_descriptor_set(
  const stDevice& device,
  VkDescriptorSet descriptorSet,
  const stTexture& texture//,
  //uint32_t swapChainImageCount
  )
{
  //for (size_t i = 0; i < swapChainImageCount; i++)
  {
    //VkDescriptorBufferInfo bufferInfo{};
    //bufferInfo.buffer = uniformBuffers[i].Buffer;
    //bufferInfo.offset = 0;
    //bufferInfo.range = sizeof(stUniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.Image.View;
    imageInfo.sampler = texture.Sampler;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

    //descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //descriptorWrites[0].dstSet = descriptorSets[i];
    //descriptorWrites[0].dstBinding = 0;
    //descriptorWrites[0].dstArrayElement = 0;
    //descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //descriptorWrites[0].descriptorCount = 1;
    //descriptorWrites[0].pBufferInfo = &bufferInfo;
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;//s[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device.LogicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
}

void
create_descriptor_sets(
  const stDevice& device,
  const stPipeline& pipeline,
  VkDescriptorPool descriptorPool,
  VkDescriptorSet* descriptorSets,
  // stBuffer* uniformBuffers,
  uint32_t swapChainImageCount,
  const stTexture& texture
  )
{
  std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount, pipeline.DescriptorSet);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = swapChainImageCount;
  allocInfo.pSetLayouts = layouts.data();

  VK_CHECK(vkAllocateDescriptorSets(device.LogicalDevice, &allocInfo, descriptorSets));

  for (size_t i = 0; i < swapChainImageCount; i++)
  {
    //VkDescriptorBufferInfo bufferInfo{};
    //bufferInfo.buffer = uniformBuffers[i].Buffer;
    //bufferInfo.offset = 0;
    //bufferInfo.range = sizeof(stUniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.Image.View;
    imageInfo.sampler = texture.Sampler;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

    //descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //descriptorWrites[0].dstSet = descriptorSets[i];
    //descriptorWrites[0].dstBinding = 0;
    //descriptorWrites[0].dstArrayElement = 0;
    //descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //descriptorWrites[0].descriptorCount = 1;
    //descriptorWrites[0].pBufferInfo = &bufferInfo;
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device.LogicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }
}

stImage
create_image(
  const stDevice& device,
  uint32_t width,
  uint32_t height,
  uint32_t mipLevels,
  VkSampleCountFlagBits numSample,
  VkFormat format,
  VkImageTiling tiling,
  VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties)
{
  stImage image = {};

  VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(width);
  imageInfo.extent.height = static_cast<uint32_t>(height);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = numSample;
  imageInfo.flags = 0; // Optional
  
  VK_CHECK(vkCreateImage(device.LogicalDevice, &imageInfo, nullptr, &image.Src));

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device.LogicalDevice, image.Src, &memRequirements);
  
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, properties);
  
  VK_CHECK(vkAllocateMemory(device.LogicalDevice, &allocInfo, nullptr, &image.Memory));
  
  vkBindImageMemory(device.LogicalDevice, image.Src, image.Memory, 0);

  return image;
}

void
transition_image_layout(
  const stDevice& device,
  VkCommandPool commandPool,
  VkImage image,
  VkFormat format,
  VkImageLayout oldLayout,
  VkImageLayout newLayout,
  uint32_t mipLevels
)
{
  VkCommandBuffer commandBuffer = begin_single_time_commands(device, commandPool);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (has_stencil_component(format))
    {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;  
  
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }
  else
  {
    assert(false);
  }
  
  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  end_single_time_command(device, commandPool, commandBuffer);
}

void
copy_buffer_to_image(
  const stDevice& device,
  VkCommandPool commandPool,
  VkBuffer buffer,
  VkImage image,
  uint32_t width,
  uint32_t height)
{
  VkCommandBuffer commandBuffer = begin_single_time_commands(device, commandPool);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {
    width,
    height,
    1
  };

  vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  end_single_time_command(device, commandPool, commandBuffer);
}

void
generate_mipmaps(
  const stDevice& device,
  VkCommandPool commandPool,
  VkImage image,
  VkFormat imageFormat,
  int32_t texWidth,
  int32_t texHeight,
  uint32_t mipLevels
)
{
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(device.PhysicalDevice, imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
  {
    assert(false);
  }

  VkCommandBuffer commandBuffer = begin_single_time_commands(device, commandPool);

  VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;
  
  for (uint32_t i = 1; i < mipLevels; i++)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier
    );

    VkImageBlit blit = {};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(commandBuffer,
      image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit,
      VK_FILTER_LINEAR
    );

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier
    );

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  end_single_time_command(device, commandPool, commandBuffer);
}

stTexture
create_texture_image(
  const stDevice& device,
  VkCommandPool commandPool,
  const char* path
)
{
  stTexture texture = {};
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  texture.MipLevels = static_cast<uint32_t>(std::floor(std::log2(utils::Max(texWidth, texHeight)))) + 1;
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  assert(pixels);

  stBuffer staging;

  create_buffer(
    device,
    imageSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    staging, nullptr
  ); 

  void* data;
  vkMapMemory(device.LogicalDevice, staging.Memory, 0, imageSize, 0, &data);
      memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device.LogicalDevice, staging.Memory);

  stbi_image_free(pixels);

  texture.Image = create_image(
    device,
    texWidth,
    texHeight,
    texture.MipLevels,
    VK_SAMPLE_COUNT_1_BIT,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );

  transition_image_layout(
    device,
    commandPool,
    texture.Image.Src,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    texture.MipLevels
  );

  copy_buffer_to_image(
    device,
    commandPool,
    staging.Buffer,
    texture.Image.Src,
    static_cast<uint32_t>(texWidth),
    static_cast<uint32_t>(texHeight)
  );

  //transition_image_layout(
  //  device,
  //  commandPool,
  //  texture.Image.Src,
  //  VK_FORMAT_R8G8B8A8_SRGB,
  //  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  //  1
  //);

  generate_mipmaps(device, commandPool, texture.Image.Src, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, texture.MipLevels);

  vkDestroyBuffer(device.LogicalDevice, staging.Buffer, nullptr);
  vkFreeMemory(device.LogicalDevice, staging.Memory, nullptr);

  return texture;
}

void
create_texture_image_view(
  const stDevice& device,
  stTexture& texture)
{
  create_image_view(device, texture.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture.MipLevels);
}

VkSampler
create_sampler(
  const stDevice& device,
  VkFilter filter,
  VkSamplerAddressMode addressMode,
  VkBorderColor borderColor,

  uint32_t mipLevels)
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

  samplerInfo.magFilter = filter;
  samplerInfo.minFilter = filter;

  samplerInfo.addressModeU = addressMode;
  samplerInfo.addressModeV = addressMode;
  samplerInfo.addressModeW = addressMode;

  VkPhysicalDeviceProperties properties = {};
  vkGetPhysicalDeviceProperties(device.PhysicalDevice, &properties);
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

  samplerInfo.borderColor = borderColor;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = (float) mipLevels;

  VkSampler sampler;
  VK_CHECK(vkCreateSampler(device.LogicalDevice, &samplerInfo, nullptr, &sampler));
  return sampler;
}

VkSampler
create_texture_sampler(
  const stDevice& device,
  uint32_t mipLevels)
{
  return create_sampler(
    device,
    VK_FILTER_LINEAR,
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    mipLevels
  );
}

uint32_t TextureCounter = 0;
stTexture Textures[MAX_TEXTURE_COUNT];
std::unordered_map<std::string, stTexture*> CachedTextures;

stTexture*
get_texture(
  uint32_t index)
{
  return &Textures[index];
}

stTexture*
get_texture(
  std::string key)
{
  return CachedTextures[key];
}

stTexture
create_texture(
  const stDevice& device,
  VkCommandPool commandPool,
  const char* path,
  stDeletionQueue* deletionQueue)
{
  stTexture* texture = &Textures[TextureCounter];

  *texture = create_texture_image(device, commandPool, path);

  texture->DescriptorSetIndex = TextureCounter;
  CachedTextures[path] = texture;
  TextureCounter++;

  create_texture_image_view(device, *texture);

  texture->Sampler = create_texture_sampler(device, texture->MipLevels);

  if (deletionQueue)
  {
    deletionQueue->PushFunction([=]{
      vkDestroySampler(device.LogicalDevice, texture->Sampler, nullptr);
      vkDestroyImageView(device.LogicalDevice, texture->Image.View, nullptr);
      vkDestroyImage(device.LogicalDevice, texture->Image.Src, nullptr);
      vkFreeMemory(device.LogicalDevice, texture->Image.Memory, nullptr);
    });
  }

  return *texture;
}

stImage
create_depth_resources(
  const stDevice& device,
  VkSampleCountFlagBits msaaSamples,
  VkExtent2D swapChainExtent,
  VkCommandPool commandPool,
  stDeletionQueue* deletionQueue)
{
  VkFormat depthFormat = find_depth_format(device);

  stImage depthImage = create_image(
    device,
    swapChainExtent.width,
    swapChainExtent.height,
    1,
    msaaSamples,
    depthFormat,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );

  create_image_view(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  transition_image_layout(
    device,
    commandPool,
    depthImage.Src,
    depthFormat,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    1
  );

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyImageView(device.LogicalDevice, depthImage.View, nullptr);
    vkDestroyImage(device.LogicalDevice, depthImage.Src, nullptr);
    vkFreeMemory(device.LogicalDevice, depthImage.Memory, nullptr);
  });

  return depthImage;
}

stImage
create_color_resources(
  const stDevice& device,
  VkExtent2D swapChainExtent,
  VkFormat swapchainImageFormat,
  VkSampleCountFlagBits msaaSamples,
  VkCommandPool commandPool,
  stDeletionQueue* deletionQueue)
{
  stImage colorImage = create_image(
    device,
    swapChainExtent.width,
    swapChainExtent.height,
    1,
    msaaSamples,
    swapchainImageFormat,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );

  create_image_view(device, colorImage, swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroyImageView(device.LogicalDevice, colorImage.View, nullptr);
    vkDestroyImage(device.LogicalDevice, colorImage.Src, nullptr);
    vkFreeMemory(device.LogicalDevice, colorImage.Memory, nullptr);
  });

  return colorImage;
}

stBuffer
create_vertex_buffer(
  const stDevice& device,
  VkCommandPool commandPool,
  stMesh& mesh,
  stDeletionQueue* deletionQueue = nullptr)
{
  stBuffer buffer = {};

  VkDeviceSize bufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();

  stBuffer stagingBuffer = {};
  init::create_buffer(
    device,
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, nullptr
  );

  void* data;
  vkMapMemory(device.LogicalDevice, stagingBuffer.Memory, 0, bufferSize, 0, &data);
    memcpy(data, mesh.Vertices.data(), (size_t) bufferSize);
  vkUnmapMemory(device.LogicalDevice, stagingBuffer.Memory);

  init::create_buffer(
    device,
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer,
    deletionQueue
  );

  init::copy_buffer(device, stagingBuffer.Buffer, buffer.Buffer, bufferSize, commandPool);

  vkDestroyBuffer(device.LogicalDevice, stagingBuffer.Buffer, nullptr);
  vkFreeMemory(device.LogicalDevice, stagingBuffer.Memory, nullptr);

  return buffer;
}

stBuffer
create_index_buffer(
  const stDevice& device,
  VkCommandPool commandPool,
  stMesh& mesh,
  stDeletionQueue* deletionQueue = nullptr)
{
  stBuffer buffer = {};

  VkDeviceSize bufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();

  stBuffer stagingBuffer = {};
  init::create_buffer(
    device,
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, nullptr
  );

  void* data;
  vkMapMemory(device.LogicalDevice, stagingBuffer.Memory, 0, bufferSize, 0, &data);
    memcpy(data, mesh.Indices.data(), (size_t) bufferSize);
  vkUnmapMemory(device.LogicalDevice, stagingBuffer.Memory);

  init::create_buffer(
    device,
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, deletionQueue
  );

  init::copy_buffer(device, stagingBuffer.Buffer, buffer.Buffer, bufferSize, commandPool);

  vkDestroyBuffer(device.LogicalDevice, stagingBuffer.Buffer, nullptr);
  vkFreeMemory(device.LogicalDevice, stagingBuffer.Memory, nullptr);

  return buffer;
}

}