
namespace utils
{

template<class T> 
const T& Max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template<class T> 
const T& Min(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template <typename T>
T Clip(const T& n, const T& lower, const T& upper) {
  return Max(lower, Min(n, upper));
}

}