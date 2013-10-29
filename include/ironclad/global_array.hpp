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
 * This provides a global array that is able to convert to an aptr<T> instead 
 * of an laptr<T>.  This is used in the case that a static-sized array 
 * is declared as a global instead of on the stack.
 */

#include <initializer_list>

#include "aptr.hpp"
#include "laptr.hpp"
#include <cassert>
#include <cstdlib>

#include "safe_mem.hpp"

namespace safe{

template<class T> class global_array_initializer{
public:
  static void init_global_array(T * data, std::initializer_list<T> static_init){
    int c = 0;
    for(auto it = static_init.begin(); it != static_init.end(); ++it){
      T * hack = (T*)&(data[c]);
      *hack = *it;
      ++c;
    } 
  }
};

template<class T> class global_array_initializer<const T>{
public:
  static void init_global_array(const T * data, std::initializer_list<const T> static_init){
    int c = 0;
    for(auto it = static_init.begin(); it != static_init.end(); ++it){
      T * hack = (T*)&(data[c]);
      *hack = *it;
      ++c;
    } 
  }
};

template <class T, size_t ARRAY_N> class global_array{
private:
  T data[ARRAY_N];

public:
  /*
   * Constructors
   */

  global_array<T,ARRAY_N>() {
  }

  void zero(){
    safe::zero<T>(getAptr(),ARRAY_N);
  }

  global_array(std::initializer_list<T> static_init) : data() {
    global_array_initializer<T> array_init;
    int c = 0;
    for(auto it = static_init.begin(); it != static_init.end(); ++it){
      array_init.init_global_array(data,static_init);
      ++c;
    }
  }

  size_t getSize() const{
    return ARRAY_N;
  }

  inline T& operator* () 
#ifndef SAFE_DONT_INLINE
    __attribute__((always_inline))
#endif
  {
    return data[0];
  }

  /*
   * Indexing operator
   */

  inline const T& operator[] (const unsigned int index) const 
#ifndef SAFE_DONT_INLINE
    __attribute__((always_inline))
#endif
  {
    // only need to check index < N since it's an unsigned int (can't be < 0)
    assert(index < ARRAY_N);
    return data[index];
  }

  inline T& operator[] (const unsigned int index) 
    __attribute__((always_inline))
  {
    assert(index < ARRAY_N);
    return data[index];
  }

  inline operator aptr<T>() const
  {
    laptr<T> newPtr(const_cast<T*>(data),ARRAY_N);
    if((size_t)data <= newPtr.tb){
      newPtr.tb |= 1;
    }
    return newPtr;
  }

  inline aptr<T> offset(unsigned int index) const 
  {
    laptr<T> newP(const_cast<T*>(&data[index]),ARRAY_N,index);
    assert(index < ARRAY_N);
    return newP;
  }
};

}
