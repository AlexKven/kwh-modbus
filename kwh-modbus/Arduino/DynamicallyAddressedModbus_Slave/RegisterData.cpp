#include <stdint.h>

class RegisterData
{
private:
  uint16_t * _Array;
  int _Length;
public:
  RegisterData(int length)
  {
    _Length = length;
    _Array = new uint16_t[length];
    for (int i = 0; i < length; i++)
      _Array[i] = 0;
  }

  RegisterData(uint16_t *_array, int length)
  {
    _Array = _array;
    _Length = length;
  }
  
  template<typename T>
  T get(int index)
  {
    T* arr = (T*)_Array;
    return arr[index];
  }

  template<typename T>
  void set(int index, T value)
  {
    T* arr = (T*)_Array;
    arr[index] = value;
  }

  template<typename T>
  int getLength()
  {
    return 2 * _Length / sizeof(T);
  }

  template<typename T>
  T* getArray()
  {
    return (T*)_Array;
  }

  uint16_t get(int index)
  {
    return _Array[index];
  }

  void set(int index, uint16_t value)
  {
    _Array[index] = value;
  }
  
  uint16_t* getArray()
  {
    return _Array;
  }

  int getLength()
  {
    return _Length;
  }

  void setString(int index, char* str, int maxLength)
  {
    int i = index;
    char* c = str;
    while (i < getLength<char>() && *c != '\0' && (maxLength < 0 || i - index < maxLength))
    {
      set<char>(i, *c);
      i++;
      c++;
    }
  }
};