#pragma once
#include <map>
#include <xs/xs.h>
#include <panda/string.h>
#include <panda/uri/URI.h>

namespace xs { namespace uri {

using panda::string;
using panda::uri::URI;

typedef URI URIx;

class XSURI {
public:
    URI*             uri;
    mutable SV*      query_cache;
    mutable uint32_t query_cache_rev;

    XSURI (URI* uri) : uri(uri), query_cache(NULL), query_cache_rev(0) {
        uri->retain();
    }

    void sync_query_hv () const;

    SV* query_hv () const {
        if (!query_cache || query_cache_rev != uri->query().rev) sync_query_hv();
        return query_cache;
    }

    ~XSURI () {
        if (query_cache) SvREFCNT_dec(query_cache);
        uri->release();
    }

    static void register_perl_scheme (const char* scheme, const char* perl_class);
    static SV*  get_perl_class       (const URI* uri);

    static void add_query_args (URI* uri, SV** sp, I32 items, bool replace = false);
    static void add_query_hv   (URI* uri, HV*, bool replace = false);
    static void add_param      (URI* uri, string key, SV* val, bool replace = false);

private:
    typedef std::map<uint64_t, SV*> UriClassMap;
    static UriClassMap uri_class_map;

    XSURI (const XSURI& s) {}
    XSURI& operator= (const XSURI& s) { return *this; }
};

}}
