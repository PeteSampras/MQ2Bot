/*
	Copyright (c) 2007-2017 Contributors as noted in the AUTHORS file

	This file is part of libzmq, the ZeroMQ core engine in C++.

	libzmq is free software; you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License (LGPL) as published
	by the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	As a special exception, the Contributors give you permission to link
	this library with independent modules to produce an executable,
	regardless of the license terms of these independent modules, and to
	copy and distribute the resulting executable under terms of your choice,
	provided that you also meet, for each linked independent module, the
	terms and conditions of the license of that module. An independent
	module is a module which is not derived from or based on this library.
	If you modify this library, you must extend this exception to your
	version of the library.

	libzmq is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
	License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//  Z85 codec, taken from 0MQ RFC project, implements RFC32 Z85 encoding

#include "z85.h"

#include <cstdio>
#include <cstring>
#include <errno.h>
#include <assert.h>


//  Maps base 256 to base 85
static char encoder[85 + 1] = { "0123456789"
							   "abcdefghij"
							   "klmnopqrst"
							   "uvwxyzABCD"
							   "EFGHIJKLMN"
							   "OPQRSTUVWX"
							   "YZ.-:+=^!/"
							   "*?&<>()[]{"
							   "}@%$#" };

//  Maps base 85 to base 256
//  We chop off lower 32 and higher 128 ranges
//  0xFF denotes invalid characters within this range
static uint8_t decoder[96] = {
  0xFF, 0x44, 0xFF, 0x54, 0x53, 0x52, 0x48, 0xFF, 0x4B, 0x4C, 0x46, 0x41,
  0xFF, 0x3F, 0x3E, 0x45, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x40, 0xFF, 0x49, 0x42, 0x4A, 0x47, 0x51, 0x24, 0x25, 0x26,
  0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
  0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x4D,
  0xFF, 0x4E, 0x43, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C,
  0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x4F, 0xFF, 0x50, 0xFF, 0xFF };

//  --------------------------------------------------------------------------
//  Encode a binary frame as a string; destination string MUST be at least
//  size * 5 / 4 bytes long plus 1 byte for the null terminator. Returns
//  dest. Size must be a multiple of 4.
//  Returns NULL and sets errno = EINVAL for invalid input.

char *zmq_z85_encode(char *dest_, const uint8_t *data_, size_t size_)
{
	if (size_ % 4 != 0) {
		errno = EINVAL;
		return NULL;
	}
	unsigned int char_nbr = 0;
	unsigned int byte_nbr = 0;
	uint32_t value = 0;
	while (byte_nbr < size_) {
		//  Accumulate value in base 256 (binary)
		value = value * 256 + data_[byte_nbr++];
		if (byte_nbr % 4 == 0) {
			//  Output value in base 85
			unsigned int divisor = 85 * 85 * 85 * 85;
			while (divisor) {
				dest_[char_nbr++] = encoder[value / divisor % 85];
				divisor /= 85;
			}
			value = 0;
		}
	}
	assert(char_nbr == size_ * 5 / 4);
	dest_[char_nbr] = 0;
	return dest_;
}


//  --------------------------------------------------------------------------
//  Decode an encoded string into a binary frame; dest must be at least
//  strlen (string) * 4 / 5 bytes long. Returns dest. strlen (string)
//  must be a multiple of 5.
//  Returns NULL and sets errno = EINVAL for invalid input.

uint8_t *zmq_z85_decode(uint8_t *dest_, const char *string_)
{
	unsigned int byte_nbr = 0;
	unsigned int char_nbr = 0;
	uint32_t value = 0;
	while (string_[char_nbr]) {
		//  Accumulate value in base 85
		if (UINT32_MAX / 85 < value) {
			//  Invalid z85 encoding, represented value exceeds 0xffffffff
			goto error_inval;
		}
		value *= 85;
		uint8_t index = string_[char_nbr++] - 32;
		if (index >= sizeof(decoder)) {
			//  Invalid z85 encoding, character outside range
			goto error_inval;
		}
		uint32_t summand = decoder[index];
		if (summand == 0xFF || summand > (UINT32_MAX - value)) {
			//  Invalid z85 encoding, invalid character or represented value exceeds 0xffffffff
			goto error_inval;
		}
		value += summand;
		if (char_nbr % 5 == 0) {
			//  Output value in base 256
			unsigned int divisor = 256 * 256 * 256;
			while (divisor) {
				dest_[byte_nbr++] = value / divisor % 256;
				divisor /= 256;
			}
			value = 0;
		}
	}
	if (char_nbr % 5 != 0) {
		goto error_inval;
	}
	assert(byte_nbr == strlen(string_) * 4 / 5);
	return dest_;

error_inval:
	errno = EINVAL;
	return NULL;
}