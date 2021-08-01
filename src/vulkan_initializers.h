
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

stDevice
create_device(
  VkPhysicalDevice physicalDevice,
  VkSurfaceKHR surface
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

VkSwapchainKHR
create_swapchain(
  const stDevice& device,
  VkSurfaceKHR surface,
  VkFormat& swapChainImageFormat,
  VkExtent2D& swapChainExtent,
  VkImage* swapchainImages,
  VkImageView* swapchainImageViews,
  uint32_t& swapchainImageCount,
  VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  VkPresentModeKHR mode = VK_PRESENT_MODE_MAILBOX_KHR)
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

  VK_CHECK(vkGetSwapchainImagesKHR(device.LogicalDevice, swapchain, &swapchainImageCount, swapchainImages));

  VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = swapChainImageFormat;
  viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCreateInfo.subresourceRange.baseMipLevel = 0;
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.subresourceRange.baseArrayLayer = 0;
  viewCreateInfo.subresourceRange.layerCount = 1;

  for (uint32_t i = 0; i < swapchainImageCount; i++)
  {
    viewCreateInfo.image = swapchainImages[i];
    VK_CHECK(vkCreateImageView(device.LogicalDevice, &viewCreateInfo, nullptr, &swapchainImageViews[i]));
  }

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
  const stDevice& device,
  VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
{
  VkCommandPool pool;

  VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  createInfo.flags = flags;
  createInfo.queueFamilyIndex = device.Queues[QUEUE_TYPE_GRAPHICS].Index;

  VK_CHECK(vkCreateCommandPool(device.LogicalDevice, &createInfo, nullptr, &pool));

  return pool;
}

VkRenderPass
create_render_pass(
  const stDevice& device,
  VkFormat renderFormat)
{
  VkRenderPass renderPass = VK_NULL_HANDLE;

  VkAttachmentDescription color_attachment = {};
	color_attachment.format = renderFormat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;// VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	// subpass.pDepthStencilAttachment = &depth_attachment_ref;

	//1 dependency, which is from "outside" into the subpass. And we can read or write color
	//VkSubpassDependency dependency = {};
	//dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	//dependency.dstSubpass = 0;
	//dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//dependency.srcAccessMask = 0;
	//dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  //array of 2 attachments, one for the color, and other for depth
	VkAttachmentDescription attachments[1] = { color_attachment };

	VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	render_pass_info.attachmentCount = ArrayCount(attachments);
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	//render_pass_info.dependencyCount = 1;
	//render_pass_info.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(device.LogicalDevice, &render_pass_info, nullptr, &renderPass));

  return renderPass;
}

VkFramebuffer
create_framebuffer(
  const stDevice& device,
  VkRenderPass renderPass,
  VkExtent2D bufferSize,
  VkImageView atachment
  )
{
  VkFramebuffer framebuffer = VK_NULL_HANDLE;

  VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
  createInfo.renderPass = renderPass;
  createInfo.width = bufferSize.width;
  createInfo.height = bufferSize.height;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments = &atachment;
  createInfo.layers = 1;

  VK_CHECK(vkCreateFramebuffer(device.LogicalDevice, &createInfo, nullptr, &framebuffer));

  return framebuffer;
}

VkFence
create_fence(
  const stDevice& device)
{
  VkFence fence = VK_NULL_HANDLE;

  VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
  createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VK_CHECK(vkCreateFence(device.LogicalDevice, &createInfo, nullptr, &fence));

  return fence;
}

VkCommandBuffer
create_command_buffer(
  const stDevice& device,
  VkCommandPool commandPool,
  VkRenderPass renderPass,
  VkFramebuffer framebuffer,
  VkExtent2D windowSize,
  const stPipeline& pipeline)
{
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(device.LogicalDevice, &allocInfo, &commandBuffer));

  VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  VkClearColorValue color = { 0.3f, 0.3f, 0.3f, 1.0f };
  VkClearValue clearValue = { color };

  VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.framebuffer = framebuffer;
  renderPassBeginInfo.renderArea.offset = { 0, 0 };
  renderPassBeginInfo.renderArea.extent = windowSize;
  renderPassBeginInfo.clearValueCount = 1;
  renderPassBeginInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  // draw call
  {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Pipeline);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  }

  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  return commandBuffer;
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

stPipeline
create_pipeline(
  const stDevice& device,
  VkExtent2D windowExtend,
  VkRenderPass renderPass)
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

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
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

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  VK_CHECK(vkCreatePipelineLayout(device.LogicalDevice, &pipelineLayoutInfo, nullptr, &pipeline.Layout));

  VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  pipelineInfo.stageCount = ArrayCount(shaderStages);
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = pipeline.Layout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  VK_CHECK(vkCreateGraphicsPipelines(device.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.Pipeline));

  vkDestroyShaderModule(device.LogicalDevice, fragShaderModule, nullptr);
  vkDestroyShaderModule(device.LogicalDevice, vertShaderModule, nullptr);

  return pipeline;
}

}