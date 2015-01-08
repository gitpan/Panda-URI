package Panda::URI;
use parent 'Panda::Export';
use 5.012;
use Panda::Lib;

our $VERSION = '0.1.0';

=head1 NAME

Panda::URI - fast URI.pm-like framework written in C, with Perl and C interface

=cut

use Panda::Export {
    ALLOW_LEADING_AUTHORITY => 1,
    PARAM_DELIM_SEMICOLON   => 2,
};

require Panda::XSLoader;
Panda::XSLoader::bootstrap();

require overload;
overload->import(
    '""'     => \&to_string,
    'eq'     => sub {
        return ($_[0]->to_string eq $_[1]) unless ref $_[1];
        return $_[0]->equals($_[1]);
    },
    fallback => 1,
);

sub connect_port {
    my $self = shift;
    my $msg = '$uri->connect_port is deprecated, use $uri->port instead.';
    warn "\e[95m$msg\e[0m";
    return $self->port;
}

sub connect_location {
    my $self = shift;
    my $msg = '$uri->connect_location is deprecated, use $uri->location instead.';
    warn "\e[95m$msg\e[0m";
    return $self->location;
}

=head1 DESCRIPTION

Panda::URI has similar functionality as URI.pm, but is much faster (sometimes 100x).
It is used as a base URI unit in all Panda::* modules.

=head1 SYNOPSIS

    use Panda::URI qw/uri :const/;
    
    my $u = Panda::URI->new("http://mysite.com:8080/my/path?a=b&c=d#myhash");
    say $u->scheme;
    say $u->host;
    say $u->port;
    say $u->path;
    say $u->query_string;
    Dumper($u->query);
    say $u->fragment;
    
    $u = Panda::URI->new("about:blank");
    say $u->scheme;
    say $u->path;

    $u->clone;
    
=head1 FUNCTIONS

=head4 uri($url, [$flags])

Creates URI object from string $url. Created object will be of special subclass (Panda::URI::http, Panda::URI::ftp, ...) if
scheme is supported. Otherwise it will be of class Panda::URI.

Created object is in "strict" mode, i.e. it has additional methods according to the scheme, however you cannot change it's
scheme. You can still set a new url to this objects, but it must have the same scheme or error will be raisen.

Also "strict"(customized) classes has its own constructors with possibly additional arguments like this:

    my $url = Panda::URI::http->new("http://google.com?b=20", q => 'something', a => 10);
    say $url->query_string; # q=something&a=10&b=20
    $url->scheme('ftp'); # CROAKS, changing scheme is disallowed.
    
See custom classes' docs for details.

$flags is a bitmask of one or more of these:

=over

=item ALLOW_LEADING_AUTHORITY

By default, RFC doesn't allow urls to begin with authority (i.e. host,port). For example

    www.google.com/hello
    
is not interpreted as you might think. In this case, url is treated as relative and "www.google.com/hello" is a path

Enabling this flag makes Panda::URI detect such urls:

    www.google.com/hello      no scheme, host is www.google.com, path is /hello
    hello/world               no scheme, host is hello, path is world
    /hello/world              no scheme, no host, path is /hello/world
    
However, Panda::URI never produces RFC-uncompliant urls on output, so

    say uri("www.google.com/hello", ALLOW_LEADING_AUTHORITY);
    
prints "//www.google.com/hello" (scheme-relative format), making it valid
    
=item PARAM_DELIM_SEMICOLON

If true, Panda::URI will use ';' as delimiter between query string params instead of a default '&'. Both for input and output.

=back

=head4 register_scheme($scheme, $perl_class)

Registers a new scheme and a perl class for that scheme (it must inherit from Panda::URI). This only applies when creating
"strict"(customized) urls via uri() function (or via custom class' constructor).

As Panda::URI is a C++ framework in its base, you want also register a C++ class for that scheme in XS to be able to do something
when such uris are constructed (even from XS/C code).

See C<REGISTERING SCHEMAS> for how to.

=head4 encode_uri_component($bytes, [$use_plus]), encodeURIComponent($bytes, [$use_plus])

Does what JavaScript's encodeURIComponent does.

    $uri = encode_uri_component("http://www.example.com/");
    # http%3A%2F%2Fwww.example.com%2F

If $use_plus is true, then produces '+' for spaces instead of '%20'.

=head4 decode_uri_component($bytes), decodeURIComponent($bytes)

Does what JavaScript's decodeURIComponent does.

    $str = decode_uri_component("http%3A%2F%2Fwww.example.com%2F");
    # http://www.example.com/

=head1 CLASS METHODS

=head4 new($url, [$flags])

Creates URI object from string $url. Created object will be "non-strict", i.e. it will be of class "Panda::URI" and won't have any
scheme-specific methods, however you can change its scheme and set new urls with defferent scheme into the object.

register_scheme() makes no effect for this method.

$flags are the same as for uri() function.

=head1 OBJECT METHODS

=head4 url([$newurl], [$flags])

Returns url as string. If $newurl is present, sets this url in object (respecting $flags). May croak if object is in "strict" mode
and $newurl's scheme differs from current. If object is "strict" and $newurl has no scheme, it's assumed to  be current
(instead of leaving it empty if object is non-strict). Examples:

    my $u = Panda::URI->new("http://facebook.com"); # non-strict mode
    $u->query({a => 1, b => 2});
    say $u->url; # http://facebook.com?a=1&b=2
    $u->url("//twitter.com"); # scheme-relative url
    say $u; # //twitter.com
    
    $u = uri("http://facebook.com"); # strict mode
    $u->url("//twitter.com");
    say $u; # http://twitter.com, force object's scheme as it cannot change
    $u->url("svn://svn.com"); # croaks, scheme cannot change
    
    $u = Panda::URI::ftp->new("//my.com"); # strict mdoe
    say $u; # ftp://my.com
    $u->url("http://ya.ru"); # croaks
    
=head4 scheme([$new_scheme]), proto([$new_scheme]), protocol([$new_scheme])

Sets/returns uri's scheme. May croak if object is strict and new scheme differs from current.

=head4 user_info([$new_uinfo])

Sets/returns user_info part of uri (ftp://<user_info>@host/...)

=head4 host([$new_host])

Sets/returns host part of uri

=head4 port([$new_port])

Sets/returns port. If no port is explicitly present in uri, returns default port for uri's scheme. If no scheme in uri, returns 0.

=head4 explicit_port()

Returns port if it was explicitly set via port() or was present in uri. Otherwise returns 0.

=head4 default_port()

Returns default port for the uri's scheme. Returns 0 if scheme is not specified/not supported.

=head4 path([$new_path])

Sets/returns path part of uri as string.

=head4 query_string([$new_query_string])

Sets/returns query string part of uri as string. String is expected/returned in decoded, but plain format,
i.e. after uri encode of all params, but before encode_uri_component of the whole result string.

=head4 raw_query([$new_query_string])

Sets/returns query string part of uri as string. String is expected/returned in RAW (encoded) format, i.e.
after uri encode of all params and after encode_uri_component of the whole result string.

=head4 query([\%new_query | %new_query | $new_query_string])

If no params specified, returns query part of uri as hashref. Keys/values are returned unencoded.
If uri has no query params, empty hash is returned.

If you change returned hash, no changes will occur in uri object.
To commit these changes, set this hash again via query($hash) or use param() method.

If params are specified, sets new query from hash or hashref or string.
Keys/values are accepted unencoded for hash/hashref.

If you pass query as string, the effect will be the same as calling query_string($new_query).

If you want to make query strings like 'a=1&a=2&a=3', set "a"'s value to an arrayref of values, like:

    $u = Panda::URI->new("http://ya.ru");
    $u->query(b => 10, a => [1,2,3]);
    say $u; # http://ya.ru?b=10&a=1&a=2&a=3
    
Note hovewer, that multiparams are NOT returned in hashref:

    say Dumper($u->query); # {b => 10, a => 1/2/3 }
    
A's value may be any of 1/2/3 depending on hash order. This is done because most of the time you don't want multiparams and don't
wanna be suprised by an arrayref in query if someone passes you second value for some key.

If you want to get all values of multiparam, use multiparam().

=head4 add_query(\%query | %query | $query_string)

Like query() but instead of replacing, adds passed query to existing query. If some key already exists in uri's query, it doesn't
get replaced, instead it becomes a multiparam.

=head4 param($name, [$value | \@values])

Without second arg, returns the value of query param '$name'. If no such param exists, return undef. If param $name is a multiparam,
returns one of its values.

With $value supplied, replaces current value(values) of $name with $value.

With \@values supplied, replaces current value(values) of $name with \@values ($name becomes multiparam).

=head4 multiparam($name, [$value | \@values])

Does the same as param() does. The only difference is when called without second arg, returns a list of param's values if param
is a multiparam. Also returns empty list instead of undef if there is no such param in query.

=head4 nparam()

Returns the number of query parameters in query (even for multiparams). For example:

    "http://google.com"; # nparam() == 0
    "http://google.com?a=1&b=2"; # nparam() == 2
    "http://google.com?a=1&b=2&b=3&b=4"; # nparam() == 4

=head4 remove_param($name)

Removes param $name from query. If param is a multiparam, removes all its values.

=head4 fragment([$new_fragment]), hash([$new_fragment])

Sets/returns fragment (hash) part of uri.

=head4 location([$new_location])

Sets/returns location part of uri. Location is a "host:port" together. If no port was explicitly set, returned
location will contain details port for the scheme. If no scheme defined, or scheme is unknown, returned location will contain
port 0 - "host:0". Examples:

    say Panda::URI->new("http://ya.ru:8080")->location; # ya.ru:8080
    say Panda::URI->new("http://ya.ru")->location; # ya.ru:80
    say Panda::URI->new("//ya.ru")->location; # ya.ru:0
    
    say Panda::URI->new("http://ya.ru")->explicit_location; # ya.ru
    say Panda::URI->new("http://ya.ru:8080")->explicit_location; # ya.ru:8080
    
=head4 explicit_location()

Returns location with explicit port set if any, otherwise returns location without port (i.e. just host).

Effect is the same as

    $u->explicit_port ? $u->host.':'.$u->port : $u->host
    
=head4 relative(), rel()

Returns uri, relative to current scheme and location, for example:

    say uri("http://ya.ru/mypath")->relative; # /mypath
    
=head4 to_string(), as_string(), '""'

Returns the whole uri as string.

=head4 secure()

Returns true if uri's scheme is secure (for example, https).

=head4 set($other_uri)

Sets uri from another uri object making them equal. May croak if current object is strict and other object has different scheme.

=head4 assign($url, [$flags])

Same as url($url, [$flags])

=head4 equals($other_uri), 'eq'

Returns true if $other_uri contains the same url (including all parts - query, fragment, etc).

=head4 clone()

Clones current uri. If current uri is in strict mode, then cloned uri will be in strict mode too.

=head4 path_segments([@new_segments])

Sets/returns path segments as list.

    $u = uri("http://ya.ru/abc/def/jopa");
    say join(", ", $u->path_segments); # abc, def, jopa
    $u->path_segments('my', 'folder');
    say $u; # http://ya.ru/my/folder
    
=head1 STRICT CLASSES

=head2 Panda::URI::http

=head4 new($url, [\%query | %query | $query_string])

If provided, adds query params to $url after creating object.

=head2 Panda::URI::https

=head4 new($url, [\%query | %query | $query_string])

If provided, adds query params to $url after creating object.

=head2 Panda::URI::ftp

=head4 user([$new_user])

Sets/returns user part of user_info in uri.

=head4 password([$new_pass])

Sets/returns password part of user_info in uri.

=head1 C++ INTERFACE

Here and below only short details are explained. For full docs see perl interface docs above.
All functions and classes are in panda::uri:: namespace.

C<string> is not an std::string, it's a panda::string, which has the same API, but is more effective and supports Copy-On-Write
regardless of your compiler version. See L<Panda::Lib> for details.

=head2 panda::uri::URI

=head4 static URI* create (const string& source, int flags = 0)

Creates uri object in strict mode. Returns object of customized class (panda::uri::URI::http, ...). If no scheme specified or
scheme is not supported, returns object of a default class panda::uri::URI.

=head4 static URI* create (const URI& source)

Creates strict uri object from another uri object.

=head4 URI ()

Creates empty non-strict uri object.

=head4 URI (const string& source, int flags = 0)

Creates non-strict uri object from string.

=head4 URI (const URI& source)

Creates non-strict uri object from another object (cloning).

=head4 URI& operator= (const URI& source)

=head4 URI& operator= (const string& source)

Sets data from another uri object or url string.

=head4 const string& scheme () const

=head4 const string& user_info () const

=head4 const string& host () const

=head4 const string& path () const

=head4 const string& fragment () const

=head4 uint16_t explicit_port () const

=head4 uint16_t default_port () const

=head4 uint16_t port () const

=head4 bool secure () const

Returns properties of uri.

=head4 virtual void assign (const URI& source)

Assign data from another uri. Same as C<URI& operator= (const URI& source)>.

=head4 void assign (const string& uristr, int flags = 0)

Assign data from url string.

=head4 const string& query_string () const

Returns unencoded query string

=head4 const string raw_query () const

Returns encoded query string

=head4 Query& query ()

Returns query params as object of class Query (std::multimap<string,string>). Unlike for perl's method, you can change this
multimap object and changes will take effect for uri object.

=head4 const Query& query () const

Same as previous method but only for reading.

=head4 virtual void scheme (const string& scheme)

Changes object's scheme. May throw an exception of class WrongScheme if object in strict mode and schemes differ.

=head4 void user_info (const string& user_info)

=head4 void host (const string& host)

=head4 void fragment (const string& fragment)

=head4 void port (uint16_t port)

=head4 void path (const string& path)

Changes properties of uri.

=head4 void query_string (const string& qstr)

Sets unencoded query string

=head4 void raw_query (const string& rq)

Sets encoded query string

=head4 void query (const string& qstr)

Same as query_string(qstr).

=head4 void query (const Query& query)

Replaces current query with new one supplied as multimap.

=head4 void add_query (const string& addstr)

Adds query params from addstr to current query.

=head4 void add_query (const Query& addquery)

Adds query params from addquery to current query.

=head4 const string& param (const string& key) const

Returns value for param with key 'key'. If it's a multiparam, returns first of its values.

=head4 void param (const string& key, const string& val)

Sets param value replacing existing one. If it's a multiparam, replaces just first of its values.

=head4 string explicit_location () const

=head4 string location () const

=head4 void location (const string& newloc)

=head4 const std::vector<string> path_segments () const

=head4 void path_segments (const std::vector<string>& list)

=head4 string to_string (bool relative = false) const

=head4 string relative () const

=head4 bool equals (const URI& uri) const

See perl interface docs for methods above.

=head4 void swap (URI& uri)

Swaps content of two uri objects.

=head4 typedef URI* (*uricreator) (const URI& uri)

Creator function type for custom scheme objects.

=head4 static void register_scheme (const string& scheme, const std::type_info* ti, uricreator, uint16_t default_port, bool secure = false)

Registers new scheme. "ti" is a typeinfo for your scheme's class. It's required for URI framework to automatically convert
scheme names to classes and vice-versa.

See C<REGISTERING SCHEMAS> for how to.

=head2 panda::uri::URI::http

=head4 http (const string& source, const Query& query, int flags = 0)

=head2 panda::uri::URI::https

=head4 https (const string& source, const Query& query, int flags = 0)

=head2 panda::uri::URI::ftp

=head4 const string user () const

=head4 void user (const string& user)

=head4 const string password () const

=head4 void password (const string& password)

=head2 panda::uri functions

=head4 char* encode_uri_component (const char* src, size_t srclen, char* dest, size_t* destlen, const char* unsafe = unsafe_query_component)

Does what JavaScript's encodeURIComponent does.

'dest' must have enough space to hold the result (in worst case = srclen*3 + 1)).
'destlen' is set to actual resulting string length.

'unsafe' is an array char[256] where index is char code to be replaced and value is either 0 or the same char code.
If value is 0 then this char should be replaced with %XX. If value isn't 0, then it is replaced with value code.
By default the alphabet for query param names and values is used. You can use one of these predefined arrays (in panda::uri::):
unsafe_scheme, unsafe_uinfo, unsafe_host, unsafe_path, unsafe_path_segment, unsafe_query, unsafe_query_component, unsafe_fragment.

=head4 void encode_uri_component (const char* src, size_t srclen, string& dest, const char* unsafe = unsafe_query_component)

=head4 void encode_uri_component (const string& src, char* dest, size_t* destlen, const char* unsafe = unsafe_query_component)

=head4 void encode_uri_component (const string& src, string& dest, const char* unsafe = unsafe_query_component)

String versions.

=head4 char* decode_uri_component (const char* src, size_t srclen, char* dest, size_t* destlen)

Does what JavaScript's decodeURIComponent does.

'dest' must have enough space to hold the result (in worst case = srclen)).
'destlen' is set to actual resulting string length.

=head4 void decode_uri_component (const char* src, size_t srclen, string& dest)

=head4 void decode_uri_component (const string& src, char* dest, size_t* destlen)

=head4 void decode_uri_component (const string& src, string& dest)

String versions.

=head1 REGISTERING SCHEMAS

Let's create our custom scheme "myproto" which like FTP uses some info from "user_info". Our protocol won't be secure and default
port is for example 12345.

Firstly we need to create our own C++ class. It must inherit from panda::uri::URI::Strict

    #include <panda/uri.h>
    using panda::uri::URI;

    class URI::myproto : public Strict {
    public:
        myproto () : Strict() {}
        myproto (const string& source, int flags = 0) : Strict(source, flags) { strict_scheme(); }
        myproto (const URI& source)                   : Strict(source)        { strict_scheme(); }

        using Strict::operator=;

        const string some_data_from_user_info () const {
            // parse user_info
            // return result
        }
        
        void some_data_from_user_info (const string& new_data) {
            // change user_info
        }
        
Notice the strict_scheme() call in constructor. It is required because it will throw an exception if scheme is wrong. In all other
methods it is done automatically, but unfortunately while in panda::uri::URI::Strict class constructor,
object is not yet ready for type_info manipulations.

Secondly, create a function that creates URI::myproto object from default URI object.

    static URI* new_myproto (const URI& source) {
        return new URI::myproto(source);
    }

Now, register your new scheme somewhere in program's initialization:

    void init () {
        ...
        URI::register_scheme("myproto", &typeid(URI::myproto), new_myproto, 12345, false);
    }

That's it. Now use your custom scheme:

    URI* uri = URI::create("myproto://myinfo@google.com");
    URI::myproto myuri = dynamic_cast<URI::myproto*>(uri); // will return not-null
    cout << myuri->some_data_from_user_info();
};

Finally, create an XS and register a perl class

    # TYPEMAP
    
    URI::myproto* XT_PANDA_URI_STRICT
    
    # XS

    MODULE = MyURI                PACKAGE = MyURI::myproto
    PROTOTYPES: DISABLE
    
    string URI::myproto::some_data_from_user_info (SV* newval = NULL) {
        if (newval) {
            THIS->some_data_from_user_info(sv2string(newval));
            XSRETURN_UNDEF;
        }
        RETVAL = THIS->some_data_from_user_info();
    }
    
    ...
    
    # Somewhere in perl
    
    Panda::URI::register_scheme("myproto", "MyURI::myproto");
    
Usage from perl:

    my $u = uri("myproto://info@google.com");
    say ref $u; # MyURI::myproto
    say $u->some_data_from_user_info;

=head1 EXPORTED TYPEMAPS

=head2 TYPEMAPS

=head4 URI*

Typemap for input/output any URI objects.

=head4 URI::http*, URI::https*, URI::ftp*

Typemaps for input/output strict uris.

=head4 URIx*

Output-only typemap for autodetecting strict uri type and setting right perl class to bless to. You must not define a 'CLASS' variable.

    # XS
    URI* my_cool_uri_create1 (string url) {
        const char* CLASS = "Panda::URI";
        RETVAL = URI::create(url);
    }
    
    URIx* my_cool_uri_create2 (string url) {
        RETVAL = URI::create(url);
    }
    
    # Perl
    
    say ref my_cool_uri_create1("http://ya.ru"); # Panda::URI
    say ref my_cool_uri_create1("ftp://ya.ru"); # Panda::URI
    say ref my_cool_uri_create2("http://ya.ru"); # Panda::URI::http
    say ref my_cool_uri_create2("ftp://ya.ru"); # Panda::URI::ftp

=head2 TYPEMAP CLASSES

=head4 XT_PANDA_URI

Typemap to inherit from for your custom typemap classes for non-strict uris.

=head4 XT_PANDA_URI_STRICT

Typemap to inherit from for your custom typemap classes for strict uris. The difference is that this typemap class will automatically
set CLASS variable to the right perl class to bless to.

=head1 AUTHOR

Pronin Oleg <syber@crazypanda.ru>, Crazy Panda, CP Decision LTD

=head1 LICENSE

You may distribute this code under the same terms as Perl itself.

=cut

package # hide from PAUSE
    Panda::URI::_userpass;
our @ISA = 'Panda::URI';

package Panda::URI::http;
our @ISA = 'Panda::URI';

package Panda::URI::https;
our @ISA = 'Panda::URI::http';

package Panda::URI::ftp;
our @ISA = 'Panda::URI::_userpass';

1;