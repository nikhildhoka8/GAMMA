int hasArg(std::vector<std::string> args,std::string searchArg);
bool runBuiltinCommand(std::vector<std::string> args,int output_fd=-1);
bool builtin_cd(std::vector<std::string> args,int output_fd=-1);
bool builtin_alias(std::vector<std::string> args,int output_fd=-1);
bool builtin_unalias(std::vector<std::string> args,int output_fd=-1);
bool builtin_help(std::vector<std::string> args, int output_fd=-1);
bool builtin_history(std::vector<std::string> args,int output_fd=-1);
bool builtin_set(std::vector<std::string> args,int output_fd=-1);
bool builtin_source(std::vector<std::string> args,int output_fd=-1);
bool builtin_shift(std::vector<std::string>args,int output_fd=-1);

void loadHelpPages();
void printHelpPage(std::string page,int output_fd=-1);

extern std::vector<std::string> directoryHistory;
extern std::string currentWorkingDirectory;
//For alias internal command
extern std::unordered_map<std::string,std::string> aliases; //key is alias name, value is alias value
//For history internal command
extern std::vector<std::string> commandHistory;
//For help internal command
extern std::unordered_map<std::string,std::string> helpPages;

//environment variables - use set internal command
extern std::unordered_map<std::string,std::string> environmentVariables;
extern bool STOP_ON_ERROR;
extern bool WILDCARD_GLOBBING;
extern bool ALLOW_OVERWRITE;
extern bool INTERACTIVEMODE;
