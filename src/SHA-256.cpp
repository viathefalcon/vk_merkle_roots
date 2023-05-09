// SHA-256.cpp: defines the class(es), type(s) and function(s) for generating SHA-256 hashes.

// Includes
//

// Windows Headers
#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif // defined (_WIN32)

// C++ Headers
#include <cstring>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

// Local Project Headers
#include "Debug.h"

// Declarations
#include "SHA-256.h"

// Macros
//

#ifndef min
#define min(x,y) (((x) <= (y)) ? (x) : (y))
#endif

// Default to little-endianness?
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

#define clear_message_block(M, words) \
	do { \
		volatile uint32_t* volatile m = M; \
		for (auto w = 0; w < words; ++w){ \
			(*m++) = 0U; \
		} \
	} while (0)

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

static void append_message_block(uint32_t* M, uint32_t size, size_t bytes) {

	const auto bits = GET_U64_BE( static_cast<uint64_t>( bytes ) << 3 );

	uint32_t bottom = 0;
	bottom |= (((uint32_t) (bits >> 56)) & 0xFF) << 24;
	bottom |= (((uint32_t) (bits >> 48)) & 0xFF) << 16;
	bottom |= (((uint32_t) (bits >> 40)) & 0xFF) << 8;
	bottom |= (((uint32_t) (bits >> 32)) & 0xFF);

	uint32_t top = 0;
	top |= (((uint32_t) (bits >> 24)) & 0xFF) << 24;
	top |= (((uint32_t) (bits >> 16)) & 0xFF) << 16;
	top |= (((uint32_t) (bits >> 8)) & 0xFF) << 8;
	top |= (((uint32_t) bits) & 0xFF);

	M[size-1] = bottom;
	M[size-2] = top;
}

static std::vector<uint32_t> cpu_sha256_n(const std::string& s) {

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

	const auto block_size = (512 / 8);
	const auto words = (block_size / sizeof( uint32_t ));

	// Get 'N', the number of blocks
	const auto size = s.size( );
	auto N = static_cast<uint32_t>( size / block_size );
	if (size % block_size){
		N++;
	}

	// If the supplementary block isn't large enough to
	// accomodate the trailing size footer..
	const auto delta = ((N * block_size) - size);
	if (delta <= 8){
		N++;
	}

	// Process each block
	auto ptr = s.data( );
	for (uint32_t i = 0; i < N; ++i){
		// Initialise an empty block
		uint32_t M[words];
		clear_message_block( M, words );

		// Prep it
		const auto offset = (i * block_size);
		if (offset >= size){
			if (size == offset){
#ifdef __LITTLE_ENDIAN__
				M[0] = 0x00000080;
#else
				M[0] = 0x80000000;
#endif
			}
			append_message_block( M, words, size );
		}else{
			const char* p = ptr + offset;
			//debug_print( p, static_cast<unsigned>( min( block_size, size - offset ) ) );

			const auto next = (offset + block_size);
			if (next < size){
				// Fill the words with the bytes in the current block
				for (uint32_t w = 0; w < words; ++w){
					// Fill the current word with the next four bytes
					uint32_t word = 0;
					for (uint32_t x = 0; x < 32; x += 8){
						const uint32_t y = static_cast<uint32_t>( *p++ );
						word |= ((y << x) & (0xFF << x));
					}
					M[w] = word;
				}
			}else{
				const uint32_t bytes = static_cast<uint32_t>( size - offset );
				const uint32_t wr = (bytes % sizeof( uint32_t ));
				uint32_t w1 = bytes / sizeof( uint32_t );
				if (wr){
					w1 += 1;
				}

				// w1: the number of whole or partial words to be filled
				// wr: the number of bytes filled in the last word (w1-1)
				for (uint32_t w0 = 0, wb = bytes; w0 < w1; ++w0){
					uint32_t word = 0;
					const uint32_t xb = min( 32, wb << 3 );
					for (uint32_t x = 0; x < xb; x += 8, --wb){
						const uint32_t y = static_cast<uint32_t>( *p++ );
						word |= ((y << x) & (0xFF << x));
					}
					M[w0] = word;
				}

				auto delta = (block_size - bytes);
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
					append_message_block( M, words, size );
				}
			}
		}
		debug_print_bits_and_bytes( M, words );

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
		for (auto t = 0; t < 64; ++t){
			// Prep the message schedule
			if (t < words){
#ifdef __LITTLE_ENDIAN__
				W[t] = SWOP_ENDS_U32( M[t] );
#else
				W[t] = M[t];
#endif
			}else{
				W[t] = sigma1( W[t-2] ) + W[t-7] + sigma0( W[t-15] ) + W[t-16];
			}

			auto T1 = h + Sigma1( e ) + Ch( e, f, g ) + c_constants[t] + W[t];
			auto T2 = Sigma0( a ) + Maj( a, b, c );
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
	return std::vector<uint32_t>( H, H + 8 );
}

static std::vector<uint32_t> cpu_sha256_1(const std::vector<uint32_t>& u) {

	const auto block_size = (512 / 8);
	const auto words = (block_size / sizeof( uint32_t ));

	// Prepare the sole block
	uint32_t M[words];
	const size_t count = min( words / 2, u.size( ) );
	if (count > 0){
		std::memcpy( M, u.data( ), sizeof( std::vector<uint32_t>::value_type ) * count );
	}
	volatile uint32_t* volatile m = M + count;
	const auto half = (m - M);
	for (auto w = half; w < words; ++w){
		(*m++) = 0U;
	}
#ifdef __LITTLE_ENDIAN__
	M[half] = 0x80000000;
	M[words-1] = 256;
#else
	M[8] = 0x00000080;
	append_message_block( M, words, 32 );
#endif
	//debug_print_bits_and_bytes( M, words );

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
	for (auto t = 0; t < 64; ++t){
		// Prep the message schedule
		if (t < words){
			W[t] = M[t];
		}else{
			W[t] = sigma1( W[t-2] ) + W[t-7] + sigma0( W[t-15] ) + W[t-16];
		}

		auto T1 = h + Sigma1( e ) + Ch( e, f, g ) + c_constants[t] + W[t];
		auto T2 = Sigma0( a ) + Maj( a, b, c );
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;
	}

	// Accumulate and return
	std::vector<uint32_t> v;
	v.push_back( a + H[0] );
	v.push_back( b + H[1] );
	v.push_back( c + H[2] );
	v.push_back( d + H[3] );
	v.push_back( e + H[4] );
	v.push_back( f + H[5] );
	v.push_back( g + H[6] );
	v.push_back( h + H[7] );
	return v;
}

static std::vector<uint32_t> cpu_sha256_2(const std::vector<uint32_t>& u1, const std::vector<uint32_t>& u2) {

	const auto block_size = (512 / 8);
	const auto words = (block_size / sizeof( uint32_t ));

	// Fill the first message block 
	uint32_t M[words];
	clear_message_block( M, words );
	auto count = min( words - 1, u1.size( ) );
	std::memcpy(
		M,
		u1.data( ),
		sizeof( std::vector<uint32_t>::value_type ) * count
	);
	std::memcpy(
		M + count,
		u2.data( ),
		sizeof( std::vector<uint32_t>::value_type ) * min( u2.size( ), words - count )
	);
	//debug_print_bits_and_bytes( M, words );

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
	for (auto i = 0; true; ){
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
		for (auto t = 0; t < 64; ++t){
			// Prep the message schedule
			if (t < words){
				W[t] = M[t];
			}else{
				W[t] = sigma1( W[t-2] ) + W[t-7] + sigma0( W[t-15] ) + W[t-16];
			}

			auto T1 = h + Sigma1( e ) + Ch( e, f, g ) + c_constants[t] + W[t];
			auto T2 = Sigma0( a ) + Maj( a, b, c );
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

		// Prepare the next block?
		if (++i >= 2){
			// Nope..
			break;
		}
		clear_message_block( M, words );
#ifdef __LITTLE_ENDIAN__
		M[0] = 0x80000000;
		M[words-1] = 512;
#else
		M[0] = 0x00000080;
		append_message_block( M, words, 64 );
#endif
		//debug_print_bits_and_bytes( M, words );
	}
	return std::vector<uint32_t>( H, H + 8 );
}

static std::string hash_to_string(const std::vector<uint32_t>& v) {

	std::string result;
	for (auto it = v.cbegin( ), end = v.cend( ); it != end; ++it){
		const auto v = *it;
#ifdef __LITTLE_ENDIAN__
		const uint32_t u = SWOP_ENDS_U32( v );
#else
		const uint u = v;
#endif
		for (uint32_t k = 0; k < sizeof( uint32_t ); ++k){
			uint32_t c = (u >> (k << 3)) & 0xFF;
			result.push_back( (char) c );
		}
	}
	return result;
}

std::string cpu_sha256(const std::string& s) {
	return hash_to_string(
		cpu_sha256_n( s )
	);
}

#define cpu_sha256d_int(s) cpu_sha256_1( cpu_sha256_n( s ) )

std::string cpu_sha256d(const std::string& s) {

	return hash_to_string(
		cpu_sha256d_int( s )
	);
}

std::string cpu_merkle_root_sha256d(const std::vector<std::string>& args) {

	auto be_to_string = [=](const std::vector<uint32_t>& be) -> std::string {
		std::string result;
		for (auto it = be.cbegin( ), end = be.cend( ); it != end; ++it){
			const auto u = *it;
			for (uint32_t k = 0; k < sizeof( uint32_t ); ++k){
				uint32_t c = (u >> (k << 3)) & 0xFF;
				result.push_back( (char) c );
			}
		}
		return result;
	};

	std::vector<std::vector<uint32_t>> u, v;
	u.reserve( args.size( ) );
	std::for_each( args.cbegin( ), args.cend( ), [&u, &be_to_string](const std::string& arg) {
		const auto H = cpu_sha256d_int( arg );

		/*
		debug_print_label( "cpu_sha256d_be: " );
		debug_print_bytes( be_to_string( H ) );
		*/

		u.push_back( H );
	} );
	v.reserve( u.size( ) );

	// Loop until we've reduced to a single element
	auto pin = &u, pout = &v;
	do {
		// Get the number of pairs of elements in the current run
		const auto count = pin->size( );
		auto pairs = count >> 1;
		if (count % 2 != 0){
			++pairs;
		}

		// Loop through them
		pout->clear( );
		for (auto it = pin->cbegin( ), end = pin->cend( ); pairs > 0; --pairs ){
			const auto& l = *(it++);
			const auto& r = (it == end ? l : (*it++));

			// Accumulate the hash of the hash of the concatenation
			const auto h = cpu_sha256_1(
				cpu_sha256_2( l, r )
			);
			pout->push_back( h );

			/*
			std::ostringstream oss;
			oss << "(" << pairs << "/" << count << "): ";
			debug_print_label( oss.str( ) );
			debug_print_bytes( be_to_string( h ) );
			*/
		}

		// Swop the pointers for the next iteration
		auto tmp = pin;
		pin = pout;
		pout = tmp;
	} while (pin->size( ) > 1);

	debug_print_label( "B/E: " );
	debug_print_bytes( be_to_string( pin->front( ) ) );

	return hash_to_string( pin->front( ) );
}
