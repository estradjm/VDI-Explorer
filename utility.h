// Code from user "Evan Teran" on stackoverflow.com to assist with trimming strings.
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

#include <string>

using namespace std;

namespace utility
{
    // trim from start (in place)
    void ltrim(string &);
    
    // trim from end (in place)
    void rtrim(string &);
    
    // trim from both ends (in place)
    void trim(string &);
    
    // trim from start (copying)
    string ltrimmed(string);
    
    // trim from end (copying)
    string rtrimmed(string);
    
    // trim from both ends (copying)
    string trimmed(string);
    
    // tokenize a string based on a given delimiter
    vector<string> tokenize(const string &, const string &);
}