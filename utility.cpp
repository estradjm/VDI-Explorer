// Code from user "Evan Teran" on stackoverflow.com to assist with trimming strings.
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <string>

#include <iostream>

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
    
    // tokenize a string based on a given delimiter
    // to_tokenize should be an already-trimmed string for best results
    // @TODO add quote handling capability
    // @TODO add escape character ('\') handling
    std::vector<std::string> tokenize(const std::string &to_tokenize, const std::string &delimiter)
    {
        std::vector<std::string> to_return;
        unsigned int cursor = 0;
        size_t pos_found = 0;
        std::string temp;
        
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