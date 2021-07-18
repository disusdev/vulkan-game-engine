
const static f64 FIXED_TIME = (1.0 / 60.0) * 1000000;

struct
engine
{
  // renderer
  window Window = {};
  input_state Input;
  double AwakeTime = 0.0;
  bool IsRunning = false;

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

    while (IsRunning)
    {
      sys::GetMessages();

      sys::UpdateInput();

      if (!IsRunning) break;

      fresh = sys::GetTime();

      delta = fresh - current;

      current = fresh;

      accumulator += delta;

      // update()

      while (accumulator >= FIXED_TIME)
      {
        // physics update
        accumulator -= FIXED_TIME;
        timer += FIXED_TIME;
      }

      // render()

      sys::SwapBuffers(Window);
    }

    return 0;
  }

};