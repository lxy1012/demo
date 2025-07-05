
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
 Issue Date: 21/01/2002

 This file contains the definitions required to use AES (Rijndael) in C.
*/

#ifndef _AES_H
#define _AES_H

#include "uitypes.h"

/*  BLOCK_SIZE is in BYTES: 16, 24, 32 or undefined for aes.c and 16, 20, 24, 28, 
    32 or undefined for aespp.c.  If left undefined a  slower version providing 
    variable block length is compiled    
*/

#define BLOCK_SIZE  16

/* key schedule length (in 32-bit words)    */

#if !defined(BLOCK_SIZE)
#define KS_LENGTH   128
#else
#define KS_LENGTH   4 * BLOCK_SIZE
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

typedef uint16_t    aes_fret;   /* type for function return value       */
#define aes_bad     0           /* bad function return value            */
#define aes_good    1           /* good function return value           */
#ifndef AES_DLL                 /* implement normal or DLL functions    */
#define aes_rval    aes_fret
#else
#define aes_rval    aes_fret __declspec(dllexport) _stdcall
#endif

typedef struct                      /* the AES context for encryption   */
{   uint32_t    k_sch[KS_LENGTH];   /* the encryption key schedule      */
    uint32_t    n_rnd;              /* the number of cipher rounds      */
    uint32_t    n_blk;              /* the number of bytes in the state */
} aes_ctx;

aes_rval aes_blk_len(unsigned int blen, aes_ctx cx[1]);

aes_rval aes_enc_key(const unsigned char in_key[], unsigned int klen, aes_ctx cx[1]);
aes_rval aes_enc_blk(const unsigned char in_blk[], unsigned char out_blk[], const aes_ctx cx[1]);

aes_rval aes_dec_key(const unsigned char in_key[], unsigned int klen, aes_ctx cx[1]);
aes_rval aes_dec_blk(const unsigned char in_blk[], unsigned char out_blk[], const aes_ctx cx[1]);

#if defined(__cplusplus)
}
#endif

#endif
