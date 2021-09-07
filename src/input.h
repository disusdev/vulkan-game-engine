
enum
enKeyAction : int
{
  KEY_NONE = 0,
  KEY_FORWARD = 1 << 0,
  KEY_BACKWARD = 1 << 1,
  KEY_LEFT = 1 << 2,
  KEY_RIGHT = 1 << 3,
  KEY_UP = 1 << 4,
  KEY_DOWN = 1 << 5,
  KEY_Q = 1 << 6,
  KEY_E = 1 << 7,
  KEY_EXIT = 1 << 8,
  KEY_TAB = 1 << 9,
  KEY_SPEED = 1 << 10,
  KEY_LOAD = 1 << 11,
};

struct
stInputState
{
  bool
  GetKey(
    enKeyAction action)
  {
    return KeysHold & action;
  }

  bool
  GetKeyDown(
    enKeyAction action)
  {
    return KeysDown & action;
  }

  bool
  GetKeyUp(
    enKeyAction action)
  {
    return KeysUp & action;
  }

  glm::vec2 CenterPosition;
  glm::vec2 CurrentMousePosition;
  glm::vec2 RotationDelta;

  int KeysHold;
  int KeysPrevHold;
  int KeysDown;
  int KeysUp;

  bool WorkInBackground = false;
};
