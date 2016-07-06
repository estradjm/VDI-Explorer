// Code from user "Evan Teran" on stackoverflow.com to assist with trimming strings.
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <string>

namespace utility
{
    // trim from start (in place)
    void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    }
    
    // trim from end (in place)
    void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    }
    
    // trim from both ends (in place)
    void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }
    
    // trim from start (copying)
    std::string ltrimmed(std::string s) {
        ltrim(s);
        return s;
    }
    
    // trim from end (copying)
    std::string rtrimmed(std::string s) {
        rtrim(s);
        return s;
    }
    
    // trim from both ends (copying)
    std::string trimmed(std::string s) {
        trim(s);
        return s;
    }
} // namespace utility