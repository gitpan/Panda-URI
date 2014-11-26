#pragma once
#include <map>
#include <panda/string.h>
#include <panda/lib.h>

namespace panda { namespace uri {

using panda::string;

class Query : public std::multimap<string, string> {
private:
    typedef std::multimap<key_type,mapped_type> base;
public:
    typedef std::pair<iterator,iterator>             pair;
    typedef std::pair<const_iterator,const_iterator> const_pair;
    uint32_t rev;

    explicit
    Query (const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type()) : base(comp, alloc), rev(1) {}

    template <class InputIterator>
    Query (InputIterator first, InputIterator last, const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
        : base(first, last, comp, alloc), rev(1) {}

    Query (const Query& x) : base(x), rev(1) {}

    Query& operator= (const Query& x) {
        rev++;
        base::operator=(x);
        return *this;
    }

    template <class InputIterator>
    void     insert (InputIterator first, InputIterator last)     { rev++; return base::insert(first, last); }
    iterator insert (const value_type& val)                       { rev++; return base::insert(val); }
    iterator insert (iterator position, const value_type& val)    { rev++; return base::insert(position, val); }
    iterator insert (const key_type& key, const mapped_type& val) { return insert(value_type(key, val)); }
    iterator insert (const char* key, const char* val)            { return insert(value_type(key, val)); }

    void      erase (iterator position)             { rev++; base::erase(position); }
    size_type erase (const key_type& k)             { rev++; return base::erase(k); }
    void      erase (iterator first, iterator last) { rev++; base::erase(first, last); }

    void swap (Query& x) {
        rev++;
        x.rev++;
        base::swap(x);
    }

    void clear () { rev++; base::clear(); }

    iterator         begin  () { rev++; return base::begin(); }
    reverse_iterator rbegin () { rev++; return base::rbegin(); }
    iterator         end    () { rev++; return base::end(); }
    reverse_iterator rend   () { rev++; return base::rend(); }

    const_iterator         cbegin  () const { return base::begin(); }
    const_iterator         cend    () const { return base::end(); }
    const_reverse_iterator crbegin () const { return base::rbegin(); }
    const_reverse_iterator crend   () const { return base::rend(); }

    iterator       find        (const key_type& k)       { rev++; return base::find(k); }
    iterator       lower_bound (const key_type& k)       { rev++; return base::lower_bound(k); }
    iterator       upper_bound (const key_type& k)       { rev++; return base::upper_bound(k); }
    const_iterator find        (const key_type& k) const { return base::find(k); }
    const_iterator lower_bound (const key_type& k) const { return base::lower_bound(k); }
    const_iterator upper_bound (const key_type& k) const { return base::upper_bound(k); }

    pair       equal_range (const key_type& k)       { rev++; return base::equal_range(k); }
    const_pair equal_range (const key_type& k) const { return base::equal_range(k); }
};

}}
