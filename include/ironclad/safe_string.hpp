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
 * This file provides wrappers for the cstring functions from the C standard 
 * library.  It also provides the macros LITERAL() and WRAP_LITERAL() that 
 * are used to turn string literals into aptr<char>.
 */

#include "aptr.hpp"
#include <cstring>

namespace safe{

#define WRAP_LITERAL(x) safe::aptr<const char>(x,safe::str_literal_size(x))

#ifdef _HAVE_IRONCLAD_STL
#define LITERAL(x) WRAP_LITERAL(x)
#else
#define LITERAL(x) x
#endif

#define PRINT(x) safe::safe_printf(LITERAL(x))

static size_t str_literal_size(const char * str){
  return strlen(str) + 1;
}

#define NULL_TERM_CHECK(s) assert(memchr(s.convert_to_void(),'\0',s.getSize()) != NULL)

static size_t safe_strlen(aptr<char> str){
  char * raw_str = str.convert_to_raw();
  for(size_t c = 0; c < str.getSize(); ++c){
    if(raw_str[c] == '\0'){
      return c;
    }
  }
  // If we didn't find a null terminator, always fail
  assert(false);
  return 0;
}

// TODO: Do we need a check on dst being null-terminated here?

// TODO: Change data and index accesses to internal accessor methods
// that are friends of the pointer classes

static aptr<char> safe_strcat(aptr<char> dst, aptr<char> src){
  NULL_TERM_CHECK(src);
  int len = strlen(src.convert_to_raw());
  assert(dst.spatialCheck(len));
  strcat(dst.convert_to_raw(),src.convert_to_raw());
  return dst;
}

static laptr<char> safe_strcat(laptr<char> dst, aptr<char> src){
  NULL_TERM_CHECK(src);
  int len = strlen(src.convert_to_raw());
  assert(dst.spatialCheck(len));
  strcat(dst.convert_to_raw(),src.convert_to_raw());
  return dst;
}

static aptr<char> safe_strncat(aptr<char> dst, aptr<char> src, size_t num){
  NULL_TERM_CHECK(src);
  assert(dst.spatialCheck(num));
  strncat(dst.convert_to_raw(),src.convert_to_raw(),num);
  return dst;
}

static int safe_strcmp( aptr<char> str1, aptr<char> str2){
  NULL_TERM_CHECK(str1);
  NULL_TERM_CHECK(str2);
  return strcmp(str1.convert_to_raw(), str2.convert_to_raw());
}

static int safe_strncmp( aptr<char> str1, aptr<char> str2, size_t num){
  str1.spatialCheck(num);
  str2.spatialCheck(num);
  return strncmp(str1.convert_to_raw(), str2.convert_to_raw(),num);
}

static aptr<char> safe_strtok(aptr<char> str, aptr<char> del){
  if(str != NULL){
    NULL_TERM_CHECK(str);
  }
  if(del != NULL){
    NULL_TERM_CHECK(del);
  }

  static aptr<char> oldString;

  if(str != NULL){
    oldString = str;
  }

  char* result = strtok(str.convert_to_raw(),del.convert_to_raw());

  if(result != NULL){
    long index = result - oldString.convert_to_raw();
    aptr<char> ret(oldString.convert_to_raw(),oldString.getSize(),index);
    return ret;
  }else{
    return nullptr;
  }
}

static aptr<char> safe_strcpy( aptr<char> str1, aptr<char> str2){
  NULL_TERM_CHECK(str2);
  size_t len = strlen(str2.convert_to_raw());
  assert(str1.spatialCheck(len));
  strcpy(str1.convert_to_raw(), str2.convert_to_raw());
  aptr<char> ret(str1);
  return ret;
}

static aptr<char> safe_strncpy( aptr<char> str1, aptr<char> str2, size_t num){
  assert(str1.getSize() >= num);
  assert(str2.getSize() >= num);
  strncpy(str1.convert_to_raw(), str2.convert_to_raw(),num);
  aptr<char> ret(str1);
  return ret;
}

static aptr<char> safe_strchr( aptr<char> str, int character ){
  NULL_TERM_CHECK(str);
  char * result = strchr(str.convert_to_raw(),character);

  if(result != NULL){
    long index = result - str.convert_to_raw();
    aptr<char> ret(result,str.getSize(),index);
    return ret;
  }else{
    return nullptr;
  }
}

template <class T> aptr<T> safe_memset(aptr<T> p, int value, size_t size){
  assert(p.spatialCheck(size));
  memset(p.convert_to_void(),value,size);
  return p;
}

template <class T> aptr<T> safe_memcpy(aptr<T> dest, aptr<T> src, size_t size){
  assert(src.spatialCheck(size));
  assert(dest.spatialCheck(size));
  memcpy(dest.convert_to_void(), src.convert_to_void(), size);
  return dest;
}

template <class T> aptr<T> safe_memmove(aptr<T> dest, aptr<T> src, size_t size){
  assert(src.spatialCheck(size));
  assert(dest.spatialCheck(size));
  memmove(dest.convert_to_void(), src.convert_to_void(), size);
  return dest;
}

template <class T> int safe_memcmp(aptr<const T> ptr1, aptr<const T> ptr2, size_t size){
  assert(ptr1.spatialCheck(size));
  assert(ptr2.spatialCheck(size));
  return memcmp(ptr1.convert_to_void(), ptr2.convert_to_void(), size);
}

}
