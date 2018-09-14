/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include <io.h>
#include <sys/timeb.h>

extern "C" {
	// hack to avoid error "unresolved external symbol" when linking
	void __mingw_raise_matherr(int typ, const char *name, double a1, double a2, double rslt) {}
#if defined(_MSC_VER) & (_MSC_VER >= 1900) // 2015
	unsigned int __cdecl _get_output_format(void) { return 1; }
#ifdef _WIN64
	FILE __iob[_IOB_ENTRIES] = {
		*stdin, *stdout, *stderr
	};
	FILE* __cdecl __iob_func(void) { return __iob; }
#endif
#endif

#ifdef REGISTER_FILTER
	void *__imp_time64           = _time64;
	void *__imp__aligned_malloc  = _aligned_malloc;
	void *__imp__aligned_realloc = _aligned_realloc;
	void *__imp__aligned_free    = _aligned_free;

	void *__imp__sopen           = _sopen;
	void *__imp__wsopen          = _wsopen;

	void *__imp___ftime64        = _ftime64;
#endif
}
