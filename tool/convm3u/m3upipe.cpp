//
// Created by paul on 12/27/23.
//

#include <string>
#include "m3upipe.h"

char * removeQuotes( char * string )
{
    char * p = string;
    if ( *p == '"' ) {
        while ( *p != '\0' ) {
            p[0] = p[1];
            p++;
        }
        p -= 2;
        if ( *p == '"' ) {
            *p = '\0';
        }
    }
    return string;
}

char * wrapInQuotes( char * string )
{
    char * p = string;
    while ( *p != '\0' ) p++;
    p += 2;
    *p-- = '\0';
    *p-- = '"';
    *p-- = '!';
    while ( p >= string ) {
        p[1] = p[0];
        --p;
    }
    *string = '"';

    return string;
}

std::string processM3U(FILE * input)
{
    std::string output;
    char lineBuffer[65535];
    char eol[3];
    char * p;
    char * m;

    /* process a line at a time */
    while ( fgets( lineBuffer, sizeof(lineBuffer), input ) != NULL ) {

        /* trim trailing line endings */
        p = strpbrk( lineBuffer, "\n\r" );
        if ( p != NULL ) {
            strncpy( eol, p, sizeof(eol));
            *p = '\0';
        }

        struct {
            char * key;
            char * value;
        } kvtable[64];
        unsigned int i = 0;

        enum { no, forward, reverse} mangle = no;

        if (memcmp( lineBuffer, "#EXTINF:", 8 ) == 0 ) {
            mangle = forward;
            output += "#STREAM:";
            p = &lineBuffer[8];
            m = strpbrk( p, " ,");
            if (m != NULL) {
                kvtable[i].key = (char*)"m3u-duration";
                *m++ = '\0';
                kvtable[i].value = p;
                i++;
                p = m;
            }
        } else if (memcmp( lineBuffer, "#STREAM: ", 9 ) == 0 ) {
            mangle = reverse;
            output += "#EXTINF:";
            p = &lineBuffer[9];
        }

        if ( mangle == no ) {
            output += lineBuffer;
            output += eol;
        } else {
            /* parse the key-value pairs into kvtable[] */
            char * lastSpace = NULL;
            while ( *p != '\0' && i < 64 ) {
                /* be selective about what is acceptable as a key, otherwise
                 * things like parameters in URLs can be mistaken for keys */
                kvtable[i].key = p;
                int keyLen = 0;
                while ( isalnum( *p ) || *p == '-' ) {
                    p++;
                    keyLen++;
                }
                /* it's only a key if it's terminated by an equals sign.
                 * Note: multiple 'false starts' are expected if the value contains spaces */
                if ( *p == '=' && keyLen > 3 ) {
                    if ( lastSpace != NULL ) {
                        *lastSpace = '\0';      /* terminate the previous value */
                    }
                    *p++ = '\0';            /* terminate this key */
                    kvtable[i].value = p;   /* value starts after the equals sign */
                    i++;
                }
                /* Scan forwards to find the next space. Which may
                 * or may not be the one preceding the next key */
                while ( *p != ' ' && *p != '\0' ) p++;
                if ( *p == ' ' ) {
                    lastSpace = p++;
                }
            }

            /* reached the end of the line */
            if ( mangle == forward ) {
                /* the last key/value pair includes the ',<title>' suffix, so break that out */
                if ( i > 1 && i < 64 ) {
                    m = strstr( kvtable[ i - 1 ].value,"\"," );
                    if ( m != NULL ) {
                        ++m;
                        *m++ = '\0';
                        kvtable[i].key = (char*)"m3u-title";
                        /* this will extend the string in lineBuffer - but that's OK, as we're at the end */
                        wrapInQuotes(m);
                        kvtable[i].value = m;
                        i++;
                    }
                }

                /* consume the next line as a 'url' parameter, with the same
                 * formatting style as the parameters on the #EXTINF line. */
                while ( *p != '\0' ) p++;
                p++;
                fgets( p, sizeof(lineBuffer) - (p - lineBuffer) - 2, input );
                /* trim off the line endings */
                m = strpbrk( p, "\n\r" );
                if ( m != NULL) {
                    strncpy( eol, m, sizeof(eol));
                    *m = '\0';
                }

                if ( i < 64 ) {
                    kvtable[i].key   = (char*)"m3u-url";
                    wrapInQuotes(p);
                    kvtable[i].value = p;
                    i++;
                }
            }

            char * m3uTitle;
            char * m3uURL;
            if (mangle == reverse) {
                for ( unsigned int j = 0; j < i; j++ ) {
                    if ( strcmp(kvtable[j].key, "m3u-duration") == 0 ) {
                        output += kvtable[j].value;
                        kvtable[j].key = NULL;
                    } else if ( strcmp(kvtable[j].key, "m3u-title") == 0 ) {
                        m3uTitle = removeQuotes( kvtable[j].value );
                        kvtable[j].key = NULL;
                    } else if ( strcmp(kvtable[j].key, "m3u-url") == 0 ) {
                        m3uURL = removeQuotes( kvtable[j].value );
                        kvtable[j].key = NULL;
                    }
                }
            }

            for ( unsigned int j = 0; j < i; j++ ) {
                if ( kvtable[j].key != NULL ) {
                    output += " " + std::string(kvtable[j].key) + "=" + kvtable[j].value;
                }
            }
            if ( mangle == reverse ) {
                output += "," + std::string(m3uTitle) + eol + m3uURL;
            }
            output += eol;
        }
    }

    return output;
}
