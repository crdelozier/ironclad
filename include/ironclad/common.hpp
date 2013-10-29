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
 * This file is no longer very necessary.  It was originally used to turn 
 * checking mechanisms on and off for gathering performance data.
 */

#define NULL_CHECK assert(data != NULL);

/*
 * ARRAY_NULL CHECK is never used because it was only for testing 
 * null checking overhead (without bounds checks)
 * The bounds check below covers a null check because a 
 * null aptr always has size == 0
 */

#define ARRAY_NULL_CHECK

#define BOUNDS_CHECK assert(index >= 0 && index < size);

#define BOUNDS_CHECK_INDEX(idx) assert(index + idx >= 0 && index + idx < size);

#define STACK_CHECK assert(other.notOnStack());
