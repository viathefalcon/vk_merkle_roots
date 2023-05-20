// SHA-256.h: declares the type(s) and function(s) for computed SHA-256 hashes
//

#if !defined (__SHA_256_H__)
#define __SHA_256_H__

// Includes
//

// C Standard Library Headers
#include <stddef.h>
#include <stdint.h>

// Macros
//

// The number of 32-bit words in a SHA-256 hash
#define SHA256_WC 8

#ifdef __cplusplus
extern "C" {
#endif

// Types
//

typedef struct _sha256_result_t {
    uint32_t words[SHA256_WC];
} sha256_result_t;

// Functions
//

// Generates and returns the SHA-256 hash of the given input
sha256_result_t sha256(const uint8_t*, size_t);

// Generates and returns the SHA-256 hash of the given input
sha256_result_t sha256w(const uint32_t*, size_t);

#ifdef __cplusplus
}
#endif

#endif // __SHA_256_H__
