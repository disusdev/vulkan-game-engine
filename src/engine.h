
const static f64 FIXED_TIME = 1.0 / 60.0;

#include "font.h"
#include "mesh.h"
#include "transform.h"
#include "physics.h"
#include "entity.h"
#include "player.h"
#include "scene.h"
#include "vulkan_renderer.h"

struct
engine
{
  // renderer
  stWindow Window = {};

  stInputState Input;

  stRenderer Renderer;

  stCamera SceneCamera;
  stSun Sun;

  stPlayer Player;

  f64 AwakeTime = 0.0;
  bool IsRunning = false;

  stEntitySystem EntitySystem;
  stTransformSystem TransformSystem;
  stPhysicsSystem PhysicsSystem;

  bool
  Run()
  {
    IsRunning = sys::WindowCreate(L"VulkanEngine", &Window);

    AwakeTime = sys::GetTime();

    f64 timer = 0.0;
    f64 current = AwakeTime;
    f64 accumulator = 0.0;
    f64 fresh = 0.0;
    f64 delta = 0.0;

    PhysicsSystem.TransformSystem = &TransformSystem;

    // "./data/models/pirate/pirate.obj"
    // "./data/models/sponza/sponza.obj"
    // "./data/models/mandalorian/mandalorian.obj"
    // "./data/models/the_nobel/the_nobel.obj"
    // "./data/models/bunny/bunny.obj"
    // "./data/models/cube/cube.obj"

    mesh::load_mesh("./data/models/cube/cube.obj");
    int startIndex, mshCount;
    //mesh::load_gltf_mesh("./data/models/shiba/scene.gltf", startIndex, mshCount);
    mesh::load_gltf_mesh("./data/models/lost-empire/loast-empire.gltf", startIndex, mshCount);
    //mesh::load_gltf_mesh("./data/models/Sponza/Sponza.gltf", startIndex, mshCount);
    mesh::load_gltf_mesh("./data/models/hairball/hairball.gltf", startIndex, mshCount);
    //mesh::load_gltf_mesh("./data/models/box/BoxVertexColors.gltf", startIndex, mshCount);
    //mesh::load_gltf_mesh("./data/models/sun/sun.gltf", startIndex, mshCount);
    // mesh::load_gltf_mesh("./data/models/cube/cube.gltf", startIndex, mshCount);

    // assert(mesh::test(mesh::get_mesh("./data/models/cube/cube.obj"), mesh::get_mesh(1)));

    //std::vector<stEntityBase> bases;
    //bases.resize(100000);
    //for (size_t x = 0; x < 100; x++)
    //{
    //  for (size_t y = 0; y < 100; y++)
    //  {
    //    for (size_t z = 0; z < 10; z++)
    //    {
    //      bases[x * 1000 + y * 10 + z] = enity::create_entity(EntitySystem, TransformSystem, glm::vec3(x * 2, y * 2,z * 2), "./data/models/shiba/scene.gltf");
    //    }
    //  }
    //}

    //stEntityBase base_entities[] =
    //{
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(1.0f,0.0f,0.0f), "./data/models/cube/cube.obj"),
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(0.0f,100.0f,0.0f), "./data/models/shiba/scene.gltf"),
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(0.0f,0.0f,0.0f), "./data/models/lost-empire/loast-empire.gltf"),
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(0.0f,0.0f,0.0f), "./data/models/Sponza/Sponza.gltf"),
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(0.0f,0.0f,0.0f), "./data/models/hairball/hairball.gltf"),
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(0.0f,0.0f,0.0f), "./data/models/sun/sun.gltf"),
      //enity::create_entity(EntitySystem, TransformSystem, glm::vec3(0.0f,0.0f,0.0f), "./data/models/box/BoxVertexColors.gltf"),
    //};

    //*base_entities[0].Entity->Transform.Tramsform = glm::scale(*base_entities[0].Entity->Transform.Tramsform, glm::vec3(100.0f, 100.0f, 100.0f));
    

    Renderer.Camera = &SceneCamera;
    Renderer.Sun = &Sun;
    Renderer.Init(Window);

    stScene scene;
    scene.Load(EntitySystem, TransformSystem);

    while (IsRunning)
    {
      sys::GetMessages();

      sys::UpdateInput();

      if (Input.GetKeyDown(KEY_EXIT))
      {
        IsRunning = false;
      }

      if (Input.GetKeyDown(KEY_LOAD))
      {
        Renderer.AddRenderingObjectsFromEntities(scene);
      }

      if (!IsRunning) break;

      fresh = sys::GetTime();

      delta = fresh - current;

      current = fresh;

      accumulator += delta;

      // update()
      Player.Update(Input, delta);
      SceneCamera.update_camera(Input, delta);
      Sun.Update((float)delta, Input);

      while (accumulator >= FIXED_TIME)
      {
        // physics update
        // PhysicsSystem.Update((float)FIXED_TIME);

        accumulator -= FIXED_TIME;
        timer += FIXED_TIME;
      }

      Renderer.Render(delta);

      sys::SwapBuffers(Window);
    }

    Renderer.Term();

    return 0;
  }

  void
  Resize()
  {
    Renderer.RecreateSwapchain();
  }

};
