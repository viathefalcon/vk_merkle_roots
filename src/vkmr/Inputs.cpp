// Inputs.cpp: defines the the types, functions and classes for reading input
//

// Includes
//

// C++ Standard Library Headers
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
    m_count( 0U ) {    
}

Input::Input(const std::string& path):
    m_fp( ::fopen( path.c_str( ), "r" ) ),
    m_owner( true ),
    m_size( 0U ),
    m_count( 0U ) {
}

Input::Input(Input&& input):
    m_fp( input.m_fp ),
    m_owner( input.m_owner ),
    m_size( input.m_size ),
    m_count( input.m_count ) {

    input.Reset( );
}

Input::Input(FILE* fp, bool owner):
    m_fp( fp ),
    m_owner( owner ),
    m_size( 0U ),
    m_count( 0U ) {
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

    // Setup
    const auto lf = static_cast<int>( '\n' );

    // Read until we hit the EOF or line-feed
    std::string str;
    for (bool not_finished = true; not_finished; ){
        const auto input = fgetc( m_fp );
        switch (input) {
            case lf:
            case EOF:
                not_finished = false;
                break;

            default:
                // Accumulate
                str.append( 1, static_cast<char>( input ) );
                break;
        }
    }

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