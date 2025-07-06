/*
 -------------------------------------------------------------------------
 Copyright (c) 2002, Markus Lagler <markus.lagler@trivadis.com>
 Copyright (c) 2002, Tim Tassonis  <timtas@dplanet.ch>

 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products
      built using this software without specific written permission.

 DISCLAIMER

 This software is provided 'as is' with no explcit or implied warranties
 in respect of any properties, including, but not limited to, correctness
 and fitness for purpose.
 -------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "aes.h"


#define BLOCK_LEN   16
#define FIX_KEY_LEN 16
#define MAX_IN_LEN  16

#ifdef __cplusplus
extern "C"{
#endif

char* aes_enc(char *in, int in_len, char *key, int key_len, int* out_len);
char* aes_dec(char *in, int in_len, char *key, int key_len, int* out_len);
/*Added by xuhq for multithread safe */
int aes_enc_r(char *in, int in_len, char *key, int key_len, char *out, int* out_len);
int aes_dec_r(char *in, int in_len, char *key, int key_len, char *out, int* out_len);

#ifdef __cplusplus
}
#endif
