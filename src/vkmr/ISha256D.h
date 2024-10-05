// ISha256D.h: declares the interface common to implementations of double SHA-256
//

#if !defined (__ISHA_256D_H__)
#define __ISHA_256D_H__

// Includes
//

// C++ Standard Library Headers
#include <string>

namespace vkmr {

// Interfaces
//

class ISha256D {
public:
    typedef ::std::string arg_type;
    typedef ::std::string out_type;
    typedef ::std::string name_type;

    ISha256D(const name_type& name): m_name( name ) { }
    virtual ~ISha256D() = default;

    const name_type& Name(void) const { return m_name; }

    virtual out_type Root(void) = 0;

    virtual bool Add(const arg_type& arg) = 0;

    virtual bool Reset(void) = 0;

protected:
    name_type m_name;
};

} // namespace vkmr

#endif // __ISHA_256D_H__
