#pragma once

// Copyright (c) 2013, Christian DeLozier. All rights reserved.

// The techniques used by this project are based on the paper "Ironclad C++: A
// Library-Augmented Type-Safe Subset of C++." by Christian DeLozier,
// Richard Eisenberg, Peter-Michael Osera, Santosh Nagarakatte, Milo M. K. Martin,
// and Steve Zdancewic.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.

// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.

// Neither the names of Ironclad C++, the University of Pennsylvania, nor the names
// of its contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/*
 * The laptr<T> class provides an "array pointer" counterpart to lptr<T>.  It 
 * mainly allows stack allocated arrays to be passed as arguments to other 
 * functions.  Due to the enforcement of dynamic checks on assignments, 
 * laptr<T> can ensure that a stack allocated array that is passed to another
 * function does not escape the stack.
 */

#include "stack.hpp"

#ifdef _HAVE_BDW_GC
#include <gc.h>
#endif

#include "common.hpp"
#include "lptr.hpp"
#include <cstdlib>
#include <cassert>

namespace safe{

template <class T> class laptr{
  T * data;
  long index;
  long size;
  unsigned long tb;

  void checkStackBoundGlobal(){
    if((size_t)this < tb){
      tb |= 1;
    }
  }

public:

  template<typename> friend class laptr;
  template<typename,int> friend class aptr;
  template<typename,size_t> friend class array;
  
  laptr() : data(NULL), index(0), size(0) {
    GETSP(tb);
  }

  laptr(T * _data, long _size) : data(_data), index(0), size(_size) {
    GETSP(tb);
    checkStackBoundGlobal();
  }

  laptr(T * _data, long _size, long _index) : data(_data), index(_index), size(_size) {
    GETSP(tb);
    checkStackBoundGlobal();
  }

  laptr(laptr & other) : data(other.data), index(other.index), size(other.size){
    GETSP(tb);
    checkStackBoundGlobal();
  }

  laptr(laptr const & other) : data(other.data), index(other.index), size(other.size){
    GETSP(tb);
    checkStackBoundGlobal();
  }

  template <class U> laptr(laptr<U> & other) : data(other.data), index(other.index), size(other.size){
    GETSP(tb);
    checkStackBoundGlobal();
  }

  template <class U> laptr(const laptr<U> & other) : data(other.data), index(other.index), size(other.size){
    GETSP(tb);
    checkStackBoundGlobal();
  }

  inline bool notOnStack() const {
    return (tb & 1);
  }

  inline bool dataNotOnStack(size_t other) const {
    return other <= tb;
  }

  inline void setAsGlobal(){
    tb |= 1;
  }

  inline long getSize() const 
  {
    return size;
  }

  inline long getIndex() const 
  {
    return index;
  }

  inline laptr<T>& operator++() {
    ++index;
    return *this;
  }

  inline laptr<T> operator++(int) {
    laptr<T> ret(*this);
    ++index;
    return ret;
  }

  inline laptr<T>& operator--() {
    --index;
    return *this;
  }

  inline laptr<T> operator--(int) {
    laptr<T> ret(*this);
    --index;
    return ret;
  }

  inline T* operator-> () {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return data + index;
  }

  inline T& operator* () {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return *(data + index);
  }

  inline T* operator-> () const {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return data + index;
  }

  inline T& operator* () const {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return *(data + index);
  }

  inline laptr<T>& operator+= (int op) {
    index += op;
    return *this;
  }

  inline laptr<T>& operator-= (int op) {
    index -= op;
    return *this;
  }

  inline std::ptrdiff_t operator- (laptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline std::ptrdiff_t operator- (const laptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline laptr<T> operator- (size_t diff) const {
    laptr<T> new_ptr(data,size);
    new_ptr.index = index - diff;
    return new_ptr;
  }

  inline laptr<T> operator+ (size_t diff) const {
    laptr<T> new_ptr(data,size);
    new_ptr.index = index + diff;
    return new_ptr;
  }

  inline laptr<T>& operator= (const laptr<T>& other) {
    assert(other.data == NULL || (other.tb & 1) || (tb ^ 1) <= (size_t)other.data);
    data = other.data;
    size = other.size;
    index = other.index;
    if(other.tb ^ 1){
      tb |= 1;
    }else{
      tb = (tb & 1) ? (tb ^ 1) : (tb | 1);
    }
    return *this;
  }

  inline T& operator[] (const unsigned int _index) const 
    __attribute__((always_inline))
  {
    assert(data != NULL);
    assert(index + _index >= 0 && index + _index < size);
    return data[index + _index];
  }

  inline T * convert_to_raw() const {
    return const_cast<T*>(data + index);
  }

  inline T * getData() const {
    return data;
  }

  inline void * convert_to_void() const {
    return (void*)(data + index);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data + index);
  }

  inline bool spatialCheck(size_t numBytes) const {
    return (index + (numBytes / sizeof(T)) >= 0 && index + (numBytes / sizeof(T)) <= size);
  }

  typedef T * laptr::*unspecified_bool_type;
  
  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &laptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  inline laptr<T> offset(unsigned int _index) const {
    laptr<T> newP(data,size,index+_index);
    return newP;
  }

  inline operator lptr<T>() const {
    lptr<T> newPtr(data+index);
    if(data != NULL){
      assert(index >= 0 && index < size);
    }
    return newPtr;
  }

  template<typename S> friend bool operator== (const laptr<S>& laptr1,const laptr<S>& laptr2);
  template<typename S> friend bool operator!= (const laptr<S>& laptr1,const laptr<S>& laptr2);

  template<typename S> friend bool operator== (const laptr<S>& laptr1, const S * laptr2);
  template<typename S> friend bool operator== (const S * laptr1, const laptr<S>& laptr2);

  template<typename S> friend bool operator!= (const laptr<S>& laptr1, const S * laptr2);
  template<typename S> friend bool operator!= (const S * laptr1, const laptr<S>& laptr2);

  template<typename S> friend bool operator> (const laptr<S>& laptr1, const laptr<S>& laptr2);
  template<typename S> friend bool operator>= (const laptr<S>& laptr1, const laptr<S>& laptr2);
  template<typename S> friend bool operator< (const laptr<S>& laptr1,const laptr<S>& laptr2);
  template<typename S> friend bool operator<= (const laptr<S>& laptr1,const laptr<S>& laptr2);
};

template<class T> inline bool operator== (const laptr<T>& laptr1,const laptr<T>& laptr2){
  return (laptr1.data + laptr1.index) == (laptr2.data + laptr2.index);
}

template<class T> inline bool operator== (const laptr<T>& laptr1, const T * laptr2){
  return (laptr1.data + laptr1.index) == laptr2;
}

template<class T> inline bool operator== (const T * laptr1, const laptr<T>& laptr2){
  return laptr1 == (laptr2.data + laptr2.index);
}

template<class T> inline bool operator!= (const laptr<T>& laptr1,const laptr<T>& laptr2){
  return (laptr1.data + laptr1.index) != (laptr2.data + laptr2.index);
}

template<class T> inline bool operator!= (const laptr<T>& laptr1, const T * laptr2){
  return (laptr1.data + laptr1.index) != laptr2;
}

template<class T> inline bool operator!= (const T * laptr1, const laptr<T>& laptr2){
  return laptr1 != (laptr2.data + laptr2.index);
}

template<class T> inline bool operator< (const laptr<T>& laptr1,const laptr<T>& laptr2){
  return (laptr1.data + laptr1.index) < (laptr2.data + laptr2.index);
}

template<class T> inline bool operator<= (const laptr<T>& laptr1,const laptr<T>& laptr2){
  return (laptr1.data + laptr1.index) <= (laptr2.data + laptr2.index);
}

template<class T> inline bool operator> (const laptr<T>& laptr1,const laptr<T>& laptr2){
  return (laptr1.data + laptr1.index) > (laptr2.data + laptr2.index);
}

template<class T> inline bool operator>= (const laptr<T>& laptr1,const laptr<T>& laptr2){
  return (laptr1.data + laptr1.index) >= (laptr2.data + laptr2.index);
}

}
