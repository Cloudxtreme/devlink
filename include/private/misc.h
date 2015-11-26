/*
 *   misc.c - Miscellaneous helpers
 *   Taken from libteam project
 *   Copyright (C) 2011-2013 Jiri Pirko <jiri@resnulli.us>
 *   Copyright (C) 2016 Jiri Pirko <jiri@mellanox.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _T_MISC_H_
#define _T_MISC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline void *myzalloc(size_t size)
{
	return calloc(1, size);
}

static inline size_t mystrlcpy(char *dst, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;

		memcpy(dst, src, len);
		dst[len] = '\0';
	}
	return ret;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif /* _T_MISC_H_ */
