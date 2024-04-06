/* Copyright (c) MediaArea.net SARL

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

(MIT License)

*/

/*
 * Tiny Fast Streamable XML parser
 */

#ifndef TFSXML_H
#define TFSXML_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** -------------------------------------------------------------------------
        Splitting of the XML content in blocks
------------------------------------------------------------------------- **/

/** Base structure for the parser
 *
 * @param buf  pointer to the buffer which has the data
 * @param len  length of the buffer
 * @param flags  private data, can change between versions of the library
 *
 * @note after init, priv should not be directly used (data may be something else than a buf/len pair)
 */
typedef struct tfsxml_string {
    const char* buf;
    unsigned    len;
    unsigned    flags;
} tfsxml_string;

/** Initialize the parser
 *
 * @param priv      pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param buf       pointer to start of the buffer
 * @param len       length of the buffer
 * @param version   API version supported by the client
 *
 * @note after init, priv should not be directly used (data may be something else than a buf/len pair)
 */
int tfsxml_init(tfsxml_string* priv, const void* buf, unsigned len, unsigned version);

/** Get next element or other content except an element value
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param n  pointer to tfsxml_string instance receiving the element name or other content
 *
 * @return  0 if an element is available at the current level
 *          -1 if no element is available at the current level
 *          1 if need of more data
 */
int tfsxml_next(tfsxml_string* priv, tfsxml_string* n);

/** Get next attribute of an element
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param n  pointer to tfsxml_string instance receiving the attribute name
 * @param v  pointer to tfsxml_string instance receiving the attribute value
 *
 * @return  0 if an attribute is available for the current element
 *          -1 if no more attribute is available for the current element
 *          1 if need of more data
 */
int tfsxml_attr(tfsxml_string* priv, tfsxml_string* n, tfsxml_string* v);

/** Get next value of an element
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param v  pointer to tfsxml_string instance receiving the value
 *
 * @return  0 if a value is available for the current element
 *          -1 if no value is available for the current element
 *          1 if need of more data
 *
 * @note if this element has sub-elements, the whole content (all sub-elements) are provided
 */
int tfsxml_value(tfsxml_string* priv, tfsxml_string* v);

/** Enter in the element in order to get sub-elements
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 *
 * @return  0 if it is possible to enter in an element
 *          -1 if it is not possible to enter in an element
 *          1 if need of more data
 */
int tfsxml_enter(tfsxml_string* priv);

/** Leave the current parsed element (going to upper level)
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 *
 * @return  0 if it is possible to leave an element
 *          -1 if it is not possible to leave an element
 *          1 if need of more data
 */
int tfsxml_leave(tfsxml_string* priv);

/** -------------------------------------------------------------------------
        Handling the encoded XML block
------------------------------------------------------------------------- **/

/** Convert encoded XML block (attribute or value) to real content (encoded in UTF-8)
 *
 * @param s         opaque data transmitted to func
 * @param v         XML content to decode
 * @param func      append function that will receive blocks of decoded content
 *
 * @param func_s    string to append to
 * @param func_buf  decoded content
 * @param func_len  length of the decoded content
 *
 * @note see tfsxml_decode_string C++ function for an example of usage
 */
void tfsxml_decode(void* s, const tfsxml_string* v, void (*func)(void* func_s, const char* func_buf, unsigned func_len));

/** -------------------------------------------------------------------------
        Helper functions related to tfsxml_string
------------------------------------------------------------------------- **/

/** Compare two strings
 *
 * @param a  string to compare
 * @param b  string to compare
 *
 * @note similar to C strcmp function
 */
int tfsxml_strcmp_charp(tfsxml_string a, const char* b);

/** Compare two strings
 *
 * @param a  string to compare
 * @param b  string to compare
 * @param n  maximum number of characters to compare
 *
 * @note similar to C strncmp function
 */
int tfsxml_strncmp_charp(tfsxml_string a, const char* b, unsigned n);

/** Locate substring
 *
 * @param a  string to be scanned
 * @param b  substring to match
 *
 * @note similar to C strstr function
 */
tfsxml_string tfsxml_strstr_charp(tfsxml_string a, const char* b);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
#include <string>

static void tfsxml_decode_string(void* d, const char* buf, unsigned len) { ((std::string*)d)->append(buf, len); }

/** Convert encoded XML block (attribute or value) to real content (encoded in UTF-8)
 *
 * @param s  string which will be appended with the decoded content
 * @param b  XML content to decode
 */
static void tfsxml_decode(std::string& s, const tfsxml_string& b) { tfsxml_decode(&s, &b, tfsxml_decode_string); }

/** Convert encoded XML block (attribute or value) to real content (encoded in UTF-8)
 *
 * @param b  XML content to decode
 * @return  decoded content
 */
static std::string tfsxml_decode(const tfsxml_string& b) { std::string s; tfsxml_decode(&s, &b, tfsxml_decode_string); return s; }

#endif /* __cplusplus */

#endif

