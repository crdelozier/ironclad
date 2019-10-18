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
 * The ptr<T> class provides strong static typing for Ironclad C++ programs 
 * by statically preventing unsafe idioms, such as void * pointers and unchecked
 * type casts.  ptr<T> allows dereference, but it does not allow indexing or 
 * pointer arithmetic.  This restriction allows ptr<T> to avoid the need 
 * for a bounds check on every dereference.  Thus, ptr<T> only requires a 
 * null check on dereference.
 */

#ifdef _HAVE_BDW_GC
#include <gc.h>
#endif

#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <cstdio>

#include "lptr.hpp"
#include "tags.hpp"

#ifdef _ENABLE_PRECISE_GC
extern void ironclad_precise_mark(void* pointer, void** source);
#endif

namespace safe{

template <class T> class ptr
#ifdef _ENABLE_PRECISE_GC
: public IroncladPreciseGC
#endif
{

  mutable T * data;

public:
  template<typename> friend class ptr;
  template<typename,int> friend class aptr;
  template<typename> friend class lptr;
  template<typename S, typename... TArgs> friend ptr<S> new_ptr(TArgs... args);

  ptr() : data(NULL) {
  }

  // TODO: Remove this constructor in favor of a nullptr constructor
  ptr(T * new_data) : data(new_data) {
  }

  ~ptr(){
  }

  ptr(ptr & other) : data(other.data) {
  }

  ptr(ptr const & other) : data(other.data) {
  }
  
  template <class U> ptr(ptr<U> & other) : data(other.data){
  }

  template <class U> ptr(const ptr<U> & other) : data(other.data){
  }

  operator lptr<T>() const{
    lptr<T> newPtr(data);
    newPtr.tb |= 1;
    return newPtr;
  }


  // TODO: Remove lptr constructor and change to from_lptr
  ptr(lptr<T> & other) : data(other.data) {
    assert(other.notOnStack());
  }

  ptr(lptr<T> const & other) : data(other.data) {
    assert(other.notOnStack());
  }

  void mark() const 
  {
#ifdef _ENABLE_PRECISE_GC
    if(data != nullptr){
      ironclad_precise_mark((void*)data,(void**)&data);
    }
#endif
  }

  inline T* operator-> () 
    __attribute__((always_inline))
  {
    assert(data != NULL);
    return data;
  }

  inline T& operator* () 
    __attribute__((always_inline))
  {
    assert(data != NULL);
    return *data;
  }

  inline T* operator-> () const 
    __attribute__((always_inline))
  {
    assert(data != NULL);
    return data;
  }

  inline T& operator* () const 
    __attribute__((always_inline))
  {
    assert(data != NULL);
    return *data;
  }

  inline std::ptrdiff_t operator- (ptr<T> & other) {
    return this->data - other.data;
  }

  inline std::ptrdiff_t operator- (const ptr<T> & other) {
    return this->data - other.data;
  }

  inline ptr<T>& operator= (const ptr<T>& other) {
    data = other.data;
    return *this;
  }

  inline void free() const {
#ifndef _HAVE_BDW_GC
    std::free(static_cast<void*>(data));
#endif
    data = NULL;
  }

  inline T * convert_to_raw() const {
    return const_cast<T*>(data);
  }

  inline void * convert_to_void() const {
    return reinterpret_cast<void *>(data);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data);
  }

  typedef T * ptr::*unspecified_bool_type;

  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &ptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  template<typename S> friend bool operator== (const ptr<S>& ptr1,const ptr<S>& ptr2);
  template<typename S> friend bool operator== (const ptr<S>& ptr1, const S * ptr2);
  template<typename S> friend bool operator== (const S * ptr1, const ptr<S>& ptr2);
  template<typename S> friend bool operator!= (const ptr<S>& ptr1,const ptr<S>& ptr2);
  template<typename S> friend bool operator!= (const ptr<S>& ptr1, const S * ptr2);
  template<typename S> friend bool operator!= (const S * ptr1, const ptr<S>& ptr2);
};

template<class T> inline bool operator== (const ptr<T>& ptr1,const ptr<T>& ptr2) 
{
  return ptr1.data == ptr2.data;
}

template<class T> inline bool operator== (const ptr<T>& ptr1, const T * ptr2)
{
  return ptr1.data == ptr2;
}

template<class T> inline bool operator== (const T * ptr1, const ptr<T>& ptr2)
{
  return ptr2.data == ptr1;
}

template<class T> inline bool operator!= (const ptr<T>& ptr1,const ptr<T>& ptr2)
{
  return ptr1.data != ptr2.data;
}


template<class T> inline bool operator!= (const ptr<T>& ptr1, const T * ptr2)
{
  return ptr1.data != ptr2;
}

template<class T> inline bool operator!= (const T * ptr1, const ptr<T>& ptr2)
{
  return ptr2.data != ptr1;
}

}
