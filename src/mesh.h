
struct
stVertex
{
	alignas(16) glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	alignas(16) glm::vec3 Normal = { 0.0f, 0.0f, 0.0f };
  alignas(16) glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
	alignas(16) glm::vec2 TexCoord = { 0.0f, 0.0f };
};

struct
stMesh
{
  std::vector<stVertex> Vertices;
  std::vector<uint32_t> Indices;
  std::string TexturePath;
  glm::mat4 RootMatrix = glm::mat4(1.0f);
};

static void fixupIndices(std::vector<unsigned int>& indices, cgltf_primitive_type& type)
{
	if (type == cgltf_primitive_type_line_loop)
	{
		std::vector<unsigned int> result;
		result.reserve(indices.size() * 2 + 2);

		for (size_t i = 1; i <= indices.size(); ++i)
		{
			result.push_back(indices[i - 1]);
			result.push_back(indices[i % indices.size()]);
		}

		indices.swap(result);
		type = cgltf_primitive_type_lines;
	}
	else if (type == cgltf_primitive_type_line_strip)
	{
		std::vector<unsigned int> result;
		result.reserve(indices.size() * 2);

		for (size_t i = 1; i < indices.size(); ++i)
		{
			result.push_back(indices[i - 1]);
			result.push_back(indices[i]);
		}

		indices.swap(result);
		type = cgltf_primitive_type_lines;
	}
	else if (type == cgltf_primitive_type_triangle_strip)
	{
		std::vector<unsigned int> result;
		result.reserve(indices.size() * 3);

		for (size_t i = 2; i < indices.size(); ++i)
		{
			int flip = i & 1;

			result.push_back(indices[i - 2 + flip]);
			result.push_back(indices[i - 1 - flip]);
			result.push_back(indices[i]);
		}

		indices.swap(result);
		type = cgltf_primitive_type_triangles;
	}
	else if (type == cgltf_primitive_type_triangle_fan)
	{
		std::vector<unsigned int> result;
		result.reserve(indices.size() * 3);

		for (size_t i = 2; i < indices.size(); ++i)
		{
			result.push_back(indices[0]);
			result.push_back(indices[i - 1]);
			result.push_back(indices[i]);
		}

		indices.swap(result);
		type = cgltf_primitive_type_triangles;
	}
	else if (type == cgltf_primitive_type_lines)
	{
		// glTF files don't require that line index count is divisible by 2, but it is obviously critical for scenes to render
		indices.resize(indices.size() / 2 * 2);
	}
	else if (type == cgltf_primitive_type_triangles)
	{
		// glTF files don't require that triangle index count is divisible by 3, but it is obviously critical for scenes to render
		indices.resize(indices.size() / 3 * 3);
	}
}

//const uint8_t* cgltf_buffer_view_data(const cgltf_buffer_view* view)
//{
//	if (view->data)
//		return (const uint8_t*)view->data;
//
//	if (!view->buffer->data)
//		return NULL;
//
//	const uint8_t* result = (const uint8_t*)view->buffer->data;
//	result += view->offset;
//	return result;
//}

namespace mesh
{

std::unordered_map<std::string, stMesh*> CachedMeshes;
stMesh Meshes[MAX_MESH_COUNT];
// stMesh *Meshes = (stMesh*) calloc(MAX_MESH_COUNT, sizeof(stMesh));

uint32_t MesheCounter = 0;

bool load_gltf_mesh(const char* path, int& startIndex, int& meshCount)
{
	// stMesh* mesh = &Meshes[MesheCounter++];

  std::string mesh_path = path;
  mesh_path = mesh_path.substr(0, mesh_path.find_last_of("\\/") + 1);

	std::vector<std::pair<size_t, size_t> > mesh_remap;

  cgltf_options options = { 0 };
  cgltf_data* data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);

  result = (result == cgltf_result_success) ? cgltf_load_buffers(&options, data, path) : result;
	result = (result == cgltf_result_success) ? cgltf_validate(data) : result;
  assert(result == cgltf_result_success);
  
  //#####################################################################
  //#####################################################################

  size_t total_indices = 0;

  //

	size_t total_primitives = 0;

	for (size_t mi = 0; mi < data->meshes_count; ++mi)
		total_primitives += data->meshes[mi].primitives_count;

//   stMesh* meshes = &Meshes[MesheCounter];
  startIndex = MesheCounter;
  MesheCounter += total_primitives;
  meshCount = total_primitives;

  //meshes->Vertices.clear();
  //meshes->Indices.clear();

	//meshes.reserve(total_primitives);
	// mesh_remap.resize(data->meshes_count);

  
  glm::mat4 RootMatrix = glm::mat4{ 1 };//glm::rotate(glm::mat4{ 1 }, 90.0f * 0.01745329251f, { 1.0f,0.0f,0.0f });
  //for (size_t i = 0; i < data->nodes_count; i++)
  //{
  //  glm::mat4 mat;
  //  for (size_t nx = 0; nx < 4; nx++)
  //  {
  //    for (size_t ny = 0; ny < 4; ny++)
  //    {
  //      mat[nx][ny] = data->nodes[i].matrix[nx * 4 + ny];
  //    }
  //  }

  //  RootMatrix *= mat;
  //}

  int index = 0;
	for (size_t mi = 0; mi < data->meshes_count; ++mi)
	{
		const cgltf_mesh& mesh = data->meshes[mi];

		// size_t remap_offset = total_primitives;

		for (size_t pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& primitive = mesh.primitives[pi];

			if (primitive.type == cgltf_primitive_type_points && primitive.indices)
			{
				fprintf(stderr, "Warning: ignoring primitive %d of mesh %d because indexed points are not supported\n", int(pi), int(mi));
				continue;
			}

			//meshes.push_back(Mesh());
			//Mesh& result = meshes.back();

      stMesh* result_mesh = &Meshes[startIndex + index++];

      result_mesh->RootMatrix = RootMatrix;      

			// result.scene = -1;

			// result.material = primitive.material;

			// result.type = primitive.type;

			// result.streams.reserve(primitive.attributes_count);

			if (primitive.indices)
			{
				result_mesh->Indices.resize(primitive.indices->count);
				for (size_t i = 0; i < primitive.indices->count; ++i)
					result_mesh->Indices[i] = unsigned(cgltf_accessor_read_index(primitive.indices, i));
			}
			else if (primitive.type != cgltf_primitive_type_points)
			{
				size_t count = primitive.attributes ? primitive.attributes[0].data->count : 0;

				// note, while we could generate a good index buffer, reindexMesh will take care of this
				result_mesh->Indices.resize(count);
				for (size_t i = 0; i < count; ++i)
					result_mesh->Indices[i] = unsigned(i);
			}

      cgltf_primitive_type type = primitive.type;

			fixupIndices(result_mesh->Indices, type);

			for (size_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const cgltf_attribute& attr = primitive.attributes[ai];

				if (attr.type == cgltf_attribute_type_invalid)
				{
					fprintf(stderr, "Warning: ignoring unknown attribute %s in primitive %d of mesh %d\n", attr.name, int(pi), int(mi));
					continue;
				}

        if (attr.type == cgltf_attribute_type_normal 
          || attr.type == cgltf_attribute_type_position 
          || attr.type == cgltf_attribute_type_texcoord
          || attr.type == cgltf_attribute_type_color)
        {
          if (result_mesh->Vertices.empty())
            result_mesh->Vertices.resize(attr.data->count);
        }

        if (attr.type == cgltf_attribute_type_position)
        {
          std::vector<cgltf_float> data_u;
	        data_u.resize(attr.data->count * 3);
	        cgltf_accessor_unpack_floats(attr.data, data_u.data(), data_u.size());

          for (size_t v = 0; v < attr.data->count; v++)
          {
            stVertex* vrt = &result_mesh->Vertices[v];

            vrt->Position = glm::vec3(data_u[v * 3 + 0], data_u[v * 3 + 1], data_u[v * 3 + 2]); // v * 3 + 1
          }
        }

        if (attr.type == cgltf_attribute_type_color)
        {
          std::vector<cgltf_float> data_u;
	        data_u.resize(attr.data->count * 3);
	        cgltf_accessor_unpack_floats(attr.data, data_u.data(), data_u.size());

          for (size_t v = 0; v < attr.data->count; v++)
          {
            stVertex* vrt = &result_mesh->Vertices[v];

            vrt->Color = glm::vec3(data_u[v * 3 + 0], data_u[v * 3 + 1], data_u[v * 3 + 2]); // v * 3 + 1
          }
        }

        if (attr.type == cgltf_attribute_type_normal)
        {
          std::vector<cgltf_float> data_u;
	        data_u.resize(attr.data->count * 3);
	        cgltf_accessor_unpack_floats(attr.data, data_u.data(), data_u.size());
          //result.Vertices.resize(attr.data->count);

          for (size_t v = 0; v < attr.data->count; v++)
          {
            stVertex* vrt = &result_mesh->Vertices[v];

            vrt->Normal = glm::vec3(data_u[v * 3 + 0], data_u[v * 3 + 1], data_u[v * 3 + 2]);
          }
        }

        if (attr.type == cgltf_attribute_type_texcoord)
        {
          std::vector<cgltf_float> data_u;
	        data_u.resize(attr.data->count * 2);
	        cgltf_accessor_unpack_floats(attr.data, data_u.data(), data_u.size());
          //result.Vertices.resize(attr.data->count);

          for (size_t v = 0; v < attr.data->count; v++)
          {
            stVertex* vrt = &result_mesh->Vertices[v];

            vrt->TexCoord = glm::vec2(data_u[v * 2 + 0], data_u[v * 2 + 1]);
          }
        }

        //if (attr.type == cgltf_attribute_type_color)
        //{
        //  std::vector<cgltf_float> data_u;
	       // data_u.resize(attr.data->count);
	       // cgltf_accessor_unpack_floats(attr.data, data_u.data(), data_u.size());
        //  //result.Vertices.resize(attr.data->count);

        //  for (size_t v = 0; v < attr.data->count; v++)
        //  {
        //    stVertex& vrt = result.Vertices.data[v];

        //    vrt. = glm::vec3(data_u[0], data_u[1], data_u[2]);
        //  }
        //}

				// result.streams.push_back(Stream());
				// Stream& s = result.streams.back();

				// s.type = attr.type;
				// s.index = attr.index;

				// readAccessor(s.data, attr.data);

				//if (attr.type == cgltf_attribute_type_color && attr.data->type == cgltf_type_vec3)
				//{
				//	for (size_t i = 0; i < s.data.size(); ++i)
				//		s.data[i].f[3] = 1.0f;
				//}
			}

			for (size_t ti = 0; ti < primitive.targets_count; ++ti)
			{
				const cgltf_morph_target& target = primitive.targets[ti];

				for (size_t ai = 0; ai < target.attributes_count; ++ai)
				{
					const cgltf_attribute& attr = target.attributes[ai];

					if (attr.type == cgltf_attribute_type_invalid)
					{
						fprintf(stderr, "Warning: ignoring unknown attribute %s in morph target %d of primitive %d of mesh %d\n", attr.name, int(ti), int(pi), int(mi));
						continue;
					}

          if (attr.type == cgltf_attribute_type_position)
          {
            // attr.data->buffer_view
            //const uint8_t* data = cgltf_buffer_view_data(attr.data->buffer_view);

            //glm::vec3 vector = *(glm::vec3*)(data);

            // printf("May prem count: %d\n", attr.data->count);
          }

					//result.streams.push_back(Stream());
					//Stream& s = result.streams.back();

					//s.type = attr.type;
					//s.index = attr.index;
					//s.target = int(ti + 1);

					//readAccessor(s.data, attr.data);
				}
			}

			//result.targets = primitive.targets_count;
			//result.target_weights.assign(mesh.weights, mesh.weights + mesh.weights_count);
			//result.target_names.assign(mesh.target_names, mesh.target_names + mesh.target_names_count);

			//result.variants.assign(primitive.mappings, primitive.mappings + primitive.mappings_count);

      std::string meshName = path + std::to_string(mi) + "_" + std::to_string(pi);

      //data->materials[primitive.material->pbr_metallic_roughness.base_color_texture.texture->name].pbr_metallic_roughness.base_color_texture.texture->name

      if (primitive.material->pbr_metallic_roughness.base_color_texture.texture)
      result_mesh->TexturePath = mesh_path + primitive.material->pbr_metallic_roughness.base_color_texture.texture->image->uri; //data->images->uri;

      //primitive.material->pbr_metallic_roughness.base_color_texture.texture->image->uri

      CachedMeshes.insert( { meshName , result_mesh } );
		}

		// mesh_remap[mi] = std::make_pair(remap_offset, meshes.size());
	}

  //#####################################################################
  //#####################################################################

  cgltf_free(data);
	return true;
}

bool load_mesh(const char* path)
{
  stMesh* mesh = &Meshes[MesheCounter++];
  fastObjMesh* obj = fast_obj_read(path);
  if (!obj)
  {
  	printf("Error loading %s: file not found\n", path);
  	return false;
  }

  mesh->Vertices.clear();
  mesh->Indices.clear();

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
          {1.0f,1.0f,1.0f},
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

  mesh->Indices.resize(total_indices);
  meshopt_remapIndexBuffer(&mesh->Indices[0], NULL, total_indices, &remap[0]);

  mesh->Vertices.resize(total_vertices);
  meshopt_remapVertexBuffer(&mesh->Vertices[0], &vertices[0], total_indices, sizeof(stVertex), &remap[0]);

  CachedMeshes.insert( { path, mesh } );

  return true;
}

bool
test(
  stMesh* mesh1,
  stMesh* mesh2)
{
  if ((mesh1->Indices.size() != mesh2->Indices.size())
      || (mesh1->Vertices.size() != mesh2->Vertices.size()))
  {
    return false;
  }
  
  for (size_t i = 0; i < mesh1->Vertices.size(); i++)
  {
    if (mesh1->Vertices[i].Position != mesh2->Vertices[i].Position) return false;
    // if (mesh1->Vertices[i].Normal != mesh2->Vertices[i].Normal) return false;
    // if (mesh1->Vertices[i].TexCoord != mesh2->Vertices[i].TexCoord) return false;
  }

  return true;
}

stMesh*
get_mesh(
  const char* name)
{
  return CachedMeshes[name];
}

std::vector<stMesh*>
get_meshes(
  const char* name)
{
  std::vector<stMesh*> meshes;
  for (auto&& mesh : CachedMeshes)
  {
    if (mesh.first.find(name) != std::string::npos)
    {
      meshes.push_back(mesh.second);
    }
  }
  return meshes;
}

stMesh*
get_mesh(
  int index)
{
  return &Meshes[index];
}

}