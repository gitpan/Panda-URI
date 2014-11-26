#include <xs/lib.h>
#include <panda/lib.h>
#include <xs/uri/XSURI.h>

namespace xs { namespace uri {

using xs::lib::sv2string;
using panda::uri::Query;
using panda::lib::string_hash;

XSURI::UriClassMap XSURI::uri_class_map;

void XSURI::register_perl_scheme (const char* scheme, const char* perl_class) {
    uri_class_map[string_hash(scheme)] = newSVpvn_share(perl_class, strlen(perl_class), 0);
}

SV* XSURI::get_perl_class (const URI* uri) {
    static SV* default_perl_class = newSVpvs_share("Panda::URI");
    UriClassMap::iterator it = uri_class_map.find(string_hash(uri->scheme().data(), uri->scheme().length()));
    if (it == uri_class_map.end()) return default_perl_class;
    else return it->second;
}

static void hv2query (HV* hvquery, Query* query) {
    I32 size = hv_iterinit(hvquery);
    char* keystr;
    I32 keylen;
    for (int i = 0; i < size; ++i) {
        SV* valueSV = hv_iternextsv(hvquery, &keystr, &keylen);
        string key(keystr, keylen, string::COPY);

        if (SvROK(valueSV) && SvTYPE(SvRV(valueSV)) == SVt_PVAV) {
            AV* values = (AV*) SvRV(valueSV);
            I32 subsize = av_len(values)+1;
            for (int j = 0; j < subsize; ++j) {
                string value;
                SV** valref = av_fetch(values, j, 0);
                if (valref && SvOK(*valref)) {
                    STRLEN vlen;
                    char* vstr = SvPV(*valref, vlen);
                    value.assign(vstr, vlen, string::COPY);
                }
                query->insert(key, value);
            }
        }
        else {
            string value;
            if (SvOK(valueSV)) {
                STRLEN vlen;
                char* vstr = SvPV(valueSV, vlen);
                value.assign(vstr, vlen, string::COPY);
            }
            query->insert(key, value);
        }
    }
}

void XSURI::add_query_args (URI* uri, SV** sp, I32 items, bool replace) {
    if (items == 1) {
        if (SvROK(*sp)) {
            SV* var = SvRV(*sp);
            if (SvTYPE(var) == SVt_PVHV) add_query_hv(uri, (HV*)var, replace);
        }
        else if (replace) uri->query(sv2string(*sp));
        else              uri->add_query(sv2string(*sp));
    }
    else {
        SV** spe = sp + items;
        for (; sp < spe; sp += 2) add_param(uri, sv2string(*sp), *(sp+1), replace);
    }
}

void XSURI::add_param (URI* uri, string key, SV* val, bool replace) {
    if (SvROK(val) && SvTYPE(SvRV(val)) == SVt_PVAV) {
        if (replace) uri->query().erase(key);
        AV* arr = (AV*) SvRV(val);
        I32 nvals = av_len(arr) + 1;
        for (I32 i = 0; i < nvals; ++i) {
            SV** elemref = av_fetch(arr, i, 0);
            if (elemref) uri->query().insert(key, sv2string(*elemref));
        }
    }
    else if (replace) uri->param(key, sv2string(val));
    else uri->query().insert(key, sv2string(val));
}

void XSURI::add_query_hv (URI* uri, HV* hash, bool replace) {
    if (replace) {
        Query query;
        hv2query((HV*)hash, &query);
        uri->query(query);
        return;
    }

    I32 size = hv_iterinit(hash);
    char* keystr;
    I32 keylen;
    for (I32 i = 0; i < size; ++i) {
        SV* value = hv_iternextsv(hash, &keystr, &keylen);
        add_param(uri, string(keystr, keylen, string::COPY), value);
    }
}

void XSURI::sync_query_hv () const {
    HV* hash;
    if (query_cache) {
        hash = (HV*) SvRV(query_cache);
        hv_clear(hash);
    }
    else {
        hash = newHV();
        query_cache = newRV_noinc((SV*)hash);
    }

    const URI* uri = this->uri;
    Query::const_iterator end = uri->query().cend();
    for (Query::const_iterator it = uri->query().cbegin(); it != end; ++it)
        hv_store(hash, it->first.data(), it->first.length(), newSVpvn(it->second.data(), it->second.length()), 0);

    query_cache_rev = uri->query().rev;
}

}}
