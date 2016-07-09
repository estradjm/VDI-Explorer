// Code from user "Evan Teran" on stackoverflow.com to assist with trimming strings.
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <string>

#include <iostream>

using namespace std;

namespace utility
{
    // trim from start (in place)
    void ltrim(string &s) {
        s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    }
    
    // trim from end (in place)
    void rtrim(string &s) {
        s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    }
    
    // trim from both ends (in place)
    void trim(string &s) {
        ltrim(s);
        rtrim(s);
    }
    
    // trim from start (copying)
    string ltrimmed(string s) {
        ltrim(s);
        return s;
    }
    
    // trim from end (copying)
    string rtrimmed(string s) {
        rtrim(s);
        return s;
    }
    
    // trim from both ends (copying)
    string trimmed(string s) {
        trim(s);
        return s;
    }
    
    // tokenize a string based on a given delimiter
    // to_tokenize should be an already-trimmed string for best results
    // @TODO add quote handling capability
    // @TODO add escape character ('\') handling
    vector<string> tokenize(const string &to_tokenize, const string &delimiter)
    {
        vector<string> to_return;
        unsigned int cursor = 0;
        size_t pos_found = 0;
        string temp;
        
        // loop through the string
        while (cursor < to_tokenize.length())
        {
            // attempt to find the delimiter
            pos_found = to_tokenize.find(delimiter, cursor);
            
            // pull out a token
            temp.assign(to_tokenize.substr(cursor, pos_found - cursor));

            // adjust the cursor position
            cursor += temp.length() + delimiter.length();

            // check the token to be sure it is one
            if (temp == delimiter || temp.length() == 0)
            {
                // if not, continue
                continue;
            }
            
            // add the token to the list of tokens
            to_return.push_back(temp);
        }
        
        // return the tokens
        return to_return;
    }
} // namespace utility