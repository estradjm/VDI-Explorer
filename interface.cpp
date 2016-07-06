#include "ext2.h"
#include "interface.h"
#include "utility.h"

#include <iostream>
#include <iomanip>
#include <string>

namespace vdi_explorer
{
    // @TODO Make an actual throwable error.
    interface::interface(ext2* _file_system)
    {
        file_system = _file_system;
        if (file_system == nullptr)
        {
            std::cout << "Error opening the file system object.";
            throw;
        }
    }
    
    interface::~interface()
    {
        return;
    }
    
    void interface::interactive()
    {
        
    }
    
    bool interface::command_cd(std::string args)
    {
        // stub
        return false;
    }
    
    bool interface::command_cp(std::string args)
    {
        // stub
        return false;
    }
    
    bool interface::command_ls(std::string args)
    {
        // stub
        return false;
    }
    
    void interface::command_exit()
    {
        // stub
        return;
    }
    
    std::vector<std::string> parse_args(std::string &args)
    {
        std::vector<std::string> to_return;
        
        // Trim leading and trailing whitespace.
        utility::trim(args);
        
        if (args.length() == 0)
        {
            return to_return;
        }
        
        
        // Split the string into tokens.
        // <stuff>
        // and <things>
        
        return to_return;
    }
} // namespace vdi_explorer