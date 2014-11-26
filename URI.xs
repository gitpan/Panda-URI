#include <xs/xs.h>
#include <xs/lib.h>
#include <xs/uri.h>
#include <iostream>
#include <panda/uri/all.h>

using namespace panda::uri;
using namespace xs::uri;
using xs::lib::sv2string;
using std::cout;
using std::endl;

static char unsafe_query_component_plus[256];


MODULE = Panda::URI                PACKAGE = Panda::URI
PROTOTYPES: DISABLE

TYPEMAP: << END
XSURI* XT_PANDA_XSURI
END

BOOT {
    unsafe_generate(unsafe_query_component_plus, UNSAFE_UNRESERVED);
    unsafe_query_component_plus[(unsigned char)' '] = '+';
    XSURI::register_perl_scheme("http",  "Panda::URI::http");
    XSURI::register_perl_scheme("https", "Panda::URI::https");
    XSURI::register_perl_scheme("ftp",   "Panda::URI::ftp");
}

URIx* uri (string url = string(), int flags = 0) {
    RETVAL = URI::create(url, flags);
}

void register_scheme (string scheme, string perl_class) {
    XSURI::register_perl_scheme(scheme.data(), perl_class.data());
}
    
INCLUDE: encode.xsi
INCLUDE: URI.xsi
INCLUDE: schemas.xsi
INCLUDE: cloning.xsi
