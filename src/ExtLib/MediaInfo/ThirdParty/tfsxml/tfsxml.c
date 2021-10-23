#include "tfsxml.h"
#ifndef NULL
    #ifdef __cplusplus
        #define NULL 0
    #else
        #define NULL ((void *)0)
    #endif
#endif

static inline void next_char(tfsxml_string* priv)
{
    priv->buf++;
    priv->len--;
}

int tfsxml_cmp_charp(tfsxml_string a, const char* b)
{
    // Compare char per char and return the difference if chars are no equal
    for (; a.len && *b; a.buf++, a.len--, b++)
    {
        char c = *a.buf - *b;
        if (c)
            return c;
    }

    if (!a.len && !*b)
        return 0; // All equal
    else if (*b)
        return -*b; // b is longer than a
    else
        return *a.buf; // a is longer than b
}

tfsxml_string tfsxml_str_charp(tfsxml_string a, const char* b)
{
    // iterate string to be scanned
    for (; a.len; a.buf++, a.len--)
    {
        const char* buf = a.buf;
        int len = a.len;
        const char* bb = b;
        // Compare char per char
        for (; len && *bb; buf++, len--, bb++)
        {
            char c = *buf - *bb;
            if (c)
                break;
        }
        if (!len)
        {
            return a;
        }
    }
    a.buf = NULL;
    a.len = 0;
    return a;
}

int tfsxml_init(tfsxml_string* priv, void *buf, int len)
{
    if (len < 0) {
        return -1;
    }
    priv->buf = buf;
    priv->len = len;
    priv->flags = 0;
    return 0;
}


int tfsxml_next(tfsxml_string* priv, tfsxml_string* b)
{
    int level = priv->flags ? 1 : 0;
    while (priv->len)
    {
        switch (*priv->buf)
        {
        case '<':
            next_char(priv);
            if (priv->len && *priv->buf == '?')
            {
                b->buf = priv->buf;
                b->len = priv->len;
                return 0;
            }
            if (priv->len && *priv->buf == '!')
            {
                b->buf = priv->buf;
                while (priv->len && *priv->buf != '>')
                {
                    next_char(priv);
                }
                next_char(priv);
                b->len = priv->buf - b->buf;
                priv->flags = 0;
                return 0;
            }
            if (level && priv->len && *priv->buf == '/')
            {
                if (!level)
                {
                    b->buf = priv->buf;
                    b->len = priv->len;
                    return 0;
                }
                level--;
                break;
            }
            if (!level)
            {
                b->buf = priv->buf;
                for (;;)
                {
                    if (!priv->len)
                        return -1;

                    switch (*priv->buf)
                    {
                    case '>':
                    case ' ':
                        b->len = priv->buf - b->buf;
                        priv->flags = 1;
                        return 0;
                    case '/':
                        while (priv->len && *priv->buf != '>')
                        {
                            next_char(priv);
                        }
                        next_char(priv);
                        b->buf = NULL;
                        b->len = 0;
                        priv->flags = 0;
                        return -1;
                    default:;
                    }
                    next_char(priv);
                }


                priv->flags = 1;
                b->len = priv->len;
                return 0;
            }
            level++;
            break;
        default:;
        }
        next_char(priv);
    }
    b->buf = NULL;
    b->len = 0;
    priv->flags = 0;
    return -1;
}

int tfsxml_attr(tfsxml_string* priv, tfsxml_string* a, tfsxml_string* v)
{
    if (!priv->flags)
        return -1;
    while (priv->len)
    {
        switch (*priv->buf)
        {
        case ' ':
        case '/':
            break;
        case '>':
            next_char(priv);
            a->buf = NULL;
            a->len = 0;
            v->buf = NULL;
            v->len = 0;
            priv->flags = 0;
            return -1;
        default:
            // Attribute
            a->buf = priv->buf;
            while (priv->len && *priv->buf != '=')
            {
                next_char(priv);
            }
            a->len = priv->buf - a->buf;
            next_char(priv);
            
            // Value
            next_char(priv);
            v->buf = priv->buf;
            while (priv->len && *priv->buf != '"')
            {
                next_char(priv);
            }
            v->len = priv->buf - v->buf;
            next_char(priv);
            return 0;
        }
        next_char(priv);
    }
    a->buf = NULL;
    a->len = 0;
    v->buf = NULL;
    v->len = 0;
    priv->flags = 0;
    return -1;
}


int tfsxml_value(tfsxml_string* priv, tfsxml_string* v)
{
    tfsxml_enter(priv, v);
    v->buf = priv->buf;
    while (priv->len && *priv->buf != '<')
    {
        next_char(priv);
    }
    v->len = priv->buf - v->buf;
    while (priv->len && *priv->buf != '>')
    {
        next_char(priv);
    }
    next_char(priv);

    return 0;
}

int tfsxml_enter(tfsxml_string* priv, tfsxml_string* v)
{
    if (priv->flags)
    {
        while (priv->len && *priv->buf != '>')
        {
            next_char(priv);
        }
        next_char(priv);
        priv->flags = 0;
    }
    
    v->buf = NULL;
    v->len = 0;

    while (priv->len)
    {
        switch (*priv->buf)
        {
        case '\n':
        case '\t':
        case '\r':
        case ' ':
            next_char(priv);
            break;
        default:
            return 0;
        }
    }

    return 0;
}

int tfsxml_leave(tfsxml_string* priv, tfsxml_string* v)
{
    int level = 0;
    int is_parsing_attr = 0;

    tfsxml_enter(priv, v);

    v->buf = NULL;
    v->len = 0;

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
                    {
                        next_char(priv);
                    }
                    next_char(priv);
                    return 0;
                }
                level--;
                break;
            }
            if (priv->len && *priv->buf == '?')
            {
                break;
            }
            if (priv->len && *priv->buf == '!')
            {
                while (priv->len && *priv->buf != '>')
                {
                    next_char(priv);
                }
                next_char(priv);
                break;
            }
            level++;
            break;
        case '>' :
            break;
        case '/' :
            level--;
            break;
        default:;
        }
        next_char(priv);
    }

    return 0;
}
