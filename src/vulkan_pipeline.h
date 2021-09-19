
struct
stPipeline
{

  VkPipeline
  build_pipeline(
    VkDevice device,
    VkRenderPass pass
  );

  std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
  VkPipelineVertexInputStateCreateInfo VertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo InputAssembly;
  VkViewport Viewport;
	VkRect2D Scissor;
	VkPipelineRasterizationStateCreateInfo Rasterizer;
	VkPipelineColorBlendAttachmentState ColorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo Multisampling;
	VkPipelineLayout PipelineLayout;
};

VkPipeline
stPipeline::build_pipeline(
  VkDevice device,
  VkRenderPass pass)
{
  VkPipeline pipeline = {};

  VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

	viewportState.viewportCount = 1;
	viewportState.pViewports = &Viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &Scissor;

	//setup dummy color blending. We aren't using transparent objects yet
	//the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &ColorBlendAttachment;
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

	pipelineInfo.stageCount = ShaderStages.size();
	pipelineInfo.pStages = ShaderStages.data();
	pipelineInfo.pVertexInputState = &VertexInputInfo;
	pipelineInfo.pInputAssemblyState = &InputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &Rasterizer;
	pipelineInfo.pMultisampleState = &Multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pDynamicState = nullptr; // &dynamicState;
	pipelineInfo.layout = PipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

  return pipeline;
}