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
 * This class provides a safe replacement for a variable-sized array in Ironclad C++.
 * In C++, an array at the end of a struct can have a variable size by simply 
 * allocating additional memory at the end of the structure using malloc.  This 
 * is used for efficiency to avoid an additional call to the allocator and to 
 * avoid allocating the maximum necessary space for each struct (i.e., with a 
 * fixed size array).  The varray<T> class encapsulates this functionality 
 * by allowing the user to specify the size and type of the variable-sized 
 * part of the structure at allocation time.
 */

#include <initializer_list>

#include <cassert>
#include <cstdlib>

namespace safe{

template <class T> class varray{
public:
  size_t size;
  T data[1];

public:
  /*
   * Constructors
   */

  varray<T>() : size(1) {}

  varray<T>(std::initializer_list<T> static_init){
    for(auto it = static_init.begin(); it != static_init.end(); ++it){
      data[size] = *it;
      ++size;
    }
  }

  /*
   * Indexing operator
   */

  size_t getSize(){
    return size;
  }


  inline const T& operator[] (const unsigned int index) const{
    // only need to check index < N since it's an unsigned int (can't be < 0)
    assert(index < size);
    return data[index];
  }

  inline T& operator[] (const unsigned int index){
    assert(index < size);
    return data[index];
  }

  void * convert_to_void(){
    return (void*)data;
  }

  T * convert_to_raw(){
    return (T*)data;
  }

  operator aptr<T>() const {
    aptr<T> newPtr((T*)data,size);
    return newPtr;
  }
};

}
