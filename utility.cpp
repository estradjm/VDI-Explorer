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
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    tokenize
     * Type:    Function
     * Purpose: Tokenize a string based on a given delimiter.
     * Input:   const string & to_tokenize, contains the string that will be tokenized.  Should be
     *          an already-trimmed string for best results.
     * Input:   const string & delimiter, contains the string to use as the delimiter.
     * Output:  vector<string>, containing the tokens that the string was broken up into.
     *
     * @TODO    add quote handling capability
     * @TODO    add escape character ('\') handling
    ----------------------------------------------------------------------------------------------*/
    vector<string> tokenize(const string & to_tokenize, const string & delimiter)
    {
        vector<string> to_return;
        unsigned int cursor = 0;
        size_t pos_found = 0;
        string temp;
        
        // Loop through the string.
        while (cursor < to_tokenize.length())
        {
            // Attempt to find the delimiter.
            pos_found = to_tokenize.find(delimiter, cursor);
            
            // Pull out a token.
            temp.assign(to_tokenize.substr(cursor, pos_found - cursor));

            // Adjust the cursor position.
            cursor += temp.length() + delimiter.length();

            // Check the token to be sure it is one.
            if (temp == delimiter || temp.length() == 0)
            {
                // If not, discard it and continue.
                continue;
            }
            
            // Otherwise, add the token to the list of tokens.
            to_return.push_back(temp);
        }
        
        // Return the tokens.
        return to_return;
    }
} // namespace utility