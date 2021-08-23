
const static f64 FIXED_TIME = 1.0 / 60.0;

#include "mesh.h"
#include "transform.h"
#include "physics.h"
#include "entity.h"
#include "vulkan_renderer.h"

struct
engine
{
  // renderer
  stWindow Window = {};
  stInputState Input;
  stRenderer Renderer;
  stCamera Camera;
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

    stEntityBase Entity1 = enity::create_entity(EntitySystem, TransformSystem, glm::vec3(1.0f,0.0f,0.0f), "./data/models/cube/cube.obj");
    stEntityBase Entity2 = enity::create_entity(EntitySystem, TransformSystem, glm::vec3(-1.0f,0.0f,0.0f), "./data/models/cube/cube.obj");

    Renderer.Camera = &Camera;
    Renderer.Init(Window);

    stEntity* entities[] =
    {
      Entity1.Entity,
      Entity2.Entity
    };

    Renderer.AddRenderingObjectsFromEntities(entities, ArrayCount(entities));

    while (IsRunning)
    {
      sys::GetMessages();

      sys::UpdateInput();

      if (Input.GetKeyDown(KEY_EXIT))
      {
        IsRunning = false;
      }

      if (!IsRunning) break;

      fresh = sys::GetTime();

      delta = fresh - current;

      current = fresh;

      accumulator += delta;

      // update()
      Camera.update_camera(Input, delta);

      while (accumulator >= FIXED_TIME)
      {
        // physics update
        PhysicsSystem.Update((float)FIXED_TIME);

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
