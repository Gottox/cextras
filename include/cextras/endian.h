/******************************************************************************
 *                                                                            *
 * Copyright (c) 2023, Enno Boland <g@s01.de>                                 *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions are     *
 * met:                                                                       *
 *                                                                            *
 * * Redistributions of source code must retain the above copyright notice,   *
 *   this list of conditions and the following disclaimer.                    *
 * * Redistributions in binary form must reproduce the above copyright        *
 *   notice, this list of conditions and the following disclaimer in the      *
 *   documentation and/or other materials provided with the distribution.     *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS    *
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  *
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR     *
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,      *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,        *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING       *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                            *
 ******************************************************************************/

/**
 * @author       Enno Boland (mail@eboland.de)
 * @file         collection.h
 */

#ifndef CEXTRA_ENDIAN_H
#define CEXTRA_ENDIAN_H

#define _DEFAULT_SOURCE

#include <stdint.h>
#if defined(__APPLE__)
#	include <libkern/OSByteOrder.h>

#	define CX_CPU_2_LE16(x) OSSwapHostToLittleInt16(x)
#	define CX_CPU_2_LE32(x) OSSwapHostToLittleInt32(x)
#	define CX_CPU_2_LE64(x) OSSwapHostToLittleInt64(x)
#	define CX_LE_2_CPU16(x) OSSwapLittleToHostInt16(x)
#	define CX_LE_2_CPU32(x) OSSwapLittleToHostInt32(x)
#	define CX_LE_2_CPU64(x) OSSwapLittleToHostInt64(x)
#	define CX_CPU_2_BE16(x) OSSwapHostToBigInt16(x)
#	define CX_CPU_2_BE32(x) OSSwapHostToBigInt32(x)
#	define CX_CPU_2_BE64(x) OSSwapHostToBigInt64(x)
#	define CX_BE_2_CPU16(x) OSSwapBigToHostInt16(x)
#	define CX_BE_2_CPU32(x) OSSwapBigToHostInt32(x)
#	define CX_BE_2_CPU64(x) OSSwapBigToHostInt64(x)

#else
#	if defined(__FreeBSD__)
#		include <sys/endian.h>
#	else
#		include <endian.h>
#	endif

#	define CX_CPU_2_LE16(x) htole16(x)
#	define CX_CPU_2_LE32(x) htole32(x)
#	define CX_CPU_2_LE64(x) htole64(x)
#	define CX_LE_2_CPU16(x) le16toh(x)
#	define CX_LE_2_CPU32(x) le32toh(x)
#	define CX_LE_2_CPU64(x) le64toh(x)
#	define CX_CPU_2_BE16(x) htobe16(x)
#	define CX_CPU_2_BE32(x) htobe32(x)
#	define CX_CPU_2_BE64(x) htobe64(x)
#	define CX_BE_2_CPU16(x) be16toh(x)
#	define CX_BE_2_CPU32(x) be32toh(x)
#	define CX_BE_2_CPU64(x) be64toh(x)

#endif

#endif /* CEXTRA_ENDIAN_H */
