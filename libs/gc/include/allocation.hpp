#ifndef _IRONCLAD_ALLOCATION_H
#define _IRONCLAD_ALLOCATION_H

#include <typeinfo>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>

class allocation_base{
public:
  virtual void mark(void * address){}
};

template <class T>
class allocation_tag : public allocation_base{
public:
  T * data;
  allocation_tag(T * _data) : data(_data){}

  virtual void mark(void * address){
    data->mark();
  }
};

template <>
class allocation_tag<char> : public allocation_base{
public:
  allocation_tag() {}
  virtual void mark(void * address){
  }
};

template <class T>
class tagged_allocation : public allocation_base{
public:
  T data;

  template <typename... TArgs>
  tagged_allocation(TArgs... args) :
	data(args...){}

  virtual void mark(void * address){
    data.mark();
  }
};

template <>
class tagged_allocation<long> : public allocation_base{
public:
  long data;

  virtual void mark(void * address){}
};

template <>
class tagged_allocation<unsigned long> : public allocation_base{
public:
  unsigned long data;

  virtual void mark(void * address){}
};

template <class T>
class tagged_array_allocation : public allocation_base{
public:
  size_t size;
  T * base;
  
  tagged_array_allocation(size_t _size, T * _base) : size(_size), base(_base){}

  virtual void mark(void * address){
    std::cout << __PRETTY_FUNCTION__ << "\n";
    for(size_t i = 0; i < size; ++i){
      base[i].mark();
    }
  }
};

template <>
class tagged_array_allocation<char> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, char * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<short> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, short * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<unsigned int> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, unsigned int * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<bool> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, bool * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<int> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, int * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<unsigned char> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, unsigned char * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<unsigned short> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, unsigned short * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<double> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, double * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<float> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, float * _base) {}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<long> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, long * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <>
class tagged_array_allocation<unsigned long> : public allocation_base{
public:
  tagged_array_allocation(size_t _size, unsigned long * _base){}

  virtual void mark(void * address){
    // No need to mark because it doesn't hold pointers
  }
};

template <class T> class type_marker{
public:
  static void mark(T & obj){
    obj.mark();
  }
};

template <> class type_marker<unsigned short>{
public:
  static void mark(unsigned short & obj){}
};

template <> class type_marker<double>{
public:
  static void mark(double & obj){}
};

template <> class type_marker<unsigned char>{
public:
  static void mark(unsigned char & obj){}
};

template <> class type_marker<char>{
public:
  static void mark(char & obj){}
};

template <> class type_marker<unsigned int>{
public:
  static void mark(unsigned int & obj){}
};

template <> class type_marker<int>{
public:
  static void mark(int & obj){}
};

template <> class type_marker<unsigned long>{
public:
  static void mark(unsigned long & obj){}
};

#endif
