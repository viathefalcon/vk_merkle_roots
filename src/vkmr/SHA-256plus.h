// SHA-256plus.h: declares the class(es), type(s) and function(s) for computed SHA-256 hashes
//

#if !defined (__SHA_256plus_H__)
#define __SHA_256plus_H__

// Includes
//

// Local Project Headers
#include "ISha256D.h"

// C++ Standard Library Headers
#include <vector>

namespace vkmr {

// Functions
//

// Calculates and return the SHA-256 of the given input
::std::string cpu_sha256(const ::std::string&);

// Calculates and return the SHA-256^2 of the given input
::std::string cpu_sha256d(const ::std::string&);

// Classes
//

class CpuSha256D : public ISha256D {
public:
    CpuSha256D(): ISha256D("Single-threaded CPU implementation") { }

    out_type Root(void);

    bool Add(const ISha256D::arg_type& arg) {
        m_args.push_back( arg );
        return true;
    }

    bool Reset(void) {
        m_args.clear( );
        return true;
    }

private:
    ::std::vector<arg_type> m_args;
};

} // namespace vkmr

#endif // __SHA_256plus_H__
