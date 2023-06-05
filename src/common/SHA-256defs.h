// SHA-256defs.h: defines values and macros common to SHA-256 implementation(s)
//

// Macros
//

#define SWOP_ENDS_U32(x) \
	(((x) >> 24) & 0x000000FF) | \
	(((x) >> 8) & 0x0000FF00) | \
	(((x) << 8) & 0x00FF0000) | \
	(((x) << 24) & 0xFF000000)

#define SHR(n,x) (((x) & 0xFFFFFFFF) >> (n))

#define ROTR(n,x) (SHR( (n), (x) ) | ((x) << (32 - (n))))

#define Sigma0(x) (ROTR( 2, (x) ) ^ ROTR( 13, (x) ) ^ ROTR( 22, (x) ))
#define Sigma1(x) (ROTR( 6, (x) ) ^ ROTR( 11, (x) ) ^ ROTR( 25, (x) ))
#define sigma0(x) (ROTR( 7, (x) ) ^ ROTR( 18, (x) ) ^ SHR( 3, (x) ))
#define sigma1(x) (ROTR( 17, (x) ) ^ ROTR( 19, (x) ) ^ SHR( 10, (x) ))

#define Ch(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

// Implicitly, this means that this implementation can only work 
// on inputs which are 2^32 bytes long..
#define MB_SIZE_BE_TOP(bytes)	    	(((bytes) & 0xE0000000) >> 29)
#define MB_SIZE_BE_BOTTOM(bytes)	    SWOP_ENDS_U32( (bytes) << 3 )

#define SHA256_MESSAGE_BLOCK_BYTE_SIZE	(512 >> 3)
#define SHA256_MESSAGE_BLOCK_WC			(SHA256_MESSAGE_BLOCK_BYTE_SIZE >> 2)
#define SHA256_WC						8

// Expands to a round of SHA-256
// (assumes endianess is aligned)
#define sha256_be_round(M, w) \
    { \
        uint W[64] = { 0 }; \
        for (uint t = 0; t < 64; ++t){ \
            if (t < SHA256_MESSAGE_BLOCK_WC){ \
                W[t] = (M)[t]; \
            }else{ \
                W[t] = sigma1( W[t-2] ) + W[t-7] + sigma0( W[t-15] ) + W[t-16]; \
            } \
            uint T1 = (w).s7 + Sigma1( (w).s4 ) + Ch( (w).s4, (w).s5, (w).s6 ) + c_constants[t] + W[t]; \
            uint T2 = Sigma0( (w).s0 ) + Maj( (w).s0, (w).s1, (w).s2 ); \
            (w).s7 = (w).s6; \
            (w).s6 = (w).s5; \
            (w).s5 = (w).s4; \
            (w).s4 = (w).s3 + T1; \
            (w).s3 = (w).s2; \
            (w).s2 = (w).s1; \
            (w).s1 = (w).s0; \
            (w).s0 = T1 + T2; \
        } \
    }

// Types
//

// This is defined for `glslc` (https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Standard_macros)
// but not for other compilers
#ifndef GL_core_profile
typedef uint32_t uint;
#endif

struct VkSha256Result {
	uint data[SHA256_WC];
};

struct VkSha256Metadata {
    uint start;
    uint size;
};
