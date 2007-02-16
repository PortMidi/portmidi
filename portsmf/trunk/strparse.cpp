#include "string.h"
#include "strparse.h"
#include "ctype.h"

void String_parse::skip_space()
{
    while (string[pos] && isspace(string[pos])) {
        pos = pos + 1;
    }
}


char String_parse::peek()
{
    return string[pos];
}


void String_parse::get_nonspace_quoted(char *field)
{
    skip_space();
    bool quoted = false;
    if (string[pos] == '"') {
        quoted = true;
        *field++ = '"';
        pos = pos + 1;
    }
    while (string[pos] && (quoted || !isspace(string[pos]))) {
        if (string[pos] == '"') {
            if (quoted) {
                *field++ = '"';
                pos = pos + 1;
            }
            *field = 0;
            return;
        }
        if (string[pos] == '\\') {
            pos = pos + 1;
        }
        if (string[pos]) {
            *field++ = string[pos];
            pos = pos + 1;    
        }
    }
    *field = 0;
}


char *escape_chars[] = {"\\n", "\\t", "\\\\", "\\r", "\\\""};

void string_escape(char *result, char *str, char *quote)
{
    int length = (int) strlen(str);
    if (quote[0]) {
        *result++ = quote[0];
    }
    for (int i = 0; i < length; i++) {
        if (!isalnum(str[i])) {
            char *chars = "\n\t\\\r\"";
            char *special = strchr(chars, str[i]);
            if (special) {
                *result++ = escape_chars[special - chars][0];
                *result++ = escape_chars[special - chars][1];
            } else {
                *result++ = str[i];
            }
        } else {
            *result++ = str[i] ;
        }
    }
    *result++ = quote[0];
    *result = 0;
}

