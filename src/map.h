
#define SIZE_OF_MAP 1024

struct
stMap
{
  struct stKey key;
  struct stValue value;
};

struct
stKey
{
  struct point {int x, y;}p1;
  struct point {int x, y;}p2;
  int i;
  int j;
};

struct
stValue
{
  int Val;
};

struct stMap map[SIZE_OF_MAP];

int get_hash(stKey *key)
{
    int result;
    /* combine all inputs in some way */
    result = key->i * key->i + (key->p1.x * key->p1.x) - (key->p2.x * key->p2.x);
    /* make sure result isn't out of bounds of the array */
    return (result % SIZE_OF_MAP);
}