/*-
 * Copyright (c) 2005 Boris Mikhaylov
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BS2BVERSION_H
#define BS2BVERSION_H

#define BS2B_VERSION_MAJOR    3
#define BS2B_VERSION_MINOR    1
#define BS2B_VERSION_RELEASE  0

#define BS2B_STRINGIFY_HELPER(X) #X
#define BS2B_STRINGIFY(X) BS2B_STRINGIFY_HELPER(X)

#define BS2B_VERSION_STR \
	BS2B_STRINGIFY(BS2B_VERSION_MAJOR) "." \
	BS2B_STRINGIFY(BS2B_VERSION_MINOR) "." \
	BS2B_STRINGIFY(BS2B_VERSION_RELEASE)

#define BS2B_VERSION_INT (\
	( BS2B_VERSION_MAJOR << 16 ) | \
	( BS2B_VERSION_MINOR << 8  ) | \
	( BS2B_VERSION_RELEASE     ))

#endif	/* BS2BVERSION_H */
