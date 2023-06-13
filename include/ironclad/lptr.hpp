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
 * The lptr<T> class allows the use of stack allocations in Ironclad C++.  All 
 * address-of operations must immediately be held by an lptr<T>, which can 
 * further check to ensure that the stack address will not become a dangling 
 * pointer in the future (when a stack frame is deallocated).
 */

#include "stack.hpp"

#ifdef _HAVE_BDW_GC
#include <gc.h>
#endif

#include <cstdlib>
#include <cassert>

namespace ironclad{

template <class T> class lptr{
  T * data;

public:
  unsigned long tb;

public:
  template<typename> friend class lptr;
  template<typename> friend class laptr;
  template<typename> friend class ptr;

  /*
   * All constructors for lptr<T> initialize the "temporal bound" (tb) to 
   * the current value of the stack pointer.  In the constructor body, 
   * we have a new stack frame, and therefore the "temporal bound" will 
   * allow the lptr<T> to accept addresses from its own stack frame, but 
   * not from any stack frames that will be allocated in the future.
   */

  lptr() : data(NULL) {
    GETSP(tb);
  }

  lptr(T * new_data) : data(new_data) {
    GETSP(tb);
    if((size_t)data < tb){
      // Either global or heap
      tb |= 1;
    }
  }

  lptr(lptr & other) : data(other.data) {
    GETSP(tb);
    if((size_t)data < tb){
      tb |= 1;
    }
  }

  lptr(lptr const & other) : data(other.data) {
    GETSP(tb);
    if((size_t)data < tb){
      tb |= 1;
    }
  }
  
  template <class U> lptr(lptr<U> & other) : data(other.data){
    GETSP(tb);
    if((size_t)data < tb){
      tb |= 1;
    }
  }
  
  template <class U> lptr(const lptr<U> & other) : data(other.data){
    GETSP(tb);
    if((size_t)data < tb){
      tb |= 1;
    }
  }

  inline bool notOnStack() const
  {
    return (tb & 1);
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

  inline std::ptrdiff_t operator- (lptr<T> & other) const
  {
    return this->data - other.data;
  }

  inline std::ptrdiff_t operator- (const lptr<T> & other) const
  {
    return this->data - other.data;
  }

  /* 
   * The assignment for lptr<T> is checked to ensure that the lptr will not point 
   * to memory that will go out of scope before the lptr goes out of scope.
   *
   * If the other lptr<T> has the last bit set, then the address that it holds 
   * came from the globals or heap, and can therefore be held by any lptr<T>.
   * 
   * If the other lptr<T> holds a stack address, the receiving lptr must check 
   * that the address that it is receiving is greater than or equal to 
   * its "temporal bound" (tb).
   */

  inline lptr<T>& operator= (const lptr<T>& other)
  {
    assert(other.data == NULL || (other.tb & 1) || (tb ^ 1) <= (size_t)other.data);
    data = other.data;
    if(other.tb ^ 1){
      tb |= 1;
    }else{
      tb = (tb & 1) ? (tb ^ 1) : (tb | 1);
    }
    return *this;
  }

  bool can_accept(T * addr){
    return addr == NULL || (tb ^ 1) <= (size_t)addr;
  }

  inline T * convert_to_raw() const 
  {
    return const_cast<T*>(data);
  }

  inline void * convert_to_void() const 
  {
    return reinterpret_cast<void *>(data);
  }

  inline unsigned long convert_to_long() const 
  {
    return reinterpret_cast<unsigned long>(data);
  }

  typedef T * lptr::*unspecified_bool_type;

  inline operator unspecified_bool_type() const
  {
    return data == 0 ? 0 : &lptr::data;
  }

  inline bool operator! () const 
  {
      return data == NULL;
  }

  template <class U> friend lptr<U> cast(lptr<T> & p);
  template <class U> friend lptr<U> unsafe_cast(lptr<T> & p);

  template<typename S> friend bool operator== (const lptr<S>& lptr1,const lptr<S>& lptr2);
  template<typename S> friend bool operator== (const lptr<S>& lptr1, const S * lptr2);
  template<typename S> friend bool operator== (const S * lptr1, const lptr<S>& lptr2);
  template<typename S> friend bool operator!= (const lptr<S>& lptr1,const lptr<S>& lptr2);
  template<typename S> friend bool operator!= (const lptr<S>& lptr1, const S * lptr2);
  template<typename S> friend bool operator!= (const S * lptr1, const lptr<S>& lptr2);
};

template<class T> inline bool operator== (const lptr<T>& lptr1,const lptr<T>& lptr2){
  return lptr1.data == lptr2.data;
}

template<class T> inline bool operator== (const lptr<T>& lptr1, const T * lptr2){
  return lptr1.data == lptr2;
}

template<class T> inline bool operator== (const T * lptr1, const lptr<T>& lptr2){
  return lptr2.data == lptr1;
}

template<class T> inline bool operator!= (const lptr<T>& lptr1,const lptr<T>& lptr2){
  return lptr1.data != lptr2.data;
}


template<class T> inline bool operator!= (const lptr<T>& lptr1, const T * lptr2){
  return lptr1.data != lptr2;
}

template<class T> inline bool operator!= (const T * lptr1, const lptr<T>& lptr2){
  return lptr2.data != lptr1;
}

}
