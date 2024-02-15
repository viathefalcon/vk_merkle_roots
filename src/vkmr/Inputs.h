// Inputs.h: declares the types, functions and classes for reading input
//

#if !defined (__VKMR_INPUTS_H__)
#define __VKMR_INPUTS_H__

// Includes
//

// C++ Standard Library Headers
#include <fstream>
#include <string>

namespace vkmr {

// Class(es)
//

class Input {
public:
    typedef size_t size_type;

    Input(const std::string&);
    Input(Input&&);
    Input(FILE*, bool owner = false);
    Input(const Input&) = delete;
    ~Input(void);

    Input& operator=(const Input&) = delete;
    Input& operator=(Input&&);
    operator bool() const;

    bool Has(void) const;
    std::string Get(void);

    size_type Size(void) const { return m_size; }
    size_type Count(void) const { return m_count; }

private:
    void Reset(void);
    void Release(void);

    FILE* m_fp;
    bool m_owner;
    size_type m_size, m_count;
};

} // namespace vkmr

#endif // __VKMR_INPUTS_H__
