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
 * The aptr<T> class template provides "array pointer" functionality for Ironclad
 * C++ programs.  Unlike ptr<T>, aptr<T> allows indexing and pointer arithmetic
 * but requires a bounds check on all dereferences.  aptr<T> can also convert
 * to a ptr<T> following a bounds check on the current index.  aptr<T> is only 
 * allowed to point to heap and global memory locations (i.e., aptr<T> cannot
 * point to the stack).
 */

#ifdef _HAVE_BDW_GC
#include <gc.h>
#endif

#include "common.hpp"
#include "ptr.hpp"
#include "laptr.hpp"
#include <cstdlib>
#include <cassert>

#include "tags.hpp"

#ifdef _ENABLE_PRECISE_GC
extern void ironclad_precise_mark(void* pointer, void** source);
#endif

namespace safe{

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T, int B = 0> class aptr
#ifdef _ENABLE_PRECISE_GC
: public IroncladPreciseGC
#endif
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;
  long index;
  long size;

public:
  template<typename,int> friend class aptr;
  template<typename,size_t> friend class array;
  template<typename S> friend aptr<S> new_array(size_t _size);

  aptr() : data(NULL), index(0), size(0) {
  }

  aptr(std::nullptr_t) : data(NULL), index(0), size(0) {}

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data), index(0), size(_size) {
  }

  aptr(T * _data, long _size, long _index) : data(_data), index(_index), size(_size){
  }

  ~aptr() {}

  aptr(aptr & other) : data(other.data), index(other.index), size(other.size) {
  }

  aptr(aptr const & other) : data(other.data), index(other.index), size(other.size) {
  }

  template <class U> aptr(aptr<U,B> & other) : data(const_cast<T*>(other.data)), 
						 index(other.index), 
						 size(other.size) {
  }

  template <class U> aptr(const aptr<U,B> & other) : data(const_cast<T*>(other.data)), 
						       index(other.index), 
						       size(other.size) {
  }

  typedef T * iterator;

  T * begin(){
    return data + index;
  }

  T * end(){
    return data + size;
  }

  void mark() const 
  {
#ifdef _ENABLE_PRECISE_GC
    if(data != nullptr){
      ironclad_precise_mark((void*)data,(void**)&data);
    }
#endif
  }

  static aptr<T,B> pointer_to(T & element){
    return aptr<T,B>(&element);
  }

  long getSize() const {
    return size;
  }

  long getIndex() const {
    return index;
  }

  /*
   * The programmer must explicitly request that an aptr be created from an 
   * laptr because the common case is that an laptr refers to a stack location 
   * and will not be allowed to be stored in an aptr
   */

  void from_laptr(laptr<T> & other){
    data = other.data;
    index = other.index;
    size = other.size;
    STACK_CHECK
  }

  void from_laptr(laptr<T> const & other){
    data = other.data;
    index = other.index;
    size = other.size;
    STACK_CHECK
  }

  inline T* operator-> () 
    __attribute__((always_inline))
  {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return data + index;
  }

  inline T& operator* () 
    __attribute__((always_inline))
  {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return *(data + index);
  }

  inline T* operator-> () const 
    __attribute__((always_inline))
  {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return data + index;
  }

  inline T& operator* () const 
    __attribute__((always_inline))
  {
    ARRAY_NULL_CHECK
    BOUNDS_CHECK
    return *(data + index);
  }

  inline aptr<T,B>& operator++() {
    ++index;
    return *this;
  }
  
  inline aptr<T,B>& operator--() {
    --index;
    return *this;
  }

  inline aptr<T,B> operator++(int) {
    aptr<T> ret(*this);
    index++;
    return ret;
  }

  inline aptr<T,B> operator--(int) {
    aptr<T> ret(*this);
    index--;
    return ret;
  }

  inline aptr<T,B> & operator+= (int op) {
    index += op;
    return *this;
  }

  inline aptr<T,B>& operator-= (int op) {
    index -= op;
    return *this;
  }

  inline std::ptrdiff_t operator- (aptr<T,B> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline std::ptrdiff_t operator- (const aptr<T,B> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline aptr<T,B> operator- (size_t diff) const {
    aptr<T,B> new_ptr(data,size);
    new_ptr.index = index - diff;
    return new_ptr;
  }

  inline aptr<T,B> operator+ (size_t diff) const {
    aptr<T,B> new_ptr(data,size);
    new_ptr.index = index + diff;
    return new_ptr;
  }

  inline aptr<T,B>& operator= (const aptr<T,B>& other) {
    data = other.data;
    size = other.size;
    index = other.index;
    return *this;
  }

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    BOUNDS_CHECK_INDEX(_index)
    return data[index + _index];
  }

  inline void free() const {
#ifndef _HAVE_BDW_GC
    std::free(static_cast<void*>(data));
#endif
    data = NULL;
  }

  /*
   * These functions should only be used for compatability with non-Ironclad code!
   */

  inline T * convert_to_raw() const {
    return const_cast<T*>(data + index);
  }

  inline T * getData() const {
    return data;
  }

  inline void * convert_to_void() const {
    return (void*)(data + index);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data + index);
  }

  /*
   * Applies a spatial safety check for functions, such as memcpy
   */

  inline bool spatialCheck(size_t numBytes) const {
    return (index + (long)(numBytes / sizeof(T)) >= 0 && index + (numBytes / sizeof(T)) <= size);
  }

  typedef T * aptr::*unspecified_bool_type;
  
  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &aptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  inline aptr<T,B> offset(unsigned int _index) const {
    aptr<T,B> newPtr(data,size);
    newPtr.index = index + _index;
    return newPtr;
  }

  inline operator ptr<T>() const {
    // Need to ensure that the index is currently in bounds to allow 
    // conversion to a singleton pointer
    if(data != NULL){
      BOUNDS_CHECK
    }
    ptr<T> newPtr(data + index);
    return newPtr;
  }

  inline operator laptr<T>() const {
    laptr<T> newPtr(data,size,index);
    newPtr.tb |= 1;
    return newPtr;
  }

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1, const S* aptr2);
  template<typename S, int C> friend bool operator== (const S* aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1, const S * aptr2);
  template<typename S, int C> friend bool operator!= (const S * aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator> (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator>= (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator< (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator<= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,1>
#ifdef _ENABLE_PRECISE_GC
: public IroncladPreciseGC
#endif
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;
  long index;

public:
  template<typename,int> friend class aptr;
  template<typename,size_t> friend class array;
  template<typename S> friend aptr<S,1> new_array_1(size_t _size);

  aptr() : data(NULL), index(0) {
    // TODO: Set null bound
  }

  aptr(std::nullptr_t) : data(NULL), index(0) {
    // TODO: Set null bound
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data), index(0) {
    asm volatile("movq %0, %%rax"
                 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
  }

  aptr(T * _data, long _size, long _index) : data(_data), index(_index) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
  }

  ~aptr() {
    // TODO: Clear bounds
  }

  aptr(aptr & other) : data(other.data), index(other.index) {
    // Don't need to remake bounds
    // From the same pointer (assuming bounds are not mixing)
  }

  aptr(aptr const & other) : data(other.data), index(other.index) {
    // Don't need to remake bounds
  }

  template <class U> aptr(aptr<U> & other) : data(const_cast<T*>(other.data)), 
						 index(other.index) {
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
  }

  template <class U> aptr(const aptr<U> & other) : data(const_cast<T*>(other.data)), 
						       index(other.index) {
    long size = other.size;
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
  }

  // TODO: Fix iterators to use bounds encoding

  void mark() const 
  {
#ifdef _ENABLE_PRECISE_GC
    if(data != nullptr){
      ironclad_precise_mark((void*)data,(void**)&data);
    }
#endif
  }

  static aptr<T,1> pointer_to(T & element){
    return aptr<T,1>(&element);
  }

  long getIndex() const {
    return index;
  }

  /*
   * The programmer must explicitly request that an aptr be created from an 
   * laptr because the common case is that an laptr refers to a stack location 
   * and will not be allowed to be stored in an aptr
   */

  void from_laptr(laptr<T> & other){
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  void from_laptr(laptr<T> const & other){
    // TODO: set bounds
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd0");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  inline T* operator-> () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd0");
    asm volatile("bndcu %rax, %bnd0");
    return data + index;
  }

  inline T& operator* () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd0");
    asm volatile("bndcu %rax, %bnd0");
    return *(data + index);
  }

  inline T* operator-> () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd0");
    asm volatile("bndcu %rax, %bnd0");
    return data + index;
  }

  inline T& operator* () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd0");
    asm volatile("bndcu %rax, %bnd0");
    return *(data + index);
  }

  inline aptr<T,1>& operator++() {
    ++index;
    return *this;
  }
  
  inline aptr<T,1>& operator--() {
    --index;
    return *this;
  }

  inline aptr<T,1> operator++(int) {
    aptr<T,1> ret(*this);
    index++;
    return ret;
  }

  inline aptr<T,1> operator--(int) {
    aptr<T,1> ret(*this);
    index--;
    return ret;
  }

  inline aptr<T,1> & operator+= (int op) {
    index += op;
    return *this;
  }

  inline aptr<T,1>& operator-= (int op) {
    index -= op;
    return *this;
  }

  inline std::ptrdiff_t operator- (aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline std::ptrdiff_t operator- (const aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline aptr<T,1> operator- (size_t diff) const {
    aptr<T,1> new_ptr(*this);
    new_ptr.index = index - diff;
    return new_ptr;
  }

  inline aptr<T,1> operator+ (size_t diff) const {
    aptr<T,1> new_ptr(*this);
    new_ptr.index = index + diff;
    return new_ptr;
  }

  inline aptr<T,1>& operator= (const aptr<T,1>& other) {
    data = other.data;
    index = other.index;
    // No need to set size since this is only allowed to be the same pointer
    return *this;
  }

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + index + _index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd0");
    asm volatile("bndcu %rax, %bnd0");
    return data[index + _index];
  }

  inline void free() const {
#ifndef _HAVE_BDW_GC
    std::free(static_cast<void*>(data));
#endif
    data = NULL;
  }

  /*
   * These functions should only be used for compatability with non-Ironclad code!
   */

  inline T * convert_to_raw() const {
    return const_cast<T*>(data + index);
  }

  inline T * getData() const {
    return data;
  }

  inline void * convert_to_void() const {
    return (void*)(data + index);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data + index);
  }

  /*
   * Applies a spatial safety check for functions, such as memcpy
   */

  inline bool spatialCheck(size_t numBytes) const {
    char *addr = (char*)data + numBytes;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    // TODO: Check lower too
    asm volatile("bndcu %rax, %bnd0");
    return true;
  }

  typedef T * aptr::*unspecified_bool_type;
  
  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &aptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  inline aptr<T,1> offset(unsigned int _index) const {
    aptr<T,1> newPtr(*this);
    newPtr.index = index + _index;
    return newPtr;
  }

  inline operator ptr<T>() const {
    // Need to ensure that the index is currently in bounds to allow 
    // conversion to a singleton pointer
    if(data != NULL){
      T *addr = data + index;
      asm volatile("movq %0, %%rax"
		   : "=r" (addr)
		   :: "rax");
      asm volatile("bndcl %rax, %bnd0");
      asm volatile("bndcu %rax, %bnd0");
    }
    ptr<T> newPtr(data + index);
    return newPtr;
  }

  inline operator laptr<T>() const {
    laptr<T> newPtr(*this);
    newPtr.tb |= 1;
    return newPtr;
  }

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1, const S* aptr2);
  template<typename S, int C> friend bool operator== (const S* aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1, const S * aptr2);
  template<typename S, int C> friend bool operator!= (const S * aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator> (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator>= (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator< (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator<= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,2>
#ifdef _ENABLE_PRECISE_GC
: public IroncladPreciseGC
#endif
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;
  long index;

public:
  template<typename,int> friend class aptr;
  template<typename,size_t> friend class array;
  template<typename S> friend aptr<S,2> new_array_2(size_t _size);

  aptr() : data(NULL), index(0) {
    // TODO: Set null bound
  }

  aptr(std::nullptr_t) : data(NULL), index(0) {
    // TODO: Set null bound
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data), index(0) {
    asm volatile("movq %0, %%rax"
                 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
  }

  aptr(T * _data, long _size, long _index) : data(_data), index(_index) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
  }

  ~aptr() {
    // TODO: Clear bounds
  }

  aptr(aptr & other) : data(other.data), index(other.index) {
    /// NOP
  }

  aptr(aptr const & other) : data(other.data), index(other.index) {
    // NOP
  }

  template <class U> aptr(aptr<U> & other) : data(const_cast<T*>(other.data)), 
						 index(other.index) {
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
  }

  template <class U> aptr(const aptr<U> & other) : data(const_cast<T*>(other.data)), 
						       index(other.index) {
    long size = other.size;
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
  }

  // TODO: Fix iterators to use bounds encoding

  void mark() const 
  {
#ifdef _ENABLE_PRECISE_GC
    if(data != nullptr){
      ironclad_precise_mark((void*)data,(void**)&data);
    }
#endif
  }

  static aptr<T,2> pointer_to(T & element){
    return aptr<T,2>(&element);
  }

  long getIndex() const {
    return index;
  }

  /*
   * The programmer must explicitly request that an aptr be created from an 
   * laptr because the common case is that an laptr refers to a stack location 
   * and will not be allowed to be stored in an aptr
   */

  void from_laptr(laptr<T> & other){
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  void from_laptr(laptr<T> const & other){
    // TODO: set bounds
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd1");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  inline T* operator-> () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd1");
    asm volatile("bndcu %rax, %bnd1");
    return data + index;
  }

  inline T& operator* () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd1");
    asm volatile("bndcu %rax, %bnd1");
    return *(data + index);
  }

  inline T* operator-> () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd1");
    asm volatile("bndcu %rax, %bnd1");
    return data + index;
  }

  inline T& operator* () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd1");
    asm volatile("bndcu %rax, %bnd1");
    return *(data + index);
  }

  inline aptr<T,2>& operator++() {
    ++index;
    return *this;
  }
  
  inline aptr<T,2>& operator--() {
    --index;
    return *this;
  }

  inline aptr<T,2> operator++(int) {
    aptr<T,2> ret(*this);
    index++;
    return ret;
  }

  inline aptr<T,2> operator--(int) {
    aptr<T,2> ret(*this);
    index--;
    return ret;
  }

  inline aptr<T,2> & operator+= (int op) {
    index += op;
    return *this;
  }

  inline aptr<T,2>& operator-= (int op) {
    index -= op;
    return *this;
  }

  inline std::ptrdiff_t operator- (aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline std::ptrdiff_t operator- (const aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline aptr<T,2> operator- (size_t diff) const {
    aptr<T,2> new_ptr(*this);
    new_ptr.index = index - diff;
    return new_ptr;
  }

  inline aptr<T,2> operator+ (size_t diff) const {
    aptr<T,2> new_ptr(*this);
    new_ptr.index = index + diff;
    return new_ptr;
  }

  inline aptr<T,2>& operator= (const aptr<T,2>& other) {
    data = other.data;
    index = other.index;
    // No need to set size since this is only allowed to be the same pointer
    return *this;
  }

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + index + _index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd1");
    asm volatile("bndcu %rax, %bnd1");
    return data[index + _index];
  }

  inline void free() const {
#ifndef _HAVE_BDW_GC
    std::free(static_cast<void*>(data));
#endif
    data = NULL;
  }

  /*
   * These functions should only be used for compatability with non-Ironclad code!
   */

  inline T * convert_to_raw() const {
    return const_cast<T*>(data + index);
  }

  inline T * getData() const {
    return data;
  }

  inline void * convert_to_void() const {
    return (void*)(data + index);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data + index);
  }

  /*
   * Applies a spatial safety check for functions, such as memcpy
   */

  inline bool spatialCheck(size_t numBytes) const {
    char *addr = (char*)data + numBytes;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    // TODO: Check lower too
    asm volatile("bndcu %rax, %bnd1");
    return true;
  }

  typedef T * aptr::*unspecified_bool_type;
  
  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &aptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  inline aptr<T,2> offset(unsigned int _index) const {
    aptr<T,2> newPtr(*this);
    newPtr.index = index + _index;
    return newPtr;
  }

  inline operator ptr<T>() const {
    // Need to ensure that the index is currently in bounds to allow 
    // conversion to a singleton pointer
    if(data != NULL){
      T *addr = data + index;
      asm volatile("movq %0, %%rax"
		   : "=r" (addr)
		   :: "rax");
      asm volatile("bndcl %rax, %bnd1");
      asm volatile("bndcu %rax, %bnd1");
    }
    ptr<T> newPtr(data + index);
    return newPtr;
  }

  inline operator laptr<T>() const {
    laptr<T> newPtr(*this);
    newPtr.tb |= 1;
    return newPtr;
  }

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1, const S* aptr2);
  template<typename S, int C> friend bool operator== (const S* aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1, const S * aptr2);
  template<typename S, int C> friend bool operator!= (const S * aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator> (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator>= (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator< (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator<= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,3>
#ifdef _ENABLE_PRECISE_GC
: public IroncladPreciseGC
#endif
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;
  long index;

public:
  template<typename,int> friend class aptr;
  template<typename,size_t> friend class array;
  template<typename S> friend aptr<S,3> new_array_3(size_t _size);

  aptr() : data(NULL), index(0) {
    // TODO: Set null bound
  }

  aptr(std::nullptr_t) : data(NULL), index(0) {
    // TODO: Set null bound
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data), index(0) {
    asm volatile("movq %0, %%rax"
                 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
  }

  aptr(T * _data, long _size, long _index) : data(_data), index(_index) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
  }

  ~aptr() {
    // TODO: Clear bounds
  }

  aptr(aptr & other) : data(other.data), index(other.index) {
    // Don't need to move bounds (same pointer)
  }

  aptr(aptr const & other) : data(other.data), index(other.index) {
    // NOP
  }

  template <class U> aptr(aptr<U> & other) : data(const_cast<T*>(other.data)), 
						 index(other.index) {
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
  }

  template <class U> aptr(const aptr<U> & other) : data(const_cast<T*>(other.data)), 
						       index(other.index) {
    long size = other.size;
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
  }

  // TODO: Fix iterators to use bounds encoding

  void mark() const 
  {
#ifdef _ENABLE_PRECISE_GC
    if(data != nullptr){
      ironclad_precise_mark((void*)data,(void**)&data);
    }
#endif
  }

  static aptr<T,3> pointer_to(T & element){
    return aptr<T,3>(&element);
  }

  long getIndex() const {
    return index;
  }

  /*
   * The programmer must explicitly request that an aptr be created from an 
   * laptr because the common case is that an laptr refers to a stack location 
   * and will not be allowed to be stored in an aptr
   */

  void from_laptr(laptr<T> & other){
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  void from_laptr(laptr<T> const & other){
    // TODO: set bounds
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd2");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  inline T* operator-> () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd2");
    asm volatile("bndcu %rax, %bnd2");
    return data + index;
  }

  inline T& operator* () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd2");
    asm volatile("bndcu %rax, %bnd2");
    return *(data + index);
  }

  inline T* operator-> () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd2");
    asm volatile("bndcu %rax, %bnd2");
    return data + index;
  }

  inline T& operator* () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd2");
    asm volatile("bndcu %rax, %bnd2");
    return *(data + index);
  }

  inline aptr<T,3>& operator++() {
    ++index;
    return *this;
  }
  
  inline aptr<T,3>& operator--() {
    --index;
    return *this;
  }

  inline aptr<T,3> operator++(int) {
    aptr<T,3> ret(*this);
    index++;
    return ret;
  }

  inline aptr<T,3> operator--(int) {
    aptr<T,3> ret(*this);
    index--;
    return ret;
  }

  inline aptr<T,3> & operator+= (int op) {
    index += op;
    return *this;
  }

  inline aptr<T,3>& operator-= (int op) {
    index -= op;
    return *this;
  }

  inline std::ptrdiff_t operator- (aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline std::ptrdiff_t operator- (const aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline aptr<T,3> operator- (size_t diff) const {
    aptr<T,3> new_ptr(*this);
    new_ptr.index = index - diff;
    return new_ptr;
  }

  inline aptr<T,3> operator+ (size_t diff) const {
    aptr<T,3> new_ptr(*this);
    new_ptr.index = index + diff;
    return new_ptr;
  }

  inline aptr<T,3>& operator= (const aptr<T,3>& other) {
    data = other.data;
    index = other.index;
    // No need to set size since this is only allowed to be the same pointer
    return *this;
  }

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + index + _index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd2");
    asm volatile("bndcu %rax, %bnd2");
    return data[index + _index];
  }

  inline void free() const {
#ifndef _HAVE_BDW_GC
    std::free(static_cast<void*>(data));
#endif
    data = NULL;
  }

  /*
   * These functions should only be used for compatability with non-Ironclad code!
   */

  inline T * convert_to_raw() const {
    return const_cast<T*>(data + index);
  }

  inline T * getData() const {
    return data;
  }

  inline void * convert_to_void() const {
    return (void*)(data + index);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data + index);
  }

  /*
   * Applies a spatial safety check for functions, such as memcpy
   */

  inline bool spatialCheck(size_t numBytes) const {
    char *addr = (char*)data + numBytes;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    // TODO: Check lower too
    asm volatile("bndcu %rax, %bnd2");
    return true;
  }

  typedef T * aptr::*unspecified_bool_type;
  
  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &aptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  inline aptr<T,3> offset(unsigned int _index) const {
    aptr<T,3> newPtr(*this);
    newPtr.index = index + _index;
    return newPtr;
  }

  inline operator ptr<T>() const {
    // Need to ensure that the index is currently in bounds to allow 
    // conversion to a singleton pointer
    if(data != NULL){
      T *addr = data + index;
      asm volatile("movq %0, %%rax"
		   : "=r" (addr)
		   :: "rax");
      asm volatile("bndcl %rax, %bnd2");
      asm volatile("bndcu %rax, %bnd2");
    }
    ptr<T> newPtr(data + index);
    return newPtr;
  }

  inline operator laptr<T>() const {
    laptr<T> newPtr(*this);
    newPtr.tb |= 1;
    return newPtr;
  }

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1, const S* aptr2);
  template<typename S, int C> friend bool operator== (const S* aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1, const S * aptr2);
  template<typename S, int C> friend bool operator!= (const S * aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator> (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator>= (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator< (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator<= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
};

// Template paramter int B is the bounds register to use (or 0 for none)
template <class T> class aptr<T,4>
#ifdef _ENABLE_PRECISE_GC
: public IroncladPreciseGC
#endif
{
  // data is mutable to allow destroy() (which is
  // a const method) to set data to null
  mutable T * data;
  long index;

public:
  template<typename,int> friend class aptr;
  template<typename,size_t> friend class array;
  template<typename S> friend aptr<S,4> new_array_4(size_t _size);

  aptr() : data(NULL), index(0) {
    // TODO: Set null bound
  }

  aptr(std::nullptr_t) : data(NULL), index(0) {
    // TODO: Set null bound
  }

  // Constructor used by new_array to create an aptr with a size bound
  aptr(T * _data, long _size) : data(_data), index(0) {
    asm volatile("movq %0, %%rax"
                 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd3");
  }

  aptr(T * _data, long _size, long _index) : data(_data), index(_index) {
    asm volatile("movq %0, %%rax"
		 : "=r" (_data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (_size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd3");
  }

  ~aptr() {
    // TODO: Clear bounds
  }

  aptr(aptr & other) : data(other.data), index(other.index) {
    // NOP
  }

  aptr(aptr const & other) : data(other.data), index(other.index) {
    // NOP
  }

  template <class U> aptr(aptr<U> & other) : data(const_cast<T*>(other.data)), 
						 index(other.index) {
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd3");
  }

  template <class U> aptr(const aptr<U> & other) : data(const_cast<T*>(other.data)), 
						       index(other.index) {
    long size = other.size;
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd3");
  }

  // TODO: Fix iterators to use bounds encoding

  void mark() const 
  {
#ifdef _ENABLE_PRECISE_GC
    if(data != nullptr){
      ironclad_precise_mark((void*)data,(void**)&data);
    }
#endif
  }

  static aptr<T,4> pointer_to(T & element){
    return aptr<T,4>(&element);
  }

  long getIndex() const {
    return index;
  }

  /*
   * The programmer must explicitly request that an aptr be created from an 
   * laptr because the common case is that an laptr refers to a stack location 
   * and will not be allowed to be stored in an aptr
   */

  void from_laptr(laptr<T> & other){
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd3");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  void from_laptr(laptr<T> const & other){
    // TODO: set bounds
    asm volatile("movq %0, %%rax"
                 : "=r" (other.data)
                 :: "rax");
    asm volatile("movq %0, %%rdx"
                 : "=r" (other.size)
                 :: "rdx");
    asm volatile("bndmk (%rax,%rdx), %bnd3");
    data = other.data;
    index = other.index;
    STACK_CHECK
  }

  inline T* operator-> () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd3");
    asm volatile("bndcu %rax, %bnd3");
    return data + index;
  }

  inline T& operator* () 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd3");
    asm volatile("bndcu %rax, %bnd3");
    return *(data + index);
  }

  inline T* operator-> () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd3");
    asm volatile("bndcu %rax, %bnd3");
    return data + index;
  }

  inline T& operator* () const 
    __attribute__((always_inline))
  {
    T *addr = data + index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd3");
    asm volatile("bndcu %rax, %bnd3");
    return *(data + index);
  }

  inline aptr<T,4>& operator++() {
    ++index;
    return *this;
  }
  
  inline aptr<T,4>& operator--() {
    --index;
    return *this;
  }

  inline aptr<T,4> operator++(int) {
    aptr<T,4> ret(*this);
    index++;
    return ret;
  }

  inline aptr<T,4> operator--(int) {
    aptr<T,4> ret(*this);
    index--;
    return ret;
  }

  inline aptr<T,4> & operator+= (int op) {
    index += op;
    return *this;
  }

  inline aptr<T,4>& operator-= (int op) {
    index -= op;
    return *this;
  }

  inline std::ptrdiff_t operator- (aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline std::ptrdiff_t operator- (const aptr<T> & other) const {
    return (this->data + this->index) - (other.data + other.index);
  }

  inline aptr<T,4> operator- (size_t diff) const {
    aptr<T,4> new_ptr(*this);
    new_ptr.index = index - diff;
    return new_ptr;
  }

  inline aptr<T,4> operator+ (size_t diff) const {
    aptr<T,4> new_ptr(*this);
    new_ptr.index = index + diff;
    return new_ptr;
  }

  inline aptr<T,4>& operator= (const aptr<T,4>& other) {
    data = other.data;
    index = other.index;
    // No need to set size since this is only allowed to be the same pointer
    return *this;
  }

  inline T& operator[] (const int _index) const 
    __attribute__((always_inline)) 
  {
    T *addr = data + index + _index;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    asm volatile("bndcl %rax, %bnd3");
    asm volatile("bndcu %rax, %bnd3");
    return data[index + _index];
  }

  inline void free() const {
#ifndef _HAVE_BDW_GC
    std::free(static_cast<void*>(data));
#endif
    data = NULL;
  }

  /*
   * These functions should only be used for compatability with non-Ironclad code!
   */

  inline T * convert_to_raw() const {
    return const_cast<T*>(data + index);
  }

  inline T * getData() const {
    return data;
  }

  inline void * convert_to_void() const {
    return (void*)(data + index);
  }

  inline unsigned long convert_to_long() const {
    return reinterpret_cast<unsigned long>(data + index);
  }

  /*
   * Applies a spatial safety check for functions, such as memcpy
   */

  inline bool spatialCheck(size_t numBytes) const {
    char *addr = (char*)data + numBytes;
    asm volatile("movq %0, %%rax"
                 : "=r" (addr)
                 :: "rax");
    // TODO: Check lower too
    asm volatile("bndcu %rax, %bnd3");
    return true;
  }

  typedef T * aptr::*unspecified_bool_type;
  
  inline operator unspecified_bool_type() const {
    return data == 0 ? 0 : &aptr::data;
  }

  inline bool operator! () const {
      return data == NULL;
  }

  inline aptr<T,4> offset(unsigned int _index) const {
    aptr<T,4> newPtr(*this);
    newPtr.index = index + _index;
    return newPtr;
  }

  inline operator ptr<T>() const {
    // Need to ensure that the index is currently in bounds to allow 
    // conversion to a singleton pointer
    if(data != NULL){
      T *addr = data + index;
      asm volatile("movq %0, %%rax"
		   : "=r" (addr)
		   :: "rax");
      asm volatile("bndcl %rax, %bnd3");
      asm volatile("bndcu %rax, %bnd3");
    }
    ptr<T> newPtr(data + index);
    return newPtr;
  }

  inline operator laptr<T>() const {
    laptr<T> newPtr(*this);
    newPtr.tb |= 1;
    return newPtr;
  }

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1,const aptr<const S, C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator!= (const aptr<const S, C>& aptr1,const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator== (const aptr<S,C>& aptr1, const S* aptr2);
  template<typename S, int C> friend bool operator== (const S* aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator!= (const aptr<S,C>& aptr1, const S * aptr2);
  template<typename S, int C> friend bool operator!= (const S * aptr1, const aptr<S,C>& aptr2);

  template<typename S, int C> friend bool operator> (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator>= (const aptr<S,C>& aptr1, const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator< (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
  template<typename S, int C> friend bool operator<= (const aptr<S,C>& aptr1,const aptr<S,C>& aptr2);
};

template<class T, int B> inline bool operator== (const aptr<T,B>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) == (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator== (const aptr<const T>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) == (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator== (const aptr<T,B>& aptr1,const aptr<const T>& aptr2){
  return (aptr1.data + aptr1.index) == (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator== (const aptr<T,B>& aptr1, const T * aptr2){
  return (aptr1.data + aptr1.index) == aptr2;
}

template<class T, int B> inline bool operator== (const T * aptr1, const aptr<T,B>& aptr2){
  return aptr1 == (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator!= (const aptr<T,B>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) != (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator!= (const aptr<const T>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) != (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator!= (const aptr<T,B>& aptr1,const aptr<const T>& aptr2){
  return (aptr1.data + aptr1.index) != (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator!= (const aptr<T,B>& aptr1, const T * aptr2){
  return (aptr1.data + aptr1.index) != aptr2;
}

template<class T, int B> inline bool operator!= (const T * aptr1, const aptr<T,B>& aptr2){
  return aptr1 != (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator< (const aptr<T,B>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) < (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator<= (const aptr<T,B>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) <= (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator> (const aptr<T,B>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) > (aptr2.data + aptr2.index);
}

template<class T, int B> inline bool operator>= (const aptr<T,B>& aptr1,const aptr<T,B>& aptr2){
  return (aptr1.data + aptr1.index) >= (aptr2.data + aptr2.index);
}

}
