// Tiny Fast Streamable XML parser

#ifndef TFSXML_H
#define TFSXML_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct tfsxml_string
{
    char*   buf;
    int     len;
    int     flags;
} tfsxml_string;

int tfsxml_cmp_charp(tfsxml_string a, const char* b);
tfsxml_string tfsxml_str_charp(tfsxml_string a, const char* b);

/** Initialize the parser
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param buf  pointer to start of the buffer
 * @param len  length of the buffer
 *
 * @note after init, priv should not be directly used (data may be something else than a buf/len pair
 */
int tfsxml_init(tfsxml_string* priv, void* buf, int len);

/** Get next element
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param b  pointer to tfsxml_string instance receiving the content data
 *
 * @return  0 if an element is available at the current level
 */
int tfsxml_next(tfsxml_string* priv, tfsxml_string* b);

/** Get next attribute
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param b  pointer to tfsxml_string instance receiving the content data
 *
 * @return  0 if an attribute is available for the current element
 *          -1 if no more attribute is available
 */
int tfsxml_attr(tfsxml_string* priv, tfsxml_string* a, tfsxml_string* v);

/** Get the value of an element
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param b  pointer to tfsxml_string instance receiving the content data
 *
 * @return  0 if a value is available for the current element
 *          -1 if problem
 *
 * @note if this element has sub-elements, the whole content (all sub-elements) are provided
 */

int tfsxml_value(tfsxml_string* priv, tfsxml_string* b);

/** Enter in the element in order to get sub-elements
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param b  not used
 *
 * @return  0 if all is OK
 *          -1 if problem
 */
int tfsxml_enter(tfsxml_string* priv, tfsxml_string* b);

/** Leave the current parsed element (going to upper level)
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param b  not used
 *
 * @return  0 if all is OK
 *          -1 if problem
 */
int tfsxml_leave(tfsxml_string* priv, tfsxml_string* b);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

