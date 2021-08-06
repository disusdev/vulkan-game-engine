
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

void
create_command_buffers(
  const stDevice& device,
  VkCommandPool commandPool,
  const std::initializer_list<std::function<void(VkCommandBuffer, uint32_t)>> darwCommands,
  VkCommandBuffer* commandBuffers = nullptr,
  uint32_t count = 0,
  VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
  VkCommandBufferUsageFlags flags = 0)
{
  VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocInfo.commandPool = commandPool;
  allocInfo.level = level;
  allocInfo.commandBufferCount = count;

  VK_CHECK(vkAllocateCommandBuffers(device.LogicalDevice, &allocInfo, commandBuffers));

  for (uint32_t i = 0; i < count; i++)
  {
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

    for (auto&& command : darwCommands)
    {
      command(commandBuffers[i], i);
    }

    VK_CHECK(vkEndCommandBuffer(commandBuffers[i]));
  }
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

  auto bindingDescription = stVertex::GetBindingDescription();
  auto attributeDescriptions = stVertex::GetAttributeDescriptions();

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

  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

  VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;

  VK_CHECK(vkCreateDescriptorSetLayout(device.LogicalDevice, &layoutInfo, nullptr, &pipeline.DescriptorSet));

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &pipeline.DescriptorSet;
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

  vkDestroyShaderModule(device.LogicalDevice, vertShaderModule, nullptr);
  vkDestroyShaderModule(device.LogicalDevice, fragShaderModule, nullptr);

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
  stBuffer& buffer)
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

bool
load_mesh(
  stMesh& mesh,
  const char* path
)
{
  fastObjMesh* obj = fast_obj_read(path);
	if (!obj)
	{
		printf("Error loading %s: file not found\n", path);
		return false;
	}

	size_t total_indices = 0;

	for (unsigned int i = 0; i < obj->face_count; ++i)
		total_indices += 3 * (obj->face_vertices[i] - 2);

	std::vector<stVertex> vertices(total_indices);

	size_t vertex_offset = 0;
	size_t index_offset = 0;

	for (unsigned int i = 0; i < obj->face_count; ++i)
	{
		for (unsigned int j = 0; j < obj->face_vertices[i]; ++j)
		{
			fastObjIndex gi = obj->indices[index_offset + j];

			stVertex v =
			{
			    {obj->positions[gi.p * 3 + 0],
			    obj->positions[gi.p * 3 + 1],
			    obj->positions[gi.p * 3 + 2]},
			    {obj->normals[gi.n * 3 + 0],
			    obj->normals[gi.n * 3 + 1],
			    obj->normals[gi.n * 3 + 2]},
			    {obj->texcoords[gi.t * 2 + 0],
			    obj->texcoords[gi.t * 2 + 1]},
			};

			// triangulate polygon on the fly; offset-3 is always the first polygon vertex
			if (j >= 3)
			{
				vertices[vertex_offset + 0] = vertices[vertex_offset - 3];
				vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
				vertex_offset += 2;
			}

			vertices[vertex_offset] = v;
			vertex_offset++;
		}

		index_offset += obj->face_vertices[i];
	}

	fast_obj_destroy(obj);

	std::vector<unsigned int> remap(total_indices);

	size_t total_vertices = meshopt_generateVertexRemap(&remap[0], NULL, total_indices, &vertices[0], total_indices, sizeof(stVertex));

	mesh.Indices.resize(total_indices);
	meshopt_remapIndexBuffer(&mesh.Indices[0], NULL, total_indices, &remap[0]);

	mesh.Vertices.resize(total_vertices);
	meshopt_remapVertexBuffer(&mesh.Vertices[0], &vertices[0], total_indices, sizeof(stVertex), &remap[0]);

	return true;
}

void
create_uniform_buffer(
  const stDevice& device,
  stBuffer* buffers,
  uint32_t count
)
{
  VkDeviceSize bufferSize = sizeof(stUniformBufferObject);

  for (size_t i = 0; i < count; i++)
  {
    create_buffer(
      device,
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      buffers[i]
    );
  }
}

void
update_uniform_buffer(
  const stDevice& device,
  stBuffer* buffers,
  double delta,
  uint32_t index,
  VkExtent2D windowSize
)
{
  static float time = 0.0f;
  time += (float) delta * 0.000000001f; // from ns to s

  stUniformBufferObject ubo = {};
  ubo.Model = glm::rotate(glm::mat4(1.0f),time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.View = glm::lookAt(glm::vec3(2.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.Proj = glm::perspective(glm::radians(75.0f), windowSize.width / (float) windowSize.height, 0.1f, 10.0f);
  ubo.Proj[1][1] *= -1;

  void* data;
  vkMapMemory(device.LogicalDevice, buffers[index].Memory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device.LogicalDevice, buffers[index].Memory);
}

VkDescriptorPool
create_descriptor_pool(
  const stDevice& device,
  uint32_t descriptorCount
)
{
  VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
  poolSize.descriptorCount = descriptorCount;

  VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = descriptorCount;

  VkDescriptorPool descriptorPool;

  vkCreateDescriptorPool(device.LogicalDevice, &poolInfo, nullptr, &descriptorPool);

  return descriptorPool;
}

void
create_descriptor_sets(
  const stDevice& device,
  const stPipeline& pipeline,
  VkDescriptorPool descriptorPool,
  VkDescriptorSet* descriptorSets,
  stBuffer* uniformBuffers,
  uint32_t count)
{
  std::vector<VkDescriptorSetLayout> layouts(count, pipeline.DescriptorSet);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = count;
  allocInfo.pSetLayouts = layouts.data();

  vkAllocateDescriptorSets(device.LogicalDevice, &allocInfo, descriptorSets);

  for (size_t i = 0; i < count; i++)
  {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i].Buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(stUniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;

    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;

    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(device.LogicalDevice, 1, &descriptorWrite, 0, nullptr);
  }
}

stImage
create_image(
  const stDevice& device,
  uint32_t width,
  uint32_t height,
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
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional
  
  VK_CHECK(vkCreateImage(device.LogicalDevice, &imageInfo, nullptr, &image.Image));

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device.LogicalDevice, image.Image, &memRequirements);
  
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  
  VK_CHECK(vkAllocateMemory(device.LogicalDevice, &allocInfo, nullptr, &image.Memory));
  
  vkBindImageMemory(device.LogicalDevice, image.Image, image.Memory, 0);

  return image;
}

void
transition_image_layout(
  const stDevice& device,
  VkCommandPool commandPool,
  VkImage image,
  VkFormat format,
  VkImageLayout oldLayout,
  VkImageLayout newLayout
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
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
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
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
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

stImage
create_texture_image(
  const stDevice& device,
  VkCommandPool commandPool
)
{
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load("./data/models/cube/default.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  assert(pixels);

  stBuffer staging;

  create_buffer(
    device,
    imageSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    staging
  ); 

  void* data;
  vkMapMemory(device.LogicalDevice, staging.Memory, 0, imageSize, 0, &data);
      memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device.LogicalDevice, staging.Memory);

  stbi_image_free(pixels);

  stImage image = create_image(
    device,
    texWidth,
    texHeight,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  );

  transition_image_layout(device, commandPool, image.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy_buffer_to_image(device, commandPool, staging.Buffer, image.Image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

  transition_image_layout(device, commandPool, image.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device.LogicalDevice, staging.Buffer, nullptr);
  vkFreeMemory(device.LogicalDevice, staging.Memory, nullptr);

  return image;
}

}