#include "ext2.h"

#include <string>
#include <vector>

using namespace std;

namespace vdi_explorer
{
    class interface
    {
        public:
            interface(ext2 *);
            ~interface();
            
            void interactive();
            
        private:
            enum command_code
            {
                code_unknown = -1,
                code_cd,
                code_cp,
                code_exit,
                code_help,
                code_ls,
                code_pwd
            };
            
            bool command_cd(const std::string &);
            bool command_cp(const std::string &, const std::string &);
            void command_exit();
            bool command_help(const std::string &);
            bool command_ls(const std::string &);
            bool command_pwd();
            
            command_code hash_command(const std::string &);
            
            // pointer to the file system object
            ext2 * file_system = nullptr;
    };
} // namespace vdi_explorer