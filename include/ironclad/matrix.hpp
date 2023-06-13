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
 * The matrix class provides a flattened 2-D array for Ironclad C++.  In the 
 * swaptions benchmark (from the PARSEC suite), we found that using a 
 * a flattened 2-D array was more efficient than using an aptr< < aptr<T> >
 * because the flattened array layout only requires a single bounds 
 * check on the combined x and y indices.  Otherwise, an aptr< aptr<T> > 
 * must first bounds check and load the 3 fields for the inner array and 
 * then bounds check that inner array (on every access).
 */

#include <cassert>

namespace ironclad{

template <class T> class matrix{
public:
  mutable T * data;
  size_t x_size;
  size_t y_size;

public:

  matrix() : data(NULL), x_size(0), y_size(0){
  }

  matrix(T * _data, size_t _x_size, size_t _y_size) : 
    data(_data), x_size(_x_size), y_size(_y_size){
  }

  T & operator() (size_t x, size_t y){
    assert(x < x_size && y < y_size);
    return data[y * x_size + x];
  }

  void free() const{
#ifndef _HAVE_BDW_GC
    std::free(data);
#endif
    data = NULL;
  }
};

}
