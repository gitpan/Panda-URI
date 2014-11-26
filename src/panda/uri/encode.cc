#include <climits>
#include <panda/uri/encode.h>
#include <panda/lib.h>

namespace panda { namespace uri {

#define FROM_HEX(ch) (std::isdigit(ch) ? ch - '0' : std::tolower(ch) - 'a' + 10)
typedef unsigned char uchar;

char unsafe_scheme[256];
char unsafe_uinfo[256];
char unsafe_host[256];
char unsafe_path[256];
char unsafe_path_segment[256];
char unsafe_query[256];
char unsafe_query_component[256];
char unsafe_fragment[256];

static char _restore[256];
static char _forward[256][2];
static char _backward[256][2];

static int init () {
    unsafe_generate(unsafe_scheme, UNSAFE_ALPHA|UNSAFE_DIGIT, "+-.");
    unsafe_generate(unsafe_uinfo, UNSAFE_UNRESERVED | UNSAFE_SUBDELIMS, ":");
    unsafe_generate(unsafe_host, UNSAFE_UNRESERVED | UNSAFE_SUBDELIMS);
    unsafe_generate(unsafe_path, UNSAFE_PCHAR, "/");
    unsafe_generate(unsafe_path_segment, UNSAFE_PCHAR);
    unsafe_generate(unsafe_query, UNSAFE_PCHAR, "/?");
    unsafe_generate(unsafe_query_component, UNSAFE_UNRESERVED);
    unsafe_generate(unsafe_fragment, UNSAFE_PCHAR, "/?");

    static char hex[] = "0123456789ABCDEF";
    char c = CHAR_MIN;
    do {
        uchar uc = (uchar)c;
        _restore[uc] = c;
        _forward[uc][0] = hex[(c >> 4) & 15];
        _forward[uc][1] = hex[(c & 15) & 15];
        _backward[uc][0] = FROM_HEX(c) << 4;
        _backward[uc][1] = FROM_HEX(c);
    } while (c++ != CHAR_MAX);

    _restore[(uchar)'%'] = 0;
    _restore[(uchar)'+'] = ' ';

    return 0;
}
static int __init = init();

char* encode_uri_component (const char* src, size_t srclen, char* dest, size_t* destlen, const char* unsafe) {
    char* buf = dest;
    for (int i = 0; i < srclen; ++i) {
        uchar uc = src[i];
        if (likely(unsafe[uc] != 0)) *buf++ = unsafe[uc];
        else {
            *buf++ = '%';
            *buf++ = _forward[uc][0];
            *buf++ = _forward[uc][1];
        }
    }

    *buf = 0;
    *destlen = buf - dest;
    return dest;
}

char* decode_uri_component (const char* src, size_t srclen, char* dest, size_t* destlen) {
    char* buf = dest;
    for (int i = 0; i < srclen; ++i) {
        char res = _restore[(uchar)src[i]];
        if (likely(res != 0)) *buf++ = res;
        else if (i < srclen-2) {
            *buf++ = _backward[(uchar)src[i+1]][0] | _backward[(uchar)src[i+2]][1];
            i += 2;
        }
    }

    *buf = 0;
    *destlen = buf - dest;
    return dest;
}

}}
