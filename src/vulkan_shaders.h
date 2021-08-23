
#include <spirv_reflect.c>

struct stShaderModule
{
  static const uint32_t BufferLength = 4 * 256 * 256;
  uint32_t Code[BufferLength];
  uint32_t Length;
  VkShaderModule Module;
};

struct
stShaderEffect
{
  struct
  stReflectionOverrides
  {
    const char* Name;
    VkDescriptorType Type;
  };

  struct
  stDescriptionSetLayoutData
  {
    uint32_t SetNumber;
    VkDescriptorSetLayoutCreateInfo CreateInfo;
    std::vector<VkDescriptorSetLayoutBinding> Bindings;
  };

  void
  add_stage(
    stShaderModule& shaderModule,
    VkShaderStageFlagBits stage);

  void
  reflect_layout(
    const stDevice& device,
    stReflectionOverrides* overrides,
    int32_t overrideCount);

  void
  fill_stages(
    std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages);

  VkPipelineLayout BuiltLayout;

  struct
  stReflectedBinding
  {
    uint32_t Set;
    uint32_t Binding;
    VkDescriptorType Type;
  };

  std::unordered_map<std::string, stReflectedBinding> Bindings;
  VkDescriptorSetLayout SetLayouts[4];
  uint32_t SetHashes[4];

  struct
  stShaderStage
  {
    stShaderModule* Module;
    VkShaderStageFlagBits Stage;
  };

  std::vector<stShaderStage> Stages;
};

void
stShaderEffect::add_stage(
  stShaderModule& shaderModule,
  VkShaderStageFlagBits stage)
{
  stShaderStage newStage = { &shaderModule, stage };
  Stages.push_back(newStage);
}

constexpr uint32_t
fnv1a_32(const char* s, size_t count)
{
  return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
}

void
stShaderEffect::reflect_layout(
  const stDevice& device,
  stReflectionOverrides* overrides,
  int32_t overrideCount)
{
  std::vector<stDescriptionSetLayoutData> set_layouts;
  std::vector<VkPushConstantRange> constant_ranges;

  for (auto &s : Stages)
  {
    SpvReflectShaderModule spvmodule;
    SpvReflectResult result = spvReflectCreateShaderModule(s.Module->Length * sizeof(uint32_t), s.Module->Code, &spvmodule);

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet *> sets(count);
    result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (size_t i_set = 0; i_set < sets.size(); ++i_set)
    {
      const SpvReflectDescriptorSet &refl_set = *(sets[i_set]);

      stDescriptionSetLayoutData layout = {};

      layout.Bindings.resize(refl_set.binding_count);
      for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding)
      {
        const SpvReflectDescriptorBinding &refl_binding = *(refl_set.bindings[i_binding]);
        VkDescriptorSetLayoutBinding &layout_binding = layout.Bindings[i_binding];
        layout_binding.binding = refl_binding.binding;
        layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);

        for (int ov = 0; ov < overrideCount; ov++)
        {
          if (strcmp(refl_binding.name, overrides[ov].Name) == 0)
          {
            layout_binding.descriptorType = overrides[ov].Type;
          }
        }

        layout_binding.descriptorCount = 1;
        for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
        {
          layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
        }
        layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(spvmodule.shader_stage);

        stReflectedBinding reflected;
        reflected.Binding = layout_binding.binding;
        reflected.Set = refl_set.set;
        reflected.Type = layout_binding.descriptorType;

        Bindings[refl_binding.name] = reflected;
      }
      layout.SetNumber = refl_set.set;
      layout.CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layout.CreateInfo.bindingCount = refl_set.binding_count;
      layout.CreateInfo.pBindings = layout.Bindings.data();

      set_layouts.push_back(layout);
    }

    //pushconstants

    result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectBlockVariable *> pconstants(count);
    result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, pconstants.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    if (count > 0)
    {
      VkPushConstantRange pcs{};
      pcs.offset = pconstants[0]->offset;
      pcs.size = pconstants[0]->size;
      pcs.stageFlags = s.Stage;

      constant_ranges.push_back(pcs);
    }
  }

  std::array<stDescriptionSetLayoutData, 4> merged_layouts;

  for (uint32_t i = 0; i < 4; i++)
  {
    stDescriptionSetLayoutData& ly = merged_layouts[i];

    ly.SetNumber = i;

    ly.CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    std::unordered_map<int, VkDescriptorSetLayoutBinding> binds;

    for (auto& s : set_layouts)
    {
      if (s.SetNumber == i)
      {
        for (auto& b : s.Bindings)
        {
          auto it = binds.find(b.binding);
          if (it == binds.end())
          {
            binds[b.binding] = b;
            //ly.Bindings.push_back(b);
          }
          else {
            //merge flags
            binds[b.binding].stageFlags |= b.stageFlags;
          }

        }
      }
    }
    for (auto [k, v] : binds)
    {
      ly.Bindings.push_back(v);
    }
    //sort the bindings, for hash purposes
    std::sort(ly.Bindings.begin(), ly.Bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
    {
      return a.binding < b.binding;
    });


    ly.CreateInfo.bindingCount = (uint32_t)ly.Bindings.size();
    ly.CreateInfo.pBindings = ly.Bindings.data();
    ly.CreateInfo.flags = 0;
    ly.CreateInfo.pNext = 0;


    if (ly.CreateInfo.bindingCount > 0)
    {
      auto hash_desription_layout_info = [](VkDescriptorSetLayoutCreateInfo* info)
      {
        std::stringstream ss;
        ss << info->flags;
        ss << info->bindingCount;

        for (auto i = 0u; i < info->bindingCount; i++)
        {
          const VkDescriptorSetLayoutBinding& binding = info->pBindings[i];

          ss << binding.binding;
          ss << binding.descriptorCount;
          ss << binding.descriptorType;
          ss << binding.stageFlags;
        }

        auto str = ss.str();

        return fnv1a_32(str.c_str(), str.length());
      };
      SetHashes[i] = hash_desription_layout_info(&ly.CreateInfo);
      vkCreateDescriptorSetLayout(device.LogicalDevice, &ly.CreateInfo, nullptr, &SetLayouts[i]);
    }
    else
    {
      SetHashes[i] = 0;
      SetLayouts[i] = VK_NULL_HANDLE;
    }
  }

  //we start from just the default empty pipeline layout info
  VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = init::pipeline_layout_create_info();

  mesh_pipeline_layout_info.pPushConstantRanges = constant_ranges.data();
  mesh_pipeline_layout_info.pushConstantRangeCount = (uint32_t)constant_ranges.size();

  std::array<VkDescriptorSetLayout, 4> compactedLayouts;
  int s = 0;
  for (int i = 0; i < 4; i++)
  {
    if (SetLayouts[i] != VK_NULL_HANDLE)
    {
      compactedLayouts[s] = SetLayouts[i];
      s++;
    }
  }

  mesh_pipeline_layout_info.setLayoutCount = s;
  mesh_pipeline_layout_info.pSetLayouts = compactedLayouts.data();


  vkCreatePipelineLayout(device.LogicalDevice, &mesh_pipeline_layout_info, nullptr, &BuiltLayout);
}

void
stShaderEffect::fill_stages(
  std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages)
{
  for (auto& s : Stages)
  {
    pipelineStages.push_back(init::pipeline_shader_stage_create_info(s.Stage, s.Module->Module));
  }
}