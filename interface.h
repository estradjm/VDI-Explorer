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
                code_none = -2,
                code_unknown,
                code_cd,
                code_cp,
                code_exit,
                code_help,
                code_ls,
                code_pwd
                // Debug
                , code_dump_pwd_inode
                , code_dump_block
            };
            
            void command_cd(const string &);
            void command_cp(const string &, const string &, const string &);
            void command_exit();
            void command_help(const string &);
            void command_ls(const string &);
            void command_pwd();
            // Debug.
            void command_dump_pwd_inode();
            void command_dump_block(u32);
            // End debug.
            
            command_code hash_command(const string &);
            
            // pointer to the file system object
            ext2 * file_system = nullptr;
    };
} // namespace vdi_explorer