// SHA-256.h: declares the class(es), type(s) and function(s) for computed SHA-256 hashes
//

#if !defined (__SHA_256_H__)
#define __SHA_256_H__

// Includes
//

// C++ Standard Library Headers
#include <string>
#include <vector>

// Functions
//

// Calculates and return the SHA-256 of the given input
std::string cpu_sha256(const std::string&);

// Calculates and return the SHA-256^2 of the given input
std::string cpu_sha256d(const std::string&);

#endif // __SHA_256_H__
