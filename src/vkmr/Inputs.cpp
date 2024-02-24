// Inputs.cpp: defines the the types, functions and classes for reading input
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <iostream>

// Declarations
#include "Inputs.h"

namespace vkmr {

// Classes
//

Input::Input(void):
    m_fp( NULL ),
    m_owner( false ),
    m_size( 0U ),
    m_count( 0U ),
    m_delim_width( max_string_length<delim_type>( ) ) {    
}

Input::Input(const std::string& path):
    m_fp( ::fopen( path.c_str( ), "r" ) ),
    m_owner( true ),
    m_size( 0U ),
    m_count( 0U ),
    m_delim_width( max_string_length<delim_type>( ) ) {
}

Input::Input(Input&& input):
    m_fp( input.m_fp ),
    m_owner( input.m_owner ),
    m_size( input.m_size ),
    m_count( input.m_count ),
    m_delim_width( max_string_length<delim_type>( ) ) {

    input.Reset( );
}

Input::Input(FILE* fp, bool owner):
    m_fp( fp ),
    m_owner( owner ),
    m_size( 0U ),
    m_count( 0U ),
    m_delim_width( max_string_length<delim_type>( ) ) {
}

Input::~Input(void) {
    Release( );
}

bool Input::Has(void) const {
    return !(::feof( m_fp ));
}

Input& Input::operator=(Input&& input) {

    if (&input != this){
        this->Release( );

        m_fp = input.m_fp;
        m_owner = input.m_owner;
        m_size = input.m_size;
        m_count = input.m_count;

        input.Reset( );
    }
    return (*this);
}

Input::operator bool(void) const {
    return static_cast<bool>( m_fp );
}

std::string Input::Get(void) {

    // Read in the length
    std::string str;
    do {
        const auto input = fgetc( m_fp );
        if (input == EOF){
            // Nope, bail
            return "";
        }

        // Accumulate
        str.append( 1, static_cast<char>( input ) );
    } while (str.size( ) < m_delim_width);

    // Parse
    const auto length = static_cast<decltype(str)::size_type>(
        std::atol( str.c_str( ) ) // 'long' because actual type is unsigned and could overflow an int
    );
    if (length < 1){
        // Bail
        return "";
    }
    str.clear( );

    // Read in the string itself
    str.reserve( static_cast<size_t>( length ) );
    do {
        const auto input = fgetc( m_fp );
        if (input == EOF){
            break;
        }

        // Accumulate
        str.append( 1, static_cast<char>( input ) );
    } while (str.size( ) < length);

    // Tally up and return
    m_size += str.size( );
    m_count += str.empty( ) ? 0 : 1; 
    return str;
}

void Input::Reset(void) {
    m_fp = NULL;
    m_owner = false;
    m_size = m_count = 0U;
}

void Input::Release(void) {
    if (m_owner && m_fp){
        ::fclose( m_fp );
    }
    Reset( );
}

} // namespace vkmr