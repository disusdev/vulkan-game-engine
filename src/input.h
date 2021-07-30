
enum
enKeyAction : int
{
  KEY_NONE = 0,
  KEY_FORWARD = 1 << 0,
  KEY_BACKWARD = 1 << 1,
  KEY_LEFT = 1 << 2,
  KEY_RIGHT = 1 << 3,
  KEY_UP = 1 << 4,
  KEY_DOWN = 1 << 5
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

  int KeysHold;
  int KeysPrevHold;
  int KeysDown;
  int KeysUp;
};
