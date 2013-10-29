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
 * This file provides wrappers for standard C IO system functions.
 */

#include <cstdio>

namespace safe{

static safe::ptr<FILE> safe_fopen( safe::aptr<const char> filename, safe::aptr<const char> mode){
  char * fname = (char*)filename.convert_to_raw();
  NULL_TERM_CHECK(filename);
  char * mde = (char*)mode.convert_to_raw();
  NULL_TERM_CHECK(mode);
  return ptr<FILE>(fopen(filename.convert_to_raw(),mode.convert_to_raw()));
}

static safe::ptr<FILE> safe_fopen( safe::laptr<const char> filename, safe::aptr<const char> mode){
  char * fname = (char*)filename.convert_to_raw();
  NULL_TERM_CHECK(filename);
  char * mde = (char*)mode.convert_to_raw();
  NULL_TERM_CHECK(mode);
  return ptr<FILE>(fopen(filename.convert_to_raw(),mode.convert_to_raw()));
}

static int safe_fgetc( safe::ptr<FILE> stream ){
  return fgetc(stream.convert_to_raw());
}

template <class T> 
static size_t safe_fread(safe::aptr<T> ptr, size_t size, 
			 size_t count, safe::ptr<FILE> stream){
  assert(size == sizeof(T));
  assert(ptr.spatialCheck(count));
  return fread(ptr.convert_to_void(),size,count,stream.convert_to_raw());
}

template <class T> 
static size_t safe_fread(safe::laptr<T> ptr, size_t size, 
			 size_t count, safe::ptr<FILE> stream){
  assert(size == sizeof(T));
  assert(ptr.spatialCheck(count));
  return fread(ptr.convert_to_void(),size,count,stream.convert_to_raw());
}

template <class T> 
static size_t safe_fwrite(safe::aptr<T> ptr, size_t size, 
			  size_t count, safe::ptr<FILE> stream){
  assert(size == sizeof(T));
  assert(ptr.spatialCheck(count));
  return fwrite(ptr.convert_to_void(), sizeof(T), count, stream.convert_to_raw());
}

template <class T> 
static size_t safe_fwrite(safe::laptr<T> ptr, size_t size, 
			  size_t count, safe::ptr<FILE> stream){
  assert(size == sizeof(T));
  assert(ptr.spatialCheck(count));
  return fwrite(ptr.convert_to_void(), sizeof(T), count, stream.convert_to_raw());
}

static int safe_fclose( ptr<FILE>& stream ){
  assert(stream != NULL);
  int ret = std::fclose(stream.convert_to_raw());
  stream = nullptr;
  return ret;
}

static long safe_ftell( ptr< FILE > stream ){
  assert(stream != NULL);
  return ftell(stream.convert_to_raw());
}

static int safe_fseek( ptr< FILE > stream, long offset, int origin){
  assert(stream != NULL);
  return fseek(stream.convert_to_raw(),offset,origin);
}

static int safe_ferror( ptr< FILE > stream ){
  return ferror(stream.convert_to_raw());
}

static aptr<char> safe_fgets( aptr<char> str, int num, ptr<FILE> stream){
  assert(str != NULL);
  assert(stream != NULL);
  assert(str.spatialCheck(num));
  char * ret = fgets(str.convert_to_raw(), num, stream.convert_to_raw());
  aptr<char> retPtr(str.convert_to_raw(),str.getSize());
  return (ret == NULL) ? NULL : retPtr;
}

static int safe_feof( ptr<FILE> stream ){
  assert(stream != NULL);
  return feof(stream.convert_to_raw());
}

/*
 * Functions that require formatting
 * For these, we use variadic templates to ensure that the number of given
 * arguments matches the number of arguments required by the format
 */

namespace internal{

static bool check_format_args(safe::aptr<const char> format_wrapped){
  const char* format = format_wrapped.convert_to_raw();
  bool started = false;

  for(int c = 0; c < format_wrapped.getSize(); c++){
    if(format[c] == '%'){
      // Only safe case without additional args is that
      // the next character is a %, meaning use wants a % symbol
      assert(c+1 < format_wrapped.getSize() && format[c+1] == '%');
      c++;
      continue;
    }
  }
}

template<typename T, typename... Args>
static bool check_format_args(safe::aptr<const char> format_wrapped, 
                              T& arg, Args... args)
{
  const char* format = format_wrapped.convert_to_raw();

  for(int c = format_wrapped.getIndex(); c < format_wrapped.getSize(); c++){
    if(format[c] == '%'){
      // If not at least one character next, incorrect format
      assert(c+1 < format_wrapped.getSize());
      if(format[c+1] == '%'){
        // Ignore, there as a % symbol
        c++;
        continue;
      }
      // Move to the next % and recurse
      for(int d = c+1; d < format_wrapped.getSize(); d++){
        if(format[d] == '%'){
          check_format_args(format_wrapped+d,args...);
        }
      }
    }
  }
}

}
  
template <typename... Args>
static int safe_fprintf( ptr<FILE> stream, aptr<const char> format, Args... args){
  int total_args = 0;
  for(int c = 0; c < safe_strlen(format); ++c){
    if(format[c] == '%'){
      ++total_args;
    }
  }
  assert(sizeof...(Args) >= total_args);
  return fprintf(stream.convert_to_raw(), format.convert_to_raw(), args...);
}

template <typename... Args>
static int safe_printf( aptr<const char> format, Args... args){
  int total_args = 0;
  for(int c = 0; c < safe_strlen(format); ++c){
    if(format[c] == '%'){
      ++total_args;
    }
  }
  assert(sizeof...(Args) >= total_args);
  return printf(format.convert_to_raw(), args...);
}

template <typename... Args>
static int safe_fscanf( safe::ptr<FILE> stream, safe::aptr<const char> format, Args... args ){
  int total_args = 0;
  for(int c = 0; c < safe_strlen(format); ++c){
    if(format[c] == '%'){
      ++total_args;
    }
  }
  assert(sizeof...(Args) >= total_args);
  return scanf(stream.convert_to_raw(), format.convert_to_raw(), args...);
}

// Specific case of fscanf that otherwise doesn't work

static int safe_fscanf( safe::ptr<FILE> stream, 
			const char * format, safe::aptr<char> str ){
  return fscanf(stream.convert_to_raw(),format,str.convert_to_raw());
}

}
