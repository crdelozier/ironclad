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
 * This file provides utility functions for Ironclad C++.  The allocation methods 
 * are included in this file.  There are also additional methods that 
 * were used for specific cases of unsafety in the SPEC and PARSEC benchmark
 * suites.
 */

#ifndef VALIDATE
#include <new>
#endif

#ifdef _HAVE_BDW_GC
#include <gc.h>
#include <gc_cpp.h>

#ifdef _ENABLE_PRECISE_GC
#include <gc/allocation.hpp>
#endif // _ENABLE_PRECISE_GC
#endif // _HVAE_BDW_GC

#ifdef _ENABLE_PTHREADS
#include <pthread.h>
#endif

#include <iostream>

#include "ptr.hpp"
#include "aptr.hpp"
#include "matrix.hpp"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <stdarg.h>
#include <cstddef>

#include <ironclad/safe_string.hpp>
#include <ironclad/safe_io.hpp>

#include <functional>

#include <sys/mman.h>
#include <sys/stat.h>

namespace ironclad {

#ifdef _HAVE_BDW_GC
#ifdef _ENABLE_PRECISE_GC
#define IC_MALLOC GC_MALLOC
#define PIC_MALLOC GC_MALLOC_PRECISE
#else
#define IC_MALLOC GC_MALLOC
#endif
#else // _HAVE_BDW_GC
#define IC_MALLOC malloc
#endif

template<int, typename T>
class allocator{
public:
  template<typename... TArgs>
  static ptr<T> 
  new_ptr(TArgs... args)
  {
    T * buffer = (T*)IC_MALLOC(sizeof(T));
    ptr<T> ret(new(buffer) T(args...));
    return ret;
  }

  static aptr<T> 
  new_array(size_t size)
  {
    T * buffer = (T*)IC_MALLOC(size * sizeof(T));
    aptr<T> ret(new(buffer) T[size],size);
    return ret;
  }
};

#ifdef _ENABLE_PRECISE_GC
template<typename T>
class allocator<1,T>{
public:
  template<typename... TArgs>
  static ptr<T> new_ptr(TArgs... args)
  {
    tagged_allocation<T> * buffer = 
      (tagged_allocation<T>*)PIC_MALLOC(sizeof(tagged_allocation<T>));

    new(buffer) tagged_allocation<T>(args...);

    ptr<T> ret(&buffer->data);
    return ret;
  }

  static aptr<T>
  new_array(size_t size)
  {
    char * buffer = 
      (char*)PIC_MALLOC((size * sizeof(T)) + 
                       sizeof(tagged_array_allocation<T>));
    char * objbuffer = buffer + sizeof(tagged_array_allocation<T>);

    new(buffer) tagged_array_allocation<T>(size,(T*)objbuffer);

    aptr<T> ret(new(objbuffer) T[size],size);
    return ret;
  }
}; 
#endif // _ENABLE_PRECISE_GC

template<class T, typename... TArgs>
ptr<T>
new_ptr(TArgs... args)
{
  return allocator<IsDerivedFrom<T,IroncladPreciseGC>::Is,T>::new_ptr(args...);
}

template<class T>
aptr<T>
new_array(size_t size)
{
  return allocator<IsDerivedFrom<T,IroncladPreciseGC>::Is,T>::new_array(size);
}

template<class T> 
matrix<T> 
new_matrix(const size_t & x_size, const size_t & y_size)
{
  assert(x_size > 0 && y_size > 0);
  char * buffer = (char*)IC_MALLOC(sizeof(T) * x_size * y_size);
  matrix<T> mat(new (buffer) T[x_size * y_size],x_size,y_size);
  return mat;
}

template<typename S, typename T, typename... TArgs> 
ptr<S> 
new_variable_ptr(size_t vSize, TArgs... args)
{
  char * buffer = (char*)IC_MALLOC(sizeof(S) - sizeof(T) + (vSize * sizeof(T)));
  ptr<S> ret = new (buffer) S(args...);
  char * sizeLoc = buffer + sizeof(S) - sizeof(T) - sizeof(size_t);
  size_t * size = (size_t*)sizeLoc;
  *size = vSize;
  assert(*size != 0 && buffer != 0);
  assert(buffer != 0);
  return ret;
}

template<class T> 
aptr<T> 
new_aligned_array(size_t size, size_t align)
{
  char * buffer = (char*)IC_MALLOC((size * sizeof(T)) + align);
  while((size_t)buffer % align != 0){
    buffer++;
  }
  return aptr<T>(new(buffer) T[size],size);
}

#ifdef _ENABLE_PRECISE_GC
template<typename T, typename... TArgs>
ptr<T>
new_ptr_impl(TArgs... args)
{
  tagged_allocation<T> * buffer = 
    (tagged_allocation<T>*)IC_MALLOC(sizeof(tagged_allocation<T>));
  ptr<T> ret(new(buffer) tagged_allocation<T>(args...));
  return ret;
}

template<class T> 
aptr<T> 
new_array_impl(size_t size)
{
  char * buffer = 
    (char*)IC_MALLOC((size * sizeof(T)) + 
    sizeof(tagged_array_allocation<T>));
  char * objbuffer = buffer + sizeof(tagged_array_allocation<T>);

  new(buffer) tagged_array_allocation<T>(size,(T*)objbuffer);

  aptr<T> ret(new(objbuffer) T[size],size);
  return ret;
}

template<class T> 
matrix<T> 
new_matrix_impl(const size_t & x_size, const size_t & y_size)
{
  assert(x_size > 0 && y_size > 0);
  char * buffer = (char*)IC_MALLOC((sizeof(T) * x_size * y_size) + 
    sizeof(tagged_array_allocation<T>));
  char * objbuffer = buffer + sizeof(tagged_array_allocation<T>);

  new(buffer) tagged_array_allocation<T>(x_size*y_size,(T*)objbuffer);

  matrix<T> mat(new (objbuffer) T[x_size * y_size],x_size,y_size);
  return mat;
}

template<typename S, typename T, typename... TArgs> 
ptr<S> 
new_variable_ptr_impl(size_t vSize, TArgs... args)
{
  char * buffer = (char*)IC_MALLOC(sizeof(S) - sizeof(T) + (vSize * sizeof(T)));
  ptr<S> ret = new (buffer) S(args...);
  char * sizeLoc = buffer + sizeof(S) - sizeof(T) - sizeof(size_t);
  size_t * size = (size_t*)sizeLoc;
  *size = vSize;
  assert(*size != 0 && buffer != 0);
  assert(buffer != 0);

  // TODO: Fix for precise collection

  return ret;
}

template<class T> 
aptr<T> 
new_aligned_array_impl(size_t size, size_t align)
{
  char * buffer = (char*)IC_MALLOC((size * sizeof(T)) + align + 
    sizeof(tagged_array_allocation<T>));

  char * objbuffer = buffer + sizeof(tagged_array_allocation<T>);

  while((size_t)objbuffer % align != 0){
    objbuffer++;
  }

  new(buffer) tagged_array_allocation<T>(size,(T*)objbuffer);

  return aptr<T>(new(objbuffer) T[size],size);
}
#endif

template <typename T1, typename T2, typename T3>
inline
T3 reduce2(aptr<T1> input1, aptr<T2> input2, 
	   const int start, const int end, 
	   T3 & init, T3 f (T1,T2)){
  T3 value = init;
  
  assert(input1.spatialCheck(start * sizeof(T2)));
  assert(input1.spatialCheck(end * sizeof(T2)));
  assert(input2.spatialCheck(start * sizeof(T3)));
  assert(input2.spatialCheck(end * sizeof(T3)));

  T1 * i1 = input1.convert_to_raw();
  T2 * i2 = input2.convert_to_raw();

  for(int i = start; i < end; ++i){
    value += f(i1[i],i2[i]);
  }

  return value;
}

template<class T> ptr<T> construct_ptr(T * new_ptr){
  return ptr<T>(new_ptr);
}

template<class T> aptr<T> construct_aptr(T * ptr, size_t size){
  return aptr<T>(ptr,size);
}

static bool isSystemLittleEndian(){
  const int test = 0;
  return *((unsigned char *)&test);
}

static aptr< aptr< char > > handleArgv(int argc, char ** argv){
  aptr< aptr < char > > newArgv(new aptr<char>[argc],argc);
  for(int c = 0; c < argc; c++){
    newArgv[c] = WRAP_LITERAL(argv[c]);
  }
  return newArgv;
}

template <class T> static ptr<T> calloc_ptr(){
  T * buffer = new T;
  memset((void*)buffer,0,sizeof(T));
  ptr<T> ret(new (buffer) T);
  return ret;
}

template <class T> static aptr<T> safe_calloc(size_t num,size_t osize){
  char * buffer = new char[num * sizeof(T)];
  memset((void*)buffer,0,num * sizeof(T));
  aptr<T> ret(new (buffer) T[num],num);
  return ret;
}

template <class T> static aptr<T> safe_realloc(aptr<T> oldPtr, size_t size){
  assert(size > 0);
  oldPtr.free();
  size_t newSize = size / sizeof(T);
  aptr<T> newPtr(new T[newSize],newSize);
  memcpy(newPtr.convert_to_void(),oldPtr.convert_to_void(),size);
  return newPtr;
}

template <class T> aptr<T> safe_mmap(aptr<T> addr, size_t len, int prot, int flags, int fildes, off_t off){
  char * buf = (char*)mmap(addr.convert_to_void(), len, prot, flags, fildes, off);
  if(len == 0){
    return NULL;
  }else{
    return aptr<T>((T*)(buf),len);
  }
}

#ifdef _ENABLE_PTHREADS
template <class T> class PthreadRunner{
public:
  ptr<T> (*routine)(ptr<T>);
  ptr<T> arg;

  void run(){
    routine(arg);
  }
};

template <class T> void * wrapper(void * arg){
  PthreadRunner<T> * runner = (PthreadRunner<T>*)arg;
  runner->run();
  return NULL;
}

template <class T> static int safe_pthread_create(ptr<pthread_t> thread, ptr<const pthread_attr_t> attr, ptr<T> (*start_routine)(ptr<T>), ptr<T> arg){
  PthreadRunner<T> * runner = new PthreadRunner<T>();
  runner->routine = start_routine;
  runner->arg = arg;
  return pthread_create(thread.convert_to_raw(), attr.convert_to_raw(), wrapper<T>, (void*)runner);
}
#endif

static aptr<const char> cstring(const char *p){
  return aptr<const char>(p,strlen(p));
}

static int safe_atoi( aptr<char> str ){
  return atoi(str.convert_to_raw());
}

static int safe_atol( aptr< char > str ){
  return atol(str.convert_to_raw());
}

static float safe_atof( aptr< char > str ){
  return atof(str.convert_to_raw());
}

static int safe_vfprintf( FILE * stream, const char * format, va_list arglist ){
  return vfprintf( stream, format, arglist );
}

static int safe_vfprintf( ptr<FILE> stream, const char * format, va_list arglist ){
  return vfprintf( stream.convert_to_raw(), format, arglist );
}

static int safe_vfprintf( aptr<FILE> stream, const char * format, va_list arglist ){
  return vfprintf( stream.convert_to_raw(), format, arglist );
}

static int safe_vfprintf( ptr<FILE> stream, ptr<const char> format, va_list arglist ){
  return vfprintf( stream.convert_to_raw(), format.convert_to_raw(), arglist );
}

static int safe_vfprintf( aptr<FILE> stream, aptr<const char> format, va_list arglist ){
  return vfprintf( stream.convert_to_raw(), format.convert_to_raw(), arglist );
}

static int safe_fscanf( FILE * stream, const char * format, ... ){
  int total = 0;
  va_list arglist;
  va_start( arglist, format );
  total += vfscanf( stream, format, arglist );
  va_end( arglist );
  return total;
}

static int safe_fscanf( ironclad::ptr<FILE> stream, const char * format, ... ){
  int total = 0;
  va_list arglist;
  va_start( arglist, format );
  total += vfscanf( stream.convert_to_raw(), format, arglist );
  va_end( arglist );
  return total;
}

static int safe_fscanf( ironclad::ptr<FILE> stream, ironclad::ptr<const char> format, ... ){
  int total = 0;
  va_list arglist;
  va_start( arglist, format );
  total += vfscanf( stream.convert_to_raw(), format.convert_to_raw(), arglist );
  va_end( arglist );
  return total;
}

// Specific case of fscanf that otherwise doesn't work

#ifndef SAFE_NO_LOCAL
static int safe_fscanf( ironclad::ptr<FILE> stream, const char * format, ironclad::laptr<char> str ){
#else
static int safe_fscanf( ironclad::ptr<FILE> stream, const char * format, ironclad::aptr<char> str ){
#endif
  return fscanf(stream.convert_to_raw(),format,str.convert_to_raw());
} 

}

#include <ironclad/safe_string.hpp>
