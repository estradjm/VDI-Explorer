#include "ext2.h"

#include <string>
#include <vector>

namespace vdi_explorer
{
    class interface
    {
        public:
            interface(ext2 *);
            ~interface();
            
            void interactive();
            
        private:
            bool command_cd(std::string);
            bool command_cp(std::string);
            bool command_ls(std::string);
            void command_exit();
            
            std::vector<std::string> parse_args(std::string &);
            
            ext2* file_system = nullptr;
    };
} // namespace vdi_explorer