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
 * This file contains all of the cast operations for Ironclad C++ smart pointers.
 * Casts operations on pointers to primitive types are template specialized.  We 
 * allow these unchecked casts because the underlying data can never be used as
 * a pointer, which is sufficient to prevent a security vulnerability due to 
 * unchecked casts.
 */

#include "ptr.hpp"
#include "aptr.hpp"
#include "lptr.hpp"
#include "laptr.hpp"

namespace safe{

namespace internal{
  template <class U, class T>
  inline ptr<U> internal_cast(ptr<T> &p)
  {
    return ptr<U>((U*)p.convert_to_raw());
  }

  template <class U, class T>
  inline aptr<U> internal_cast(aptr<T> &p)
  {
    long newSize = (p.getSize() * sizeof(T)) / sizeof(U);
    aptr<U> newPtr((U*)p.getData(),
                   newSize,p.getIndex());
    return newPtr;
  }

  template <class U, class T>
  inline laptr<U> internal_cast(laptr<T> &p)
  {
    long newSize = (p.getSize() * sizeof(T)) / sizeof(U);
    laptr<U> newPtr((U*)p.getData(),
                    newSize,p.getIndex());
    return newPtr;
  }
}

template <class U,class T>
inline ptr<U> cast(ptr<T> &p) 
{
  ptr<U> newPtr(dynamic_cast<U*>(p.convert_to_raw()));
  return newPtr;
}

template<>
inline ptr<unsigned int> cast(ptr<double> &p)
{
  return internal::internal_cast<unsigned int,double>(p);
}

template <class U,class T>
inline ptr<U> cnst_cast(ptr<T> &p) 
{
  ptr<U> newPtr(const_cast<U*>(p.convert_to_raw()));
  return newPtr;
}

template <class U, class T>
inline aptr<U> cast(aptr<T> &p) 
{
  aptr<U> newPtr(dynamic_cast<U*>(p.getData()),p.getSize(),p.getIndex());
  return newPtr;
}

template<>
inline aptr<char> cast(aptr<unsigned char> &p)
{
  return internal::internal_cast<char,unsigned char>(p);
}

template<>
inline aptr<char> cast(aptr<int> &p)
{
  return internal::internal_cast<char,int>(p);
}

template<>
inline aptr<char> cast(aptr<double> &p)
{
  return internal::internal_cast<char,double>(p);
}

template<>
inline aptr<char> cast(aptr<float> &p)
{
  return internal::internal_cast<char,float>(p);
}

template<>
inline aptr<unsigned char> cast(aptr<unsigned int> &p)
{
  return internal::internal_cast<unsigned char,unsigned int>(p);
}

template<>
inline aptr<unsigned short> cast(aptr<unsigned char> &p)
{
  return internal::internal_cast<unsigned short,unsigned char>(p);
}

template<>
inline aptr<unsigned char> cast(aptr<char> &p)
{
  return internal::internal_cast<unsigned char,char>(p);
}

template<>
inline aptr<unsigned short> cast(aptr<unsigned int> &p)
{
  return internal::internal_cast<unsigned short,unsigned int>(p);
}

template <class U, class T>
inline aptr<U> cnst_cast(aptr<T> &p) 
{
  aptr<U> newPtr(const_cast<U*>(p.getData()),p.getSize(),p.index);
  return newPtr;
}

template <class U, class T>
inline lptr<U> cast(lptr<T> &p) 
{
  return lptr<U>(dynamic_cast<U*>(p.convert_to_raw()));
}

template <class U, class T>
inline laptr<U> cast(laptr<T> &p) 
{
  laptr<U> newPtr(dynamic_cast<U*>(p.getData()),p.getSize(),p.getIndex());
  return newPtr;
}

template<>
inline laptr<char> cast(laptr<unsigned char> &p)
{
  return internal::internal_cast<char,unsigned char>(p);
}

template<>
inline laptr<unsigned char> cast(laptr<long> &p){
  return internal::internal_cast<unsigned char,long>(p);
}

} // namespace safe
