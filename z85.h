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

#pragma once

#include <cstdint>

//  --------------------------------------------------------------------------
//  Encode a binary frame as a string; destination string MUST be at least
//  size * 5 / 4 bytes long plus 1 byte for the null terminator. Returns
//  dest. Size must be a multiple of 4.
//  Returns NULL and sets errno = EINVAL for invalid input.
char *zmq_z85_encode(char *dest_, const uint8_t *data_, size_t size_);

//  --------------------------------------------------------------------------
//  Decode an encoded string into a binary frame; dest must be at least
//  strlen (string) * 4 / 5 bytes long. Returns dest. strlen (string)
//  must be a multiple of 5.
//  Returns NULL and sets errno = EINVAL for invalid input.
uint8_t *zmq_z85_decode(uint8_t *dest_, const char *string_);