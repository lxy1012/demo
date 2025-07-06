
/*
 -------------------------------------------------------------------------
 Copyright (c) 2001, Dr Brian Gladman <brg@gladman.uk.net>, Worcester, UK.
 All rights reserved.

 TERMS

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted subject to the following conditions:

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer. 

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution. 

  3. The copyright holder's name must not be used to endorse or promote 
     any products derived from this software without his specific prior 
     written permission. 

 This software is provided 'as is' with no express or implied warranties 
 of correctness or fitness for purpose.
 -------------------------------------------------------------------------
 Issue Date: 01/02/2002

 This file contains code to obtain or set the definitions for fixed length 
 unsigned integer types.
*/

#ifndef _UITYPES_H
#define _UITYPES_H

#if defined(__GNU_LIBRARY__)
#define HAS_INTTYPES_H
#elif !defined(_MSC_VER)
#include <limits.h>
#if ULONG_MAX > 0xFFFFFFFFUL
  #define MODEL_64
#else
  #define MODEL_32
#endif
#endif

#if defined HAS_INTTYPES_H
#include <inttypes.h>
#define s_u32     u
#define s_u64   ull
#elif defined MODEL_32
typedef unsigned char            uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;
typedef unsigned long long int  uint64_t;
#define s_u32     u
#define s_u64   ull
#elif defined MODEL_64
typedef unsigned char            uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;
typedef unsigned long int       uint64_t;
#define s_u32     u
#define s_u64    ul
#elif defined(_MSC_VER)
typedef unsigned  __int8         uint8_t;
typedef unsigned __int16        uint16_t;
typedef unsigned __int32        uint32_t;
typedef unsigned __int64        uint64_t;
#define s_u32    ui32
#define s_u64    ui64
#else
#error You need to define fixed length types in uitypes.h
#endif

#define sfx_lo(x,y) x##y
#define sfx_hi(x,y) sfx_lo(x,y)
#define x_32(p)     sfx_hi(0x##p,s_u32)
#define x_64(p)     sfx_hi(0x##p,s_u64)

#endif
