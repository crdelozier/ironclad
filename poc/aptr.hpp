#pragma once

#include <cstdlib>
#include <cassert>

namespace safe{

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T, int B = 0> class aptr
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;
  long size;

public:
  template<typename,int> friend class aptr;
  template<typename S> friend aptr<S> new_array(size_t _size);

  aptr() : data(NULL), size(0) {
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data),  size(_size) {
  }

  ~aptr() {}

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    assert(_index >= 0 && _index < size);
    return data[_index];
  }
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,1>
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;

public:
  template<typename,int> friend class aptr;
  template<typename S> friend aptr<S> new_array(size_t _size);

  aptr() : data(NULL) {
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
		 :: "rax");
    asm volatile("movq %0, %%rdx"
		 : "=r" (_size)
		 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
  }

  ~aptr() {}

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + _index;
    asm volatile("movq %0, %%rax"
		 : "=r" (addr)
		 :: "rax");
    asm volatile("bndcl %rax, %bnd0");
    asm volatile("bndcu %rax, %bnd0");
    return data[_index];
  }
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,2>
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;

public:
  template<typename,int> friend class aptr;
  template<typename S> friend aptr<S> new_array(size_t _size);

  aptr() : data(NULL) {
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
		 :: "rax");
    asm volatile("movq %0, %%rdx"
		 : "=r" (_size)
		 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
  }

  ~aptr() {}

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + _index;
    asm volatile("movq %0, %%rax"
		 : "=r" (addr)
		 :: "rax");
    asm volatile("bndcl %rax, %bnd1");
    asm volatile("bndcu %rax, %bnd1");
    return data[_index];
  }
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,3>
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;

public:
  template<typename,int> friend class aptr;
  template<typename S> friend aptr<S> new_array(size_t _size);

  aptr() : data(NULL) {
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
		 :: "rax");
    asm volatile("movq %0, %%rdx"
		 : "=r" (_size)
		 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
  }

  ~aptr() {}

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + _index;
    asm volatile("movq %0, %%rax"
		 : "=r" (addr)
		 :: "rax");
    asm volatile("bndcl %rax, %bnd2");
    asm volatile("bndcu %rax, %bnd2");
    return data[_index];
  }
};
  
template<typename T>
static aptr<T>
new_array(size_t size)
{
  char * buffer = 
    (char*)malloc((size * sizeof(T)));
  
  aptr<T> ret(new(buffer) T[size],size);
  return ret;
}

template<typename T>
static aptr<T,1> new_array_1(size_t size)
{
  char * buffer = 
    (char*)malloc((size * sizeof(T)));
  
  aptr<T,1> ret(new(buffer) T[size],size);
  return ret;
}

template<typename T>
static aptr<T,2> new_array_2(size_t size)
{
  char * buffer = 
    (char*)malloc((size * sizeof(T)));
  
  aptr<T,2> ret(new(buffer) T[size],size);
  return ret;
}

template<typename T>
static aptr<T,3> new_array_3(size_t size)
{
  char * buffer = 
    (char*)malloc((size * sizeof(T)));
  
  aptr<T,3> ret(new(buffer) T[size],size);
  return ret;
}
  
}
