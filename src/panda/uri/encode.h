#pragma once
#include <panda/string.h>
using panda::string;

namespace panda { namespace uri {

static const int UNSAFE_DIGIT      =  1;
static const int UNSAFE_ALPHA      =  2;
static const int UNSAFE_SUBDELIMS  =  4;
static const int UNSAFE_GENDELIMS  =  8;
static const int UNSAFE_RESERVED   = 16;
static const int UNSAFE_UNRESERVED = 32;
static const int UNSAFE_PCHAR      = 64;

extern char unsafe_scheme[256];
extern char unsafe_uinfo[256];
extern char unsafe_host[256];
extern char unsafe_path[256];
extern char unsafe_path_segment[256];
extern char unsafe_query[256];
extern char unsafe_query_component[256];
extern char unsafe_fragment[256];

char* encode_uri_component (const char* src, size_t srclen, char* dest, size_t* destlen, const char* unsafe = unsafe_query_component);
char* decode_uri_component (const char* src, size_t srclen, char* dest, size_t* destlen);

inline void encode_uri_component (const char* src, size_t srclen, string& dest, const char* unsafe = unsafe_query_component) {
    size_t final_size;
    encode_uri_component(src, srclen, dest.reserve(srclen*3), &final_size, unsafe);
    dest.resize(final_size);
}

inline void decode_uri_component (const char* src, size_t srclen, string& dest) {
    size_t final_size;
    decode_uri_component(src, srclen, dest.reserve(srclen), &final_size);
    dest.resize(final_size);
}

inline void encode_uri_component (const string& src, char* dest, size_t* destlen, const char* unsafe = unsafe_query_component) {
    encode_uri_component(src.data(), src.length(), dest, destlen, unsafe);
}

inline void decode_uri_component (const string& src, char* dest, size_t* destlen) {
    decode_uri_component(src.data(), src.length(), dest, destlen);
}

inline void encode_uri_component (const string& src, string& dest, const char* unsafe = unsafe_query_component) {
    encode_uri_component(src.data(), src.length(), dest, unsafe);
}

inline void decode_uri_component (const string& src, string& dest) {
    decode_uri_component(src.data(), src.length(), dest);
}

inline void unsafe_generate (char* unsafe, int flags, const char* chars = NULL) {
    if (flags & UNSAFE_DIGIT)      unsafe_generate(unsafe, 0, "0123456789");
    if (flags & UNSAFE_ALPHA)      unsafe_generate(unsafe, 0, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if (flags & UNSAFE_SUBDELIMS)  unsafe_generate(unsafe, 0, "!$&'()*+,;=");
    if (flags & UNSAFE_GENDELIMS)  unsafe_generate(unsafe, 0, ":/?#[]@");
    if (flags & UNSAFE_RESERVED)   unsafe_generate(unsafe, UNSAFE_SUBDELIMS | UNSAFE_GENDELIMS);
    if (flags & UNSAFE_UNRESERVED) unsafe_generate(unsafe, UNSAFE_ALPHA | UNSAFE_DIGIT, "-._~");
    if (flags & UNSAFE_PCHAR)      unsafe_generate(unsafe, UNSAFE_UNRESERVED | UNSAFE_SUBDELIMS, ":@");
    if (chars) while (char c = *chars++) unsafe[(unsigned char) c] = c;
}

}}
