// SHA-256.c: defines the type(s) and function(s) for generating SHA-256 hashes.

// Includes
//

// Windows Headers
#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif // defined (_WIN32)

// Declarations
#include "SHA-256.h"

// Macros
//

#ifndef min
#define min(x,y) (((x) <= (y)) ? (x) : (y))
#endif

// Default to little-endianness
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__ 1
#endif

#define SWOP_ENDS_U32(x) \
	(((x) >> 24) & 0x000000FF) | \
	(((x) >> 8) & 0x0000FF00) | \
	(((x) << 8) & 0x00FF0000) | \
	(((x) << 24) & 0xFF000000)

#ifdef __LITTLE_ENDIAN__ 
#define GET_U32_BE(x) SWOP_ENDS_U32(x)
#else
#define GET_U32_BE(x) (x)
#endif

#define SWOP_ENDS_U64(x) \
	(((x) >> 56) & 0x00000000000000FF)	| \
	(((x) >> 40) & 0x000000000000FF00)	| \
	(((x) >> 24) & 0x0000000000FF0000)	| \
	(((x) >> 8) & 0x00000000FF000000)	| \
	(((x) << 8) & 0x000000FF00000000)	| \
	(((x) << 24) & 0x0000FF0000000000)	| \
	(((x) << 40) & 0x00FF000000000000)	| \
	(((x) << 56) & 0xFF00000000000000)

#ifdef __LITTLE_ENDIAN__ 
#define GET_U64_BE(x) SWOP_ENDS_U64(x)
#else
#define GET_U64_BE(x) (x)
#endif

#define SHR(n,x) (((x) & 0xFFFFFFFF) >> (n))

#define ROTR(n,x) (SHR( (n), (x) ) | ((x) << (32 - (n))))

#define Sigma0(x) (ROTR( 2, (x) ) ^ ROTR( 13, (x) ) ^ ROTR( 22, (x) ))
#define Sigma1(x) (ROTR( 6, (x) ) ^ ROTR( 11, (x) ) ^ ROTR( 25, (x) ))
#define sigma0(x) (ROTR( 7, (x) ) ^ ROTR( 18, (x) ) ^ SHR( 3, (x) ))
#define sigma1(x) (ROTR( 17, (x) ) ^ ROTR( 19, (x) ) ^ SHR( 10, (x) ))

#define Ch(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define clear_message_block(M, wc) \
	do { \
		volatile uint32_t* volatile m = M; \
		for (size_t w = 0; w < wc; ++w){ \
			(*m++) = 0U; \
		} \
	} while (0)

#define SHA256_MESSAGE_BLOCK_BYTE_SIZE	(512 / 8)
#define SHA256_MESSAGE_BLOCK_WC			(SHA256_MESSAGE_BLOCK_BYTE_SIZE / sizeof( uint32_t ))

// Constants
//

static const uint32_t c_constants[] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf,
										0xe9b5dba5, 0x3956c25b, 0x59f111f1,
										0x923f82a4, 0xab1c5ed5, 0xd807aa98,
										0x12835b01, 0x243185be, 0x550c7dc3,
										0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
										0xc19bf174, 0xe49b69c1, 0xefbe4786,
										0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
										0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
										0x983e5152, 0xa831c66d, 0xb00327c8,
										0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
										0x06ca6351, 0x14292967, 0x27b70a85,
										0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
										0x650a7354, 0x766a0abb, 0x81c2c92e,
										0x92722c85, 0xa2bfe8a1, 0xa81a664b,
										0xc24b8b70, 0xc76c51a3, 0xd192e819,
										0xd6990624, 0xf40e3585, 0x106aa070,
										0x19a4c116, 0x1e376c08, 0x2748774c,
										0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
										0x5b9cca4f, 0x682e6ff3, 0x748f82ee,
										0x78a5636f, 0x84c87814, 0x8cc70208,
										0x90befffa, 0xa4506ceb, 0xbef9a3f7,
										0xc67178f2 };

// Functions
//

static void append_mb_size_be(uint32_t* M, uint32_t size, size_t bytes) {

    const uint32_t mask = 0xE0000000;
    const auto top = (bytes & mask) >> 29;
	M[size-2] = top;
	M[size-1] = GET_U32_BE( bytes << 3 );
}

sha256_result_t sha256(const uint8_t* data, size_t size) {

	// Set the initial hash value(s)
	uint32_t H[] = {
		0x6a09e667,
		0xbb67ae85,
		0x3c6ef372,
		0xa54ff53a,
		0x510e527f,
		0x9b05688c,
		0x1f83d9ab,
		0x5be0cd19
	};

	// Get 'N', the number of blocks
	uint32_t N = (uint32_t)( size / SHA256_MESSAGE_BLOCK_BYTE_SIZE );
	if (size % SHA256_MESSAGE_BLOCK_BYTE_SIZE){
		N++;
	}

	// If the supplementary block isn't large enough to
	// accomodate the trailing size footer..
	const uint32_t delta = ((N * SHA256_MESSAGE_BLOCK_BYTE_SIZE) - size);
	if (delta <= 8){
		N++;
	}

	// Process each block
	const size_t wc = SHA256_MESSAGE_BLOCK_WC;
	for (uint32_t i = 0; i < N; ++i){
		// Initialise an empty block
		uint32_t M[SHA256_MESSAGE_BLOCK_WC];
		clear_message_block( M, SHA256_MESSAGE_BLOCK_WC );

		// Prep it
		const size_t offset = (i * SHA256_MESSAGE_BLOCK_BYTE_SIZE);
		if (offset >= size){
			if (size == offset){
#ifdef __LITTLE_ENDIAN__
				M[0] = 0x00000080;
#else
				M[0] = 0x80000000;
#endif
			}
			append_mb_size_be( M, wc, size );
		}else{
			const uint8_t* p = data + offset;
			const size_t next = (offset + SHA256_MESSAGE_BLOCK_BYTE_SIZE);
			if (next < size){
				// Fill the wc with the bytes in the current block
				for (uint32_t w = 0; w < wc; ++w){
					// Fill the current word with the next four bytes
					uint32_t word = 0;
					for (uint32_t x = 0; x < 32; x += 8){
						const uint32_t y = (uint32_t)( *p++ );
						word |= ((y << x) & (0xFF << x));
					}
					M[w] = word;
				}
			}else{
				const uint32_t bytes = (uint32_t)( size - offset );
				const uint32_t wr = (bytes % sizeof( uint32_t ));
				uint32_t w1 = bytes / sizeof( uint32_t );
				if (wr){
					w1 += 1;
				}

				// w1: the number of whole or partial wc to be filled
				// wr: the number of bytes filled in the last word (w1-1)
				for (uint32_t w0 = 0, wb = bytes; w0 < w1; ++w0){
					uint32_t word = 0;
					const uint32_t xb = min( 32, wb << 3 );
					for (uint32_t x = 0; x < xb; x += 8, --wb){
						const uint32_t y = (uint32_t)( *p++ );
						word |= ((y << x) & (0xFF << x));
					}
					M[w0] = word;
				}

				size_t delta = (SHA256_MESSAGE_BLOCK_BYTE_SIZE - bytes);
				if (delta > 0){
					// The word to be selected is the last partially-filled
					// one or the next one after the last fully-filled one
					const uint32_t w0 = (wr) ? (w1 - 1) : w1;
					uint32_t word = M[w0];

					// Raise the leading bit on the indicated byte within
					// the word, and apply
					const uint32_t x = (wr << 3);
					word |= ((0x80 << x) & (0xFF << x));
					M[w0] = word;

					delta--;
				}else{
					// It'll be handled in the next block..
					;
				}
				if (delta >= 8){
					append_mb_size_be( M, wc, size );
				}
			}
		}

		// Initialise the working variables
		uint32_t a = H[0];
		uint32_t b = H[1];
		uint32_t c = H[2];
		uint32_t d = H[3];
		uint32_t e = H[4];
		uint32_t f = H[5];
		uint32_t g = H[6];
		uint32_t h = H[7];

		// Loop
		uint32_t W[64];
		for (size_t t = 0; t < 64; ++t){
			// Prep the message schedule
			if (t < wc){
#ifdef __LITTLE_ENDIAN__
				W[t] = SWOP_ENDS_U32( M[t] );
#else
				W[t] = M[t];
#endif
			}else{
				W[t] = sigma1( W[t-2] ) + W[t-7] + sigma0( W[t-15] ) + W[t-16];
			}

			uint32_t T1 = h + Sigma1( e ) + Ch( e, f, g ) + c_constants[t] + W[t];
			uint32_t T2 = Sigma0( a ) + Maj( a, b, c );
			h = g;
			g = f;
			f = e;
			e = d + T1;
			d = c;
			c = b;
			b = a;
			a = T1 + T2;
		}

		// Compute the i-th intermediate hash value
		H[0] = a + H[0];
		H[1] = b + H[1];
		H[2] = c + H[2];
		H[3] = d + H[3];
		H[4] = e + H[4];
		H[5] = f + H[5];
		H[6] = g + H[6];
		H[7] = h + H[7];
	}

	// Return 
	sha256_result_t result = { 0 };
	for (size_t s = 0; s < SHA256_WC; ++s){
		const uint32_t v = H[s];

		// The output of the algorithm is big-endian in nature
#ifdef __LITTLE_ENDIAN__
		result.words[s] = SWOP_ENDS_U32( v );
#else
		result.words[s] = v;
#endif
	}
	return result;
}

sha256_result_t sha256w(const uint32_t* words, size_t size) {

	// Set the initial hash value(s)
	uint32_t H[] = {
		0x6a09e667,
		0xbb67ae85,
		0x3c6ef372,
		0xa54ff53a,
		0x510e527f,
		0x9b05688c,
		0x1f83d9ab,
		0x5be0cd19
	};

	// Get 'N', the number of blocks
	uint32_t N = (uint32_t)( size / SHA256_MESSAGE_BLOCK_BYTE_SIZE );
	if (size % SHA256_MESSAGE_BLOCK_BYTE_SIZE){
		N++;
	}

	// If the supplementary block isn't large enough to
	// accomodate the trailing size footer..
	const uint32_t delta = ((N * SHA256_MESSAGE_BLOCK_BYTE_SIZE) - size);
	if (delta <= 8){
		N++;
	}

	// Process each block
	const size_t wc = SHA256_MESSAGE_BLOCK_WC;
	for (uint32_t i = 0; i < N; ++i){
		// Initialise an empty block
		uint32_t M[SHA256_MESSAGE_BLOCK_WC];
		clear_message_block( M, SHA256_MESSAGE_BLOCK_WC );

		// Prep it
		const size_t offset = (i * SHA256_MESSAGE_BLOCK_BYTE_SIZE);
		if (offset >= size){
			if (size == offset){
#ifdef __LITTLE_ENDIAN__
				M[0] = 0x00000080;
#else
				M[0] = 0x80000000;
#endif
			}
			append_mb_size_be( M, wc, size );
		}else{
			const uint32_t* ptr = words + (offset / sizeof( uint32_t ));
			const size_t next = (offset + SHA256_MESSAGE_BLOCK_BYTE_SIZE);
			if (next < size){
				// Fill the message with the bytes in the current block
				for (uint32_t w = 0; w < wc; ++w){
					M[w] = *(ptr++);
				}
			}else{
				// Count the number of outstanding bytes
				const uint32_t bytes = (uint32_t)( size - offset );
				const uint32_t wr = (bytes % sizeof( uint32_t ));
				uint32_t w1 = bytes / sizeof( uint32_t );
				if (wr){
					w1 += 1;
				}

				// w1: the number of whole or partial words to be filled
				// wr: the number of bytes filled in the last word (w1-1)
				for (uint32_t w = 0; w < w1; ++w){
					M[w] = *(ptr++);
				}

				size_t delta = (SHA256_MESSAGE_BLOCK_BYTE_SIZE - bytes);
				if (delta > 0){
					// The word to be selected is the last partially-filled
					// one or the next one after the last fully-filled one
					const uint32_t w0 = (wr) ? (w1 - 1) : w1;
					uint32_t word = M[w0];

					// Raise the leading bit on the indicated byte within
					// the word, and apply
					const uint32_t x = (wr << 3);
					word |= ((0x80 << x) & (0xFF << x));
					M[w0] = word;

					delta--;
				}else{
					// It'll be handled in the next block..
					;
				}
				if (delta >= 8){
					append_mb_size_be( M, wc, size );
				}
			}
		}

		// Initialise the working variables
		uint32_t a = H[0];
		uint32_t b = H[1];
		uint32_t c = H[2];
		uint32_t d = H[3];
		uint32_t e = H[4];
		uint32_t f = H[5];
		uint32_t g = H[6];
		uint32_t h = H[7];

		// Loop
		uint32_t W[64];
		for (size_t t = 0; t < 64; ++t){
			// Prep the message schedule
			if (t < wc){
#ifdef __LITTLE_ENDIAN__
				W[t] = SWOP_ENDS_U32( M[t] );
#else
				W[t] = M[t];
#endif
			}else{
				W[t] = sigma1( W[t-2] ) + W[t-7] + sigma0( W[t-15] ) + W[t-16];
			}

			uint32_t T1 = h + Sigma1( e ) + Ch( e, f, g ) + c_constants[t] + W[t];
			uint32_t T2 = Sigma0( a ) + Maj( a, b, c );
			h = g;
			g = f;
			f = e;
			e = d + T1;
			d = c;
			c = b;
			b = a;
			a = T1 + T2;
		}

		// Compute the i-th intermediate hash value
		H[0] = a + H[0];
		H[1] = b + H[1];
		H[2] = c + H[2];
		H[3] = d + H[3];
		H[4] = e + H[4];
		H[5] = f + H[5];
		H[6] = g + H[6];
		H[7] = h + H[7];
	}

	// Return 
	sha256_result_t result = { 0 };
	for (size_t s = 0; s < SHA256_WC; ++s){
		const uint32_t v = H[s];

		// The output of the algorithm is big-endian in nature
#ifdef __LITTLE_ENDIAN__
		result.words[s] = SWOP_ENDS_U32( v );
#else
		result.words[s] = v;
#endif
	}
	return result;
}