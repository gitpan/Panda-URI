#pragma once
#include <map>
#include <vector>
#include <cctype>
#include <typeinfo>
#include <stdexcept>
#include <panda/lib.h>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/uri/Query.h>
#include <panda/uri/encode.h>

namespace panda { namespace uri {

using panda::string;
using panda::lib::itoa;

extern char unsafe_query[256];

class URIError : public std::logic_error {
public:
  explicit URIError (const std::string& what_arg) : logic_error(what_arg) {}
};

class WrongScheme : public URIError {
public:
  explicit WrongScheme (const std::string& what_arg) : URIError(what_arg) {}
};

class URI : public virtual panda::RefCounted {

public:
    enum flags_t {
        ALLOW_LEADING_AUTHORITY = 1, // allow urls to begin with authority (i.e. 'google.com', 'login@mysite.com:8080/mypath', etc (but NOT with IPV6 [xx:xx:...])
        PARAM_DELIM_SEMICOLON   = 2, // allow query string param to be delimiter by ';' rather than '&'
    };

    class Strict;
    class httpX;
    class UserPass;

    class http;
    class https;
    class ftp;

    typedef URI* (*uricreator) (const URI& uri);

    static void register_scheme (const string& scheme, const std::type_info*, uricreator, uint16_t default_port, bool secure = false);

    static URI* create (const string& source, int flags = 0) {
        URI temp(source, flags);
        if (temp.scheme_info) return temp.scheme_info->creator(temp);
        else                  return new URI(temp);
    }

    static URI* create (const URI& source) {
        if (source.scheme_info) return source.scheme_info->creator(source);
        else                    return new URI(source);
    }

    URI ()                                    : scheme_info(NULL), _port(0), _qrev(1), _flags(0)     {}
    URI (const string& source, int flags = 0) : scheme_info(NULL), _port(0), _qrev(1), _flags(flags) { parse(source); }
    URI (const URI& source)                                                                          { assign(source); }

    URI& operator= (const URI& source)    { if (this != &source) assign(source); return *this; }
    URI& operator= (const string& source) { assign(source); return *this; }

    const string& scheme        () const { return _scheme; }
    const string& user_info     () const { return _user_info; }
    const string& host          () const { return _host; }
    const string& path          () const { return _path; }
    const string& fragment      () const { return _fragment; }
    uint16_t      explicit_port () const { return _port; }
    uint16_t      default_port  () const { return scheme_info ? scheme_info->default_port : 0; }
    uint16_t      port          () const { return _port ? _port : default_port(); }
    bool          secure        () const { return scheme_info ? scheme_info->secure : false; }

    virtual void assign (const URI& source) {
        _scheme     = source._scheme;
        scheme_info = source.scheme_info;
        _user_info  = source._user_info;
        _host       = source._host;
        _path       = source._path;
        _qstr       = source._qstr;
        _query      = source._query;
        _qrev       = source._qrev;
        _fragment   = source._fragment;
        _port       = source._port;
        _flags      = source._flags;
    }

    void assign (const string& uristr, int flags = 0) {
        clear();
        _flags = flags;
        parse(uristr);
    }

    const string& query_string () const {
        sync_query_string();
        return _qstr;
    }

    const string raw_query () const {
        sync_query_string();
        string decoded;
        decode_uri_component(_qstr, decoded);
        return decoded;
    }

    Query& query () {
        sync_query();
        return _query;
    }

    const Query& query () const {
        sync_query();
        return _query;
    }

    virtual void scheme (const string& scheme) {
        _scheme.assign(scheme);
        sync_scheme_info();
    }

    void user_info (const string& user_info) { _user_info.assign(user_info); }
    void host      (const string& host)      { _host.assign(host); }
    void fragment  (const string& fragment)  { _fragment.assign(fragment); }
    void port      (uint16_t port)           { _port = port; }

    void path (const string& path) {
        if (path.length() > 0 && path[0] != '/') {
            _path.assign(1, '/');
            _path.append(path);
        }
        else _path.assign(path);
    }

    void query_string (const string& qstr) {
        _qstr.assign(qstr);
        ok_qstr();
    }

    void raw_query (const string& rq) {
        _qstr.clear();
        encode_uri_component(rq, _qstr, unsafe_query);
    }

    void query (const string& qstr) { query_string(qstr); }
    void query (const Query& query) {
        _query = query;
        ok_query();
    }

    void add_query (const string& addstr) {
        if (!addstr.length()) return;
        sync_query_string();
        ok_qstr();
        if (_qstr.length() > 0) {
            _qstr.reserve(_qstr.length() + addstr.length() + 1);
            _qstr.append(1, '&');
            _qstr.append(addstr);
        } else {
            _qstr.assign(addstr);
        }
    }

    void add_query (const Query& addquery);

    const string& param (const string& key) const {
        Query::const_iterator row = _query.find(key);
        return row == _query.cend() ? _empty : row->second;
    }

    void param (const string& key, const string& val) {
        Query::iterator row = _query.find(key);
        if (row != _query.end()) row->second.assign(val);
        else _query.insert(key, val);
    }

    string explicit_location () const {
        string str(_host.length() + 6);
        if (_host.length()) str.append(_host);

        if (_port) {
            str.append(1, ':');
            str.append(itoa(_port));
        }

        return str;
    }

    string location () const {
        string str(_host.length() + 6);
        if (_host.length()) str.append(_host);

        int cport = port();
        str.append(1, ':');
        str.append(itoa(cport));

        return str;
    }

    void location (const string& newloc) {
        size_t len = newloc.length();
        if (!len) {
            _host.clear();
            _port = 0;
            return;
        }

        size_t delim = newloc.rfind(':');
        if (delim == string::npos) _host.assign(newloc);
        else {
            size_t ipv6end = newloc.rfind(']');
            if (ipv6end != string::npos && ipv6end > delim) _host.assign(newloc);
            else {
                _host.assign(newloc, 0, delim);
                _port = std::strtol(newloc.data() + delim + 1, NULL, 10);
            }
        }
    }

    const std::vector<string> path_segments () const;
    void path_segments (const std::vector<string>& list);

    string to_string (bool relative = false) const;
    string relative  () const { return to_string(true); }

    bool equals (const URI& uri) const {
        if (_path != uri._path || _host != uri._host || _user_info != uri._user_info || _fragment != uri._fragment || _scheme != uri._scheme) return false;
        if (_port != uri._port && port() != uri.port()) return false;
        sync_query_string();
        uri.sync_query_string();
        return _qstr == uri._qstr;
    }

    void swap (URI& uri) {
        std::swap(_scheme,     uri._scheme);
        std::swap(scheme_info, uri.scheme_info);
        std::swap(_user_info,  uri._user_info);
        std::swap(_host,       uri._host);
        std::swap(_port,       uri._port);
        std::swap(_path,       uri._path);
        std::swap(_qstr,       uri._qstr);
        std::swap(_query,      uri._query);
        std::swap(_qrev,       uri._qrev);
        std::swap(_fragment,   uri._fragment);
        std::swap(_flags,      uri._flags);
    }

    virtual ~URI () {}

protected:
    struct scheme_info_t {
        int        index;
        string     scheme;
        uricreator creator;
        uint16_t   default_port;
        bool       secure;
        const std::type_info* type_info;
    };
    typedef std::map<const string, scheme_info_t*> SchemeMap;
    typedef std::map<uint64_t, scheme_info_t*> SchemeTIMap;
    typedef std::vector<scheme_info_t*> SchemeVector;

    scheme_info_t* scheme_info;

    static SchemeMap    scheme_map;
    static SchemeTIMap  scheme_ti_map;
    static SchemeVector schemas;

    virtual void parse (const string& uristr);

private:
    string           _scheme;
    string           _user_info;
    string           _host;
    string           _path;
    string           _fragment;
    uint16_t         _port;
    mutable string   _qstr;
    mutable Query    _query;
    mutable uint32_t _qrev; // last query rev we've synced query string with (0 if query itself isn't synced with string)
    int              _flags;

    static const string _empty;

    void ok_qstr      () const { _qrev = 0; }
    void ok_query     () const { _qrev = _query.rev - 1; }
    void ok_qboth     () const { _qrev = _query.rev; }
    bool has_ok_qstr  () const { return !_qrev || _qrev == _query.rev; }
    bool has_ok_query () const { return _qrev != 0; }

    void clear () {
        _port = 0;
        _scheme.clear();
        scheme_info = NULL;
        _user_info.clear();
        _host.clear();
        _path.clear();
        _qstr.clear();
        _query.clear();
        _fragment.clear();
        ok_qboth();
        _flags = 0;
    }

    inline void guess_leading_authority ();

    void compile_query () const;
    void parse_query   () const;

    void sync_query_string () const { if (!has_ok_qstr()) compile_query(); }
    void sync_query        () const { if (!has_ok_query()) parse_query(); }

    void sync_scheme_info () {
        size_t len = _scheme.length();
        if (!len) {
            scheme_info = NULL;
            return;
        }

        // lowercase the scheme
        char* p = _scheme.buf();
        for (size_t i = 0; i < len; i++) p[i] = tolower(p[i]);

        SchemeMap::iterator it = scheme_map.find(_scheme);
        if (it == scheme_map.end()) scheme_info = NULL;
        else                        scheme_info = it->second;
    }
};

inline std::ostream& operator<< (std::ostream& os, const URI& uri) { return os << uri.to_string(); }
inline bool operator== (const URI& lhs, const URI& rhs) { return lhs.equals(rhs); }
inline bool operator!= (const URI& lhs, const URI& rhs) { return !lhs.equals(rhs); }
inline void swap (URI& l, URI& r) { l.swap(r); }

}}
