// Code from user "Evan Teran" on stackoverflow.com to assist with trimming strings.
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

#include <string>

namespace utility
{
    // trim from start (in place)
    void ltrim(std::string &);
    
    // trim from end (in place)
    void rtrim(std::string &);
    
    // trim from both ends (in place)
    void trim(std::string &);
    
    // trim from start (copying)
    std::string ltrimmed(std::string);
    
    // trim from end (copying)
    std::string rtrimmed(std::string);
    
    // trim from both ends (copying)
    std::string trimmed(std::string);
}