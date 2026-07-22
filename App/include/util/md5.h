/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See md5.c for more information.
 */

#ifndef RBX_UTIL_MD5_H
#define RBX_UTIL_MD5_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t RBX_MD5_u32plus;

typedef struct {
	RBX_MD5_u32plus lo, hi;
	RBX_MD5_u32plus a, b, c, d;
	unsigned char buffer[64];
	RBX_MD5_u32plus block[16];
} RBX_MD5_CTX;

#ifdef __cplusplus
extern "C" 
{
#endif

extern void RBX_MD5_Init(RBX_MD5_CTX *ctx);
extern void RBX_MD5_Update(RBX_MD5_CTX *ctx, const void *data, size_t size);
extern void RBX_MD5_Final(unsigned char *result, RBX_MD5_CTX *ctx);

#ifdef __cplusplus
}
#endif

#endif
