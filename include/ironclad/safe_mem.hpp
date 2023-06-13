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
 * This file provides wrappers and replacements for the standard memory functions 
 * in the C standard library.  Most notably, memset and memcpy are replaced
 * by calls to zero<T> and copy<T>.
 */

#include <cstring>
#include <utility>

namespace ironclad {

template <typename T>
struct has_zero {
private:
  template <typename U>
  static decltype(std::declval<U>().zero(), void(), std::true_type()) test(int);
  
  template <typename>
  static std::false_type test(...);
public:
  typedef decltype(test<T>(0)) type;
  enum { value = type::value };
};

template<typename T, bool H = has_zero<T>::value >
struct zero_helper{
  static inline void zero(T & val){
    val = 0;
  }
};

template<typename T>
struct zero_helper<T,true>{
  static inline void zero(T & val){
    val.zero();
  }
};

/*
 * Safe replacement for memset with value of zero
 */

template<typename T>
inline void zero(aptr<T> p, size_t size){
  for(size_t c = 0; c < size; ++c){
    zero_helper<T>::zero(p[c]);
  }
}

template<typename T>
inline void zero(laptr<T> p, size_t size){
  for(size_t c = 0; c< size; ++c){
    zero_helper<T>::zero(p[c]);
  }
}

/*
 * Safe replacement for memset with a non-zero value
 */

template<typename T> 
inline aptr<T> fill(aptr<T> p, T value, size_t size){
  for(size_t c = 0; c < size; ++c){
    p[c] = value;
  }
  return p;
}

/*
 * Safe replacement for memcpy
 */

template<typename T> 
inline void copy(aptr<T> dest, aptr<T> src, size_t size){
  for(size_t c = 0; c < size; ++c){
    dest[c] = src[c];
  }
}

template<typename T>
inline void copy(aptr<T> dest, laptr<T> src, size_t size){
  for(size_t c = 0; c < size; ++c){
    dest[c] = src[c];
  }
}

template<typename T>
inline void copy(laptr<T> dest, laptr<T> src, size_t size){
  for(size_t c = 0; c < size; ++c){
    dest[c] = src[c];
  }
}

template<>
inline void copy<int>(aptr<int> dest, aptr<int> src, size_t size){
  dest.spatialCheck(size * sizeof(int));
  src.spatialCheck(size * sizeof(int));
  memcpy(dest.convert_to_void(),src.convert_to_void(),size * sizeof(int));
}

template<>
inline void copy<int>(laptr<int> dest, laptr<int> src, size_t size){
  dest.spatialCheck(size * sizeof(int));
  src.spatialCheck(size * sizeof(int));
  memcpy(dest.convert_to_void(),src.convert_to_void(),size * sizeof(int));
}

/*
 * Alternative to copy that uses placement new instead of assignment
 */

template<typename T> 
inline aptr<T> clone(aptr<T> dest, aptr<T> src, size_t size){
  for(size_t c = 0; c < size; ++c){
    new(((char*)&dest[c])) T(src[c]);
  }
  return dest;
}

}
