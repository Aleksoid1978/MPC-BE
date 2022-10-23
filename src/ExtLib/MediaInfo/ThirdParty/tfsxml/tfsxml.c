/* Copyright (c) MediaArea.net SARL. All Rights Reserved.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

(zlib license)

*/

#include "tfsxml.h"
#ifndef NULL
    #ifdef __cplusplus
        #define NULL 0
    #else
        #define NULL ((void *)0)
    #endif
#endif
#if !defined(inline)
    #define inline
#endif

/*
 * priv flags :
 * 0: is inside an element header
 * 1: previous element is closed
 */

/*
 * attribute / value flags :
 * 0: must be decoded
 */

static inline void set_flag(tfsxml_string* priv, int offset)
{
    priv->flags |= (1 << offset);
}

static inline int get_flag(tfsxml_string* priv, int offset)
{
    return priv->flags & (1 << offset);
}

static inline void unset_flag(tfsxml_string* priv, int offset)
{
    priv->flags &= ~(1 << offset);
}

static inline void next_char(tfsxml_string* priv)
{
    priv->buf++;
    priv->len--;
}

static inline int tfsxml_leave_element_header(tfsxml_string* priv)
{
    /* Skip attributes */
    tfsxml_string n, v;
    while (!tfsxml_attr(priv, &n, &v));

    return 0;
}


static inline int tfsxml_has_value(tfsxml_string* v)
{
    int is_end = 0;
    const char* buf = v->buf;
    int len = v->len;
    while (len && !is_end)
    {
        switch (*buf)
        {
        case '\n':
        case '\t':
        case '\r':
        case ' ':
            buf++;
            len--;
            break;
        default:
            is_end = 1;
        }
    }
    if (is_end)
        return 0;
    return -1;
}

int tfsxml_strcmp_charp(tfsxml_string a, const char* b)
{
    /* Compare char per charand return the difference if chars are no equal */
    for (; a.len && *b; a.buf++, a.len--, b++)
    {
        char c = *a.buf - *b;
        if (c)
            return c;
    }

    if (!a.len && !*b)
        return 0; /* All equal */
    else if (*b)
        return -*b; /* b is longer than a */
    else
        return *a.buf; /* a is longer than b */
}

tfsxml_string tfsxml_strstr_charp(tfsxml_string a, const char* b)
{
    /* Iterate string to be scanned */
    for (; a.len; a.buf++, a.len--)
    {
        const char* buf = a.buf;
        int len = a.len;
        const char* bb = b;
        /* Compare char per char */
        for (; len && *bb; buf++, len--, bb++)
        {
            char c = *buf - *bb;
            if (c)
                break;
        }
        if (!len || *bb)
        {
            return a;
        }
    }
    a.buf = NULL;
    a.len = 0;
    return a;
}

int tfsxml_init(tfsxml_string* priv, const void* buf, int len)
{
    const char* buf_8 = (const char*)buf;

    /* BOM detection */
    if (len > 3
        && (unsigned char)buf_8[0] == 0xEF
        && (unsigned char)buf_8[1] == 0xBB
        && (unsigned char)buf_8[2] == 0xBF)
    {
        buf_8 += 3;
        len -= 3;
    }

    /* Start detection */
    if (len < 1
        || buf_8[0] != '<') {
        return -1;
    }

    /* Init */
    priv->buf = (const char*)buf;
    priv->len = len;
    priv->flags = 0;
    set_flag(priv, 1);

    return 0;
}

int tfsxml_next(tfsxml_string* priv, tfsxml_string* n)
{
    int level;

    /* Exiting previous element header analysis if needed */
    if (get_flag(priv, 0) && tfsxml_leave_element_header(priv))
        return -1;

    /* Leaving previous element content if needed */
    if (!get_flag(priv, 1) && tfsxml_leave(priv))
        return -1;

    level = 0;
    while (priv->len)
    {
        switch (*priv->buf)
        {
        case '<':
            next_char(priv);
            if (priv->len && *priv->buf == '?')
            {
                n->buf = priv->buf;
                while (priv->len && *priv->buf != '>')
                {
                    next_char(priv);
                }
                n->len = priv->buf - n->buf;
                if (priv->len)
                    next_char(priv);
                set_flag(priv, 1);
                return 0;
            }
            if (priv->len && *priv->buf == '!')
            {
                unsigned long long probe = 0;
                int i;
                for (i = 1; i < 8; i++)
                {
                    probe <<= 8;
                    probe |= priv->buf[i];
                }
                if (probe == 0x5B43444154415BULL) /* "[CDATA[" */
                {
                    probe = 0;
                    priv->buf += 9;
                    priv->len -= 9;
                    while (priv->len)
                    {
                        probe &= 0xFFFF;
                        probe <<= 8;
                        probe |= *priv->buf;
                        if (probe == 0x5D5D3EULL) /* "]]>" */
                            break;
                        priv->buf++;
                        priv->len--;
                    }
                    break;
                }
                n->buf = priv->buf;
                if (priv->len >= 3 && priv->buf[1] == '-' && priv->buf[2] == '-')
                {
                    int len_sav = priv->len;
                    const char* buf = priv->buf + 3;
                    int len = priv->len - 3;
                    probe = 0;
                    while (len)
                    {
                        probe &= 0xFFFF;
                        probe <<= 8;
                        probe |= *buf;
                        if (probe == 0x2D2D3EULL) /* "-->" */
                            break;
                        buf++;
                        len--;
                    }
                    n->buf = priv->buf;
                    n->len = len_sav - len;
                    priv->buf += n->len;
                    priv->len -= n->len;
                    if (priv->len)
                        next_char(priv);
                    return 0;
                }
                while (priv->len && *priv->buf != '>')
                    next_char(priv);
                n->len = priv->buf - n->buf;
                if (priv->len)
                    next_char(priv);
                set_flag(priv, 1);
                return 0;
            }
            if (*priv->buf == '/')
            {
                while (priv->len && *priv->buf != '>')
                    next_char(priv);
                next_char(priv);
                if (!level)
                {
                    n->buf = NULL;
                    n->len = 0;
                    set_flag(priv, 1);
                    return -1;
                }
                level--;
                break;
            }
            if (!level)
            {
                n->buf = priv->buf;
                for (;;)
                {
                    if (!priv->len)
                        return -1;

                    switch (*priv->buf)
                    {
                    case '\n':
                    case '\t':
                    case '\r':
                    case ' ':
                    case '/':
                    case '>':
                        n->len = priv->buf - n->buf;
                        set_flag(priv, 0);
                        unset_flag(priv, 1);
                        return 0;
                    default:;
                    }
                    next_char(priv);
                }
            }
            level++;
            break;
        default:;
        }
        next_char(priv);
    }
    n->buf = NULL;
    n->len = 0;
    unset_flag(priv, 0);
    unset_flag(priv, 1);
    return -1;
}

int tfsxml_attr(tfsxml_string* priv, tfsxml_string* n, tfsxml_string* v)
{
    if (!get_flag(priv, 0))
        return -1;

    v->flags = 0;
    while (priv->len)
    {
        switch (*priv->buf)
        {
        case '\n':
        case '\t':
        case '\r':
        case ' ':
            break;
        case '/':
            set_flag(priv, 1);
            break;
        case '>':
            next_char(priv);
            n->buf = NULL;
            n->len = 0;
            v->buf = NULL;
            v->len = 0;
            v->flags = 0;
            unset_flag(priv, 0);
            return -1;
        default:
        {
            /* Attribute */
            n->buf = priv->buf;
            while (priv->len && *priv->buf != '=')
            {
                next_char(priv);
            }
            n->len = priv->buf - n->buf;
            if (!priv->len)
                return -1;
            next_char(priv);
        }
        {
            /* Value */
            const char quote = *priv->buf;
            if (!priv->len)
                return -1;
            next_char(priv);
            v->buf = priv->buf;
            while (priv->len && *priv->buf != quote)
            {
                if (*priv->buf == '&')
                    set_flag(v, 0);
                next_char(priv);
            }
            v->len = priv->buf - v->buf;
            if (!priv->len)
                return -1;
            next_char(priv);
            return 0;
        }
        }
        next_char(priv);
    }
    n->buf = NULL;
    n->len = 0;
    n->flags = 0;
    v->buf = NULL;
    v->len = 0;
    v->flags = 0;
    unset_flag(priv, 0);
    return -1;
}


int tfsxml_value(tfsxml_string* priv, tfsxml_string* v)
{
    tfsxml_string priv_bak = *priv;
    int len_sav;

    /* Exiting previous element header analysis if needed */
    int is_first = 0;
    if (get_flag(priv, 0))
    {
        if (tfsxml_leave_element_header(priv))
            return -1;
        is_first = 1;

        /* Previous element must not be finished */
        if (get_flag(priv, 1))
            return -1;
    }

    len_sav = priv->len;
    v->flags = 0;
    while (priv->len)
    {
        switch (*priv->buf)
        {
        case '&':
            set_flag(v, 0);
            break;
        case '<':
            if (priv->len == len_sav && priv->len > 8)
            {
                unsigned long long probe = 0;
                int i;
                for (i = 1; i <= 8; i++)
                {
                    probe <<= 8;
                    probe |= priv->buf[i];
                }
                if (probe == 0x215B43444154415BULL) /* "![CDATA[" */
                {
                    const char* buf = priv->buf + 9;
                    int len = priv->len - 9;
                    probe = 0;
                    while (len)
                    {
                        probe &= 0xFFFF;
                        probe <<= 8;
                        probe |= *buf;
                        if (probe == 0x5D5D3EULL) /* "]]>" */
                            break;
                        buf++;
                        len--;
                    }
                    v->buf = priv->buf;
                    v->len = len_sav - len;
                    if (v->len < priv->len)
                        v->len++;
                    priv->buf += v->len;
                    priv->len -= v->len;
                    v->buf += 9;
                    v->len -= 12;
                    return 0;
                }
            }
            v->len = len_sav - priv->len;
            v->buf = priv->buf - v->len;
            if (tfsxml_has_value(v))
            {
                *priv = priv_bak;
                return -1;
            }
            if (is_first)
            {
                unset_flag(priv, 1);
                tfsxml_leave(priv);
            }
            return 0;
        default:;
        }
        next_char(priv);
    }

    v->len = len_sav;
    v->buf = priv->buf - v->len;
    v->flags = 0;
    if (tfsxml_has_value(v))
    {
        *priv = priv_bak;
        return -1;
    }
    return 0;
}

int tfsxml_enter(tfsxml_string* priv)
{
    /* Exiting previous element header analysis if needed */
    if (get_flag(priv, 0) && tfsxml_leave_element_header(priv))
        return -1;

    /* Previous element must not be finished */
    if (get_flag(priv, 1))
        return -1;

    set_flag(priv, 1);
    return 0;
}

int tfsxml_leave(tfsxml_string* priv)
{
    int level;

    /* Exiting previous element header analysis if needed */
    if (get_flag(priv, 0) && tfsxml_leave_element_header(priv))
        return -1;

    level = get_flag(priv, 1) ? 1 : 0;
    while (priv->len)
    {
        switch (*priv->buf)
        {
        case '<':
            next_char(priv);
            if (priv->len && *priv->buf == '/')
            {
                if (!level)
                {
                    while (priv->len && *priv->buf != '>')
                        next_char(priv);
                    if (priv->len)
                        next_char(priv);
                    set_flag(priv, 1);
                    return 0;
                }
                level--;
                if (priv->len)
                    next_char(priv);
                break;
            }
            if (priv->len && *priv->buf == '?')
            {
                while (priv->len && *priv->buf != '>')
                {
                    next_char(priv);
                }
                if (priv->len)
                    next_char(priv);
                set_flag(priv, 1);
                break;
            }
            if (priv->len && *priv->buf == '!')
            {
                unsigned long long probe = 0;
                int i;
                for (i = 1; i < 8; i++)
                {
                    probe <<= 8;
                    probe |= priv->buf[i];
                }
                if (probe == 0x5B43444154415BULL) /* "[CDATA[" */
                {
                    probe = 0;
                    priv->buf += 9;
                    priv->len -= 9;
                    while (priv->len)
                    {
                        probe &= 0xFFFF;
                        probe <<= 8;
                        probe |= *priv->buf;
                        if (probe == 0x5D5D3EULL) /* "]]>" */
                            break;
                        priv->buf++;
                        priv->len--;
                    }
                    break;
                }
                if (priv->len >= 3 && priv->buf[1] == '-' && priv->buf[2] == '-')
                {
                    const char* buf = priv->buf + 3;
                    int len_sav = priv->len;
                    int len = priv->len - 3;
                    probe = 0;
                    while (len)
                    {
                        probe &= 0xFFFF;
                        probe <<= 8;
                        probe |= *buf;
                        if (probe == 0x2D2D3EULL) /* "-->" */
                            break;
                        buf++;
                        len--;
                    }
                    len = len_sav - len;
                    priv->buf += len;
                    priv->len -= len;
                    if (priv->len)
                        next_char(priv);
                    break;
                }
                while (priv->len && *priv->buf != '>')
                    next_char(priv);
                if (priv->len)
                    next_char(priv);
                break;
            }
            for (;;)
            {
                int split;

                if (!priv->len)
                    return -1;

                split = 0;
                switch (*priv->buf)
                {
                case '\n':
                case '\t':
                case '\r':
                case ' ':
                case '/':
                case '>':
                    set_flag(priv, 0);
                    unset_flag(priv, 1);
                    split = 1;
                    break;
                default:;
                }
                if (split)
                    break;
                next_char(priv);
            }
            if (tfsxml_leave_element_header(priv))
                return -1;
            if (!get_flag(priv, 1))
                level++;
            break;
        default:;
            next_char(priv);
        }
    }

    set_flag(priv, 1);
    set_flag(priv, 3);
    return 0;
}

static const char* const tfsxml_decode_markup[2] =
{
    "amp\0apos\0gt\0lt\0quot",
    "&'><\"",
};

void tfsxml_decode(void* s, const tfsxml_string* v, void (*func)(void*, const char*, int))
{
    const char* buf_begin;
    const char* buf = v->buf;
    int len = v->len;

    if (!(v->flags & 1))
    {
        func(s, buf, len);
        return;
    }

    buf_begin = buf;
    while (len)
    {
        if (*buf == '&')
        {
            const char* buf_end = buf;
            int len_end = len;
            while (len_end && *buf_end != ';')
            {
                buf_end++;
                len_end--;
            }
            if (len_end)
            {
                const char* buf_beg = buf + 1;
                int len_beg = buf_end - buf_beg;
                if (len_beg && *buf_beg == '#')
                {
                    unsigned long value = 0;
                    buf_beg++;
                    len_beg--;
                    if (*buf_beg == 'x' || *buf_beg == 'X')
                    {
                        buf_beg++;
                        len_beg--;
                        while (len_beg)
                        {
                            char c = *buf_beg++;
                            len_beg--;
                            value <<= 4;
                            if (value >= 0x110000)
                            {
                                value = -1;
                                break;
                            }
                            if (c >= '0' && c <= '9')
                                value |= c - '0';
                            else if (c >= 'A' && c <= 'F')
                                value |= c - ('A' - 10);
                            else if (c >= 'a' && c <= 'f')
                                value |= c - ('a' - 10);
                            else
                            {
                                value = -1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        while (len_beg)
                        {
                            char c = *buf_beg++;
                            len_beg--;
                            if (c < '0' || c > '9')
                            {
                                value = -1;
                                break;
                            }
                            value *= 10;
                            value += c - '0';
                            if (value >= 0x110000)
                            {
                                value = -1;
                                break;
                            }
                        }
                    }

                    if (value != (unsigned long)-1)
                    {
                        char utf8[4];
                        func(s, buf_begin, buf - buf_begin);
                        if (value < 0x0080)
                        {
                            utf8[0] = (char)value;
                            func(s, utf8, 1);
                        }
                        else if (value < 0x0800)
                        {
                            utf8[0] = 0xC0 | (char)(value >> 6);
                            utf8[1] = 0x80 | (char)(value & 0x3F);
                            func(s, utf8, 2);
                        }
                        else if (value < 0x10000)
                        {
                            utf8[0] = 0xE0 | (char)((value >> 12));
                            utf8[1] = 0x80 | (char)((value >> 6) & 0x3F);
                            utf8[2] = 0x80 | (char)((value & 0x3F));
                            func(s, utf8, 3);
                        }
                        else if (value < 0x110000)
                        {
                            utf8[0] = 0xF0 | (char)((value >> 18));
                            utf8[1] = 0x80 | (char)((value >> 12) & 0x3F);
                            utf8[2] = 0x80 | (char)((value >> 6) & 0x3F);
                            utf8[3] = 0x80 | (char)((value & 0x3F));
                            func(s, utf8, 4);
                        }

                        len -= buf_end - buf;
                        buf = buf_end;
                        buf_begin = buf_end + 1;
                    }
                }
                else
                {
                    const char* const buf_beg_sav = buf_beg;
                    const char* replaced = tfsxml_decode_markup[0];
                    const char* replace_bys = tfsxml_decode_markup[1];
                    for (;;)
                    {
                        char replace_by = *replace_bys++;
                        if (!replace_by)
                            break;

                        while (*replaced)
                        {
                            if (buf_beg == buf_end || !*replaced || *replaced != *buf_beg)
                                break;
                            replaced++;
                            buf_beg++;
                        }
                        if (buf_beg == buf_end && !*replaced)
                        {
                            func(s, buf_begin, buf - buf_begin);
                            func(s, &replace_by, 1);
                            len -= buf_end - buf;
                            buf = buf_end;
                            buf_begin = buf_end + 1;
                            break;
                        }
                        buf_beg = buf_beg_sav;
                        while (*replaced)
                            replaced++;
                        replaced++;
                    }
                }
            }
        }
        buf++;
        len--;
    }
    func(s, buf_begin, buf - buf_begin);
}
