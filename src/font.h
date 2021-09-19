

// align enum
enum
enFontAlign : uint8_t
{
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT,
  ALIGN_TOP,
  ALIGN_BOTTOM
};

enum
enColor : uint32_t
{
  COLOR_BLACK = 0x00000000,
  COLOR_WHITE = 0xFFFFFFFF
};

typedef int FontStyleID;
typedef uint32_t tColor;

#define DEFAULT_FONT_SIZE 16

// struct params
struct
stFontParams
{
  float PosX = 0.0f;
  float PosY = 0.0f;
  float Scaling = 1.0f;
  uint32_t Size = DEFAULT_FONT_SIZE;
  float SpacingX = 1.0f;
  float SpacingY = 1.0f;
  enFontAlign HorizontalAlign = ALIGN_LEFT;
  enFontAlign VerticalAlign = ALIGN_TOP;
  tColor Color = COLOR_WHITE;
  tColor ShadowColor = COLOR_BLACK;
  float HorizontalWrap = -1.0f;
  int Style = 0;
};

// functions

namespace
font
{

FontStyleID
AddStyle(const char* fontPath)
{
  FILE* file = fopen(fontPath, "rb");

  assert(file);
  
  fseek(file, 0L, SEEK_END);
  uint32_t fileSize = ftell(file);
  fseek(file, 0L, SEEK_SET);

  static uint8_t buffer[1 * 1024 * 1024];
  
  assert(fileSize > (1 * 1024 * 1024));

  fread(buffer, fileSize, 1, file);
  fclose(file);

  int offset = stbtt_GetFontOffsetForIndex(buffer, 0);

  stbtt_fontinfo fontInfo;
  int ascent, descent, lineGap;

  assert(stbtt_InitFont(&fontInfo, buffer, offset));

  stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

  return 1;
}

}