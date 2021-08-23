
struct
stVertex
{
	alignas(16) glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	alignas(16) glm::vec3 Normal = { 0.0f, 0.0f, 0.0f };
	alignas(16) glm::vec2 TexCoord = { 0.0f, 0.0f };
};

struct
stMesh
{
  bool LoadMesh(const char* path)
  {
    fastObjMesh* obj = fast_obj_read(path);
	  if (!obj)
	  {
	  	printf("Error loading %s: file not found\n", path);
	  	return false;
	  }

    Vertices.clear();
    Indices.clear();

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

	  Indices.resize(total_indices);
	  meshopt_remapIndexBuffer(&Indices[0], NULL, total_indices, &remap[0]);

	  Vertices.resize(total_vertices);
	  meshopt_remapVertexBuffer(&Vertices[0], &vertices[0], total_indices, sizeof(stVertex), &remap[0]);

	  return true;
  }

  std::vector<stVertex> Vertices;
  std::vector<uint32_t> Indices;
};