#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <experimental/filesystem>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unordered_map>
#include <cstring>

const int TABLE_SIZE = 2;

//terminal environment variables
bool STOP_ON_ERROR = true;
bool WILDCARD_GLOBBING = true;
bool ALLOW_OVERWRITE = true;
bool INTERACTIVEMODE;

struct Node;
struct Bucket;
class HashMap;


// Function prototypes
void processFile(const std::string& filePath, HashMap& map);
void handleQuestionMark(std::string &token, std::vector<std::string> &tokens, int questionMarkIndex);
void handleAsterisk(std::string &token, std::vector<std::string> &tokens, int asteriskIndex);
void handleBracket(std::string &token, std::vector<std::string> &tokens, int leftBracketIndex, int rightBracketIndex);
void interactiveMode();
bool postPrepareLine(std::vector<std::string>& lineVector);
void batchMode(int argc, char* argv[]);
std::vector<std::string> tokenizeLine(const std::string &line);
bool forkLinux(const std::vector<std::string>& args, int input_fd = STDIN_FILENO, int output_fd = STDOUT_FILENO, int error_fd = STDERR_FILENO);
int hasArg(std::vector<std::string> args,std::string searchArg);
bool processLine(std::vector<std::string> & args);
bool prepareLine(std::string &line);

//internal command function prototypes
bool runBuiltinCommand(std::vector<std::string> args,int output_fd=-1);
bool builtin_cd(std::vector<std::string> args,int output_fd=-1);
bool builtin_alias(std::vector<std::string> args,int output_fd=-1);
bool builtin_unalias(std::vector<std::string> args,int output_fd=-1);
bool builtin_help(std::vector<std::string> args, int output_fd=-1);
bool builtin_history(std::vector<std::string> args,int output_fd=-1);
bool builtin_set(std::vector<std::string> args,int output_fd=-1);

//Internal command variables and helper function prototypes
//For CD internal command
std::vector<std::string> directoryHistory;
std::string currentWorkingDirectory = "/home/user";
//For alias internal command
std::unordered_map<std::string,std::string> aliases; //key is alias name, value is alias value
//For history internal command
std::vector<std::string> commandHistory;
//For help internal command
std::unordered_map<std::string,std::string> helpPages;
void loadHelpPages();
void printHelpPage(std::string page,int output_fd=-1);
//environment variables - use set internal command
std::unordered_map<std::string,std::string> environmentVariables;


struct Node {
    std::string functionName;
    Node* next;

    Node(const std::string& name = "", Node* nxt = nullptr)
        : functionName(name), next(nxt) {}
};

struct Bucket {
    std::string categoryName;
    Node* head;

    Bucket() : categoryName(""), head(nullptr) {}
};

class HashMap {
public:
    std::vector<Bucket> table;

    HashMap() : table(TABLE_SIZE) {}

    int hashFunction(const std::string& categoryName) {
        int hash = 0;
        for (char c : categoryName) {
            hash += c;
        }
        return hash % TABLE_SIZE;
    }

    void insert(const std::string& categoryName, const std::string& functionName) {
        int index = hashFunction(categoryName);
        Node* newNode = new Node(functionName);
        if (!table[index].head) {
            table[index].categoryName = categoryName;
            table[index].head = newNode;
        } else {
            Node* current = table[index].head;
            while (current->next) {
                current = current->next;
            }
            current->next = newNode;
        }
    }


    //create a search function that returns the categoryName. and if it does not exist, return an empty string
    std::string search(const std::string& functionName) {
        for (auto& bucket : table) {
            Node* current = bucket.head;
            while (current) {
                if (current->functionName == functionName) {
                    return bucket.categoryName;
                }
                current = current->next;
            }
        }
        return "";
    }

    void printTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << "Bucket " << i << ":" << table[i].categoryName << "->";
            Node* current = table[i].head;
            while (current) {
                std::cout << current->functionName << " ->";
                current = current->next;
            }
            std::cout << "nullptr\n";
        }
    }

    ~HashMap() {
        for (auto& bucket : table) {
            Node* current = bucket.head;
            while (current != nullptr) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
    }
};
HashMap map;

void processFile(const std::string& filePath, HashMap& map) {
    std::ifstream file(filePath);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::getline(iss, token, ':'); // Read category name
        std::string categoryName = token;

        while (std::getline(iss, token, ',')) {
            map.insert(categoryName, token);
        }
    }
}

int main(int argc, char *argv[]){
        currentWorkingDirectory = std::experimental::filesystem::current_path();
    loadHelpPages();
    processFile("words.txt", map);
    std::cin.sync_with_stdio(false);
    if(std::cin.rdbuf()->in_avail()!=0){
            INTERACTIVEMODE = false;
            std::string line;
            while(getline(std::cin,line)){
                std::cout << line << std::endl;
                        prepareLine(line);
            }
    }
    else{
        //check for the remaining 3 conditions
        if(argc==1 && !std::cin.eof()){
                INTERACTIVEMODE==true;
                interactiveMode();
        }
        else if(argc >= 2){
                INTERACTIVEMODE = false;
                batchMode(argc,argv);
        }
    }
    return 0;
}

void interactiveMode(){
    //get the current working directory
    currentWorkingDirectory = std::experimental::filesystem::current_path();

    bool keepGoing = true;
    while(keepGoing){
        std::cout << currentWorkingDirectory + "/ GAMMA$:";
        std::string line;
        std::string concatLine;
        std::getline(std::cin, line);
        if(line=="exit"){
            keepGoing = false;
            break;
        }
        while(line.find("\\n")==line.size()-2){
                std::cout<<">>>";
                getline(std::cin,concatLine);
                line = line.substr(0,line.size()-2)+concatLine;
        }
	bool originalLine = true;
        //this is the history shortcut: !{n} will call the nth last command, !! will call the last command
        if(line.size()>=2){
                if(line.at(0)=='!'){
                        if(line.at(1)=='!'){
                                int historyFinalIndex = commandHistory.size()-1;
                                if(historyFinalIndex>=0){
                                        line=commandHistory.at(historyFinalIndex);
                                        originalLine=false;
                                }
                        }else {
                                try{
                                        int historyNum = stoi(line.substr(1));
                                        int historyIndex = commandHistory.size()-historyNum;
                                        if(historyIndex >=0){
                                                line =commandHistory.at(historyIndex);
                                                originalLine=false;
                                        }
                                } catch (std::exception e){
                                        e.what();
                                }
                        }
                        //else, proceed as normal
                }
        }
        //only push to history if the line is original, don't do it if line was changed to a line in history
        if(originalLine){
                if(commandHistory.size()>0&&commandHistory.at(commandHistory.size()-1)!=line){
                        commandHistory.push_back(line);
                } else if (commandHistory.size()==0){
                        commandHistory.push_back(line);
                }
        }
        prepareLine(line);

    }
}

void batchMode(int argc, char* argv[]){
        std::string filePath = argv[1];
        std::ifstream file(filePath);
        std::string line;
        std::string concatLine;
        if (!file.is_open()) {
                std::cerr << "Error opening file: " << filePath << std::endl;
                return;
        }
        while (std::getline(file, line)) {

                if(line.empty()){
                        continue;
                }
                while(line.find("\\n")==line.size()-2){
                        std::getline(file,concatLine);
                        line = line.substr(0,line.size()-2)+concatLine;
                }
                bool originalLine = true;
                //this is the history shortcut: !{n} will call the nth last command, !! will call the last command
                if(line.size()>=2){
                        if(line.at(0)=='!'){
                                if(line.at(1)=='!'){
                                        int historyFinalIndex = commandHistory.size()-1;
                                        if(historyFinalIndex>=0){
                                                line=commandHistory.at(historyFinalIndex);
                                                originalLine=false;
                                        }
                                }else {
                                        try{
                                                int historyNum = stoi(line.substr(1));
                                                int historyIndex = commandHistory.size()-historyNum;
                                                if(historyIndex >=0){
                                                        line =commandHistory.at(historyIndex);
                                                        originalLine=false;
                                                }
                                        } catch (std::exception e){
                                                e.what();
                                        }
                                }
                                //else, proceed as normal
                        }
                }
                if(commandHistory.size()>0&&commandHistory.at(commandHistory.size()-1)!=line){
                        commandHistory.push_back(line);
                } else if (commandHistory.size()==0){
                        commandHistory.push_back(line);
                }
               bool executedNormally = prepareLine(line);
	       if(!executedNormally && STOP_ON_ERROR){
			break;
		}
        }
}

std::vector<std::string> tokenizeLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false; // Flag to track whether we're inside quotes

    for (char ch : line) {
        // Check if the current character is a quote
        if (ch == '\"') {
            inQuotes = !inQuotes; // Flip the inQuotes flag
            continue; // Skip adding the quote character to the token
        }

        // Check if the character is one of the specified delimiters
        if (!inQuotes && (ch == '&' || ch == ';' || ch == '(' || ch == ')' || ch == '<' ||  ch == ' ' || ch == '\t' || ch == '\n')) {
            // If the token is not empty, add it to the list of tokens
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear(); // Clear the token to start a new one
            }
        } else {
            // If the character is not a delimiter, or we are inside quotes, add it to the current token
            token += ch;
        }
    }

    // Add the last token if it's not empty
    if (!token.empty()) {
        tokens.push_back(token);
    }

    for (auto& token : tokens) {
        if(token[0] == '\\' && token[1] == 'n'){
            token = token.substr(2);
        }
    }
    return tokens;
}
//path

//arg checker
//returns 0 if the arg isn't found
//returns index if it is ; if itsa numeric arg returns the value
int hasArg(std::vector<std::string> args,std::string searchArg){
    if(searchArg == "-{n}"){
        for(int i =0;i<args.size();i++){
                try{
                        int returned = stoi(args[i]);
                        return returned;
                } catch (...) {

                }
        }
        return 0;
    }
    for(int i =0;i<args.size();i++){
        if(args.at(i)==searchArg){
                return i;
        }
    }
    return 0;
}


void handleQuestionMark(std::string &token, std::vector<std::string> &tokens, int questionMarkIndex){
        //run the command on every file/directory that matches the ? in the current directory
        std::string leftArgs = token.substr(0,questionMarkIndex);
        std::string rightArgs = token.substr(questionMarkIndex+1);

        if (leftArgs.empty() || rightArgs.empty()) {
        return; // Ensure both left and right args are present
        }
        //the file should have both left and right args
        if(leftArgs.size() > 0 && rightArgs.size() > 0){
                std::experimental::filesystem::directory_iterator dir(currentWorkingDirectory);
                int currentPositon = 0;
                for(auto& p: dir){
                        //match everything with the token that has the left and the right args but only the position of the ? can or cannot be different
                        //the file should have both left and right args and can only have any other character in between
                        std::string filename = p.path().filename().string();

                        // Check if filename matches the pattern: leftArgs + any single character + rightArgs
                        if (filename.size() == leftArgs.size() + 1 + rightArgs.size() && filename.substr(0, leftArgs.size()) == leftArgs && filename.substr(filename.size() - rightArgs.size()) == rightArgs){
                                //replace the current token with the file name
                                // if it is the first iteration of the directory, then replace the token, else duplicate the command and add it to the tokens array
                                if(currentPositon == 0){
                                        token = p.path().filename().string();
                                        currentPositon++;
                                } else {
                                        tokens.insert(tokens.begin() + currentPositon , p.path().filename().string());
                                        ++currentPositon;
                                }
                        }
                }
        }
}

void handleAsterisk(std::string &token, std::vector<std::string> &tokens, int asteriskIndex) {
    std::string leftArgs, rightArgs;
    // Determine the left and right arguments based on the position of the asterisk
        if (asteriskIndex >=0) {
                leftArgs = token.substr(0, asteriskIndex);
                rightArgs = token.substr(asteriskIndex + 1);
                //std::cout << "Left Args: " << leftArgs << std::endl;
               	//std::cout << "Right Args: " << rightArgs << std::endl;
        }

        std::experimental::filesystem::directory_iterator dir(currentWorkingDirectory);
        int currentPositon = 0;
        for (auto& p : dir) {
                std::string filename = p.path().filename().string();

                // Check for matches based on the asterisk position and presence of left/right args
                bool matchFound = false;

                if (!leftArgs.empty() && !rightArgs.empty()) {
                // Match any file that contains leftArgs and rightArgs with any characters in between
                        matchFound = filename.find(leftArgs) != std::string::npos && filename.find(rightArgs) != std::string::npos;
                }
                else if (asteriskIndex == 0 && !rightArgs.empty()) {
                // Match any file that ends with rightArgs
                        matchFound = filename.size() >= rightArgs.size() && filename.compare(filename.size() - rightArgs.size(), rightArgs.size(), rightArgs) == 0;
                } else {
                // Match any file that starts with leftArgs
                        matchFound = filename.size() >= leftArgs.size() && filename.compare(0, leftArgs.size(), leftArgs) == 0;
                }

                if (matchFound) {
                        if(currentPositon == 0){
                                token = p.path().filename().string();
                                currentPositon++;
                        } else {
                                tokens.insert(tokens.begin() + currentPositon , p.path().filename().string());
                                ++currentPositon;
                        }
                }
        }
}

void handleBracket(std::string &token, std::vector<std::string> &tokens, int leftBracketIndex, int rightBracketIndex){
        //std::cout << "Here in handleBracket" << std::endl;
        std::string leftArgs = token.substr(0,leftBracketIndex);
        std::string rightArgs = token.substr(rightBracketIndex+1);
        if(leftArgs.empty() || rightArgs.empty()){
                return;
        }
        //look into the bracket to see the characters or numbers that are in the bracket
        std::string bracketContent = token.substr(leftBracketIndex+1,rightBracketIndex-leftBracketIndex-1);
        //find the file names that have any character or number in the bracket. IF so append it to the tokens like for the asterisk and question mark
        std::experimental::filesystem::directory_iterator dir(currentWorkingDirectory);
        int currentPositon = 0;
        for(auto& p: dir){
                std::string filename = p.path().filename().string();
                if(filename.size() == leftArgs.size() + 1 + rightArgs.size() && filename.substr(0, leftArgs.size()) == leftArgs && filename.substr(filename.size() - rightArgs.size()) == rightArgs){
                        for (char c : bracketContent) {
                                if (filename.find(c) != std::string::npos) {
                                        if(currentPositon == 0){
                                                token = p.path().filename().string();
                                                currentPositon++;
                                        } else {
                                                tokens.insert(tokens.begin() + currentPositon , p.path().filename().string());
                                                ++currentPositon;
                                        }
                                }
                        }
                }
        }
}

std::vector<std::string> splitLineBySemicolon (std::string& line){
        std::istringstream lineStream(line);
        std::vector<std::string> lineVector;
	std::string splitLineString;
	while(getline(lineStream,splitLineString,';')){
		lineVector.push_back(splitLineString);
	}
        return lineVector;
}
std::vector<std::vector<std::string>> splitTokensByPipe(std::vector<std::string>& tokens){
        std::vector<std::vector<std::string>> splitTokens;
        int currentVector = 0;
        std::vector<std::string> pipedCommand;
        splitTokens.push_back(pipedCommand);
        for(std::string token: tokens){
                if(token =="|"){
                        std::vector<std::string> newPipedCommand;
                        splitTokens.push_back(newPipedCommand);
                        currentVector++;
                } else {
                        splitTokens.at(currentVector).push_back(token);
                }
        }
        return splitTokens;
}
void applyAliases(std::vector<std::string>& tokens){
        if(tokens.size()>0){
                std::string command = tokens[0];
        	if(aliases.find(command)!=aliases.end()){
                       	//there is an alias for the associated command
                       	std::string aliasValue = aliases.find(command)->second;
                       	std::vector<std::string> aliasTokens = tokenizeLine(aliasValue);
                       	std::vector<std::string> temp;
                   	for(int i=0;i<aliasTokens.size();i++){
                        	temp.push_back(aliasTokens.at(i));
                       	}
                       	for(int i=1;i<tokens.size();i++){
                               	temp.push_back(tokens.at(i));
                      	}
                       	tokens=temp;
              	}
        }
}
void changeEnvVariables(std::vector<std::string>& tokens){
        if(tokens.size()>1){
                for(int i=1;i<tokens.size();i++){
                        if(tokens.at(i).size()>0){
                                if(tokens.at(i).at(0)=='$'){
                                        //potential environment variable
                                        auto it = environmentVariables.find(tokens.at(i));
                                        if(it!=environmentVariables.end()){
                                                tokens.at(i)=it->second;
                                        }
                                }
                        }
                }
        }
}
void globWildcards(std::vector<std::string>& tokens){
        for (auto& token : tokens) {
                //std::cout << "Token: " << token << std::endl;
                int asterikIndex = token.find("*");
                int questionMarkIndex = token.find("?");
                if(asterikIndex >=0){
                        handleAsterisk(token,tokens,asterikIndex);
                }
                //std::cout<<"Post asterisk\n";
                if(questionMarkIndex>=0){
                        handleQuestionMark(token,tokens,questionMarkIndex);
                }
                //std::cout<<"Post questionmark\n";
                int leftBracketIndex = token.find("[");
                int rightBracketIndex = token.find("]");
                if(leftBracketIndex>=0 && rightBracketIndex>=0){
                        handleBracket(token,tokens,leftBracketIndex,rightBracketIndex);
                }
                //std::cout<<"Post brackets\n";
        }
}
bool forkLinux(const std::vector<std::string>& args, int input_fd, int output_fd, int error_fd) {
    if (args.empty()) return false; // Sanity check

    std::vector<char*> argv; // Prepare arguments for execvp
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); // execvp expects a null-terminated array

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork" << std::endl;
        return false;
    } else if (pid == 0) {
        // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        if (error_fd != STDERR_FILENO) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }
        execvp(argv[0], argv.data());
        // If execvp returns, it must have failed.
        std::cerr << "Failed to execute command: " << args[0] << std::endl;
        exit(EXIT_FAILURE);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
bool runBuiltinCommand(std::vector<std::string> args,int output_fd){
        std::string command = args[0];
        if(command == "cd"){
                return builtin_cd(args,output_fd);
        } else if (command == "alias"){
                return builtin_alias(args,output_fd);
        } else if (command == "unalias"){
                return builtin_unalias (args,output_fd);
        } else if (command =="history"){
                return builtin_history(args,output_fd);
        } else if (command =="help"){
                return builtin_help(args,output_fd);
        } else if (command =="set"){
                return builtin_set(args,output_fd);
        } else{
                std::cout << "Internal command not implemented" << std::endl;
                return false;
        }
}
bool prepareLine(std::string& line){
        //comments check
        int hashtag = line.find("#");
        if(hashtag>=0){
                int escaped = line.find("\\");
                //if it's not escaped, cut the line off at the hashtag
                if(!(escaped!=-1&&escaped==hashtag-1)){
                        line = line.substr(0,hashtag);
                }
        }
	//split lines by ;
	std::vector<std::string> lineVector=splitLineBySemicolon(line);

        //check if my first string has if and my last string has endif
        //if so, then execute the commands in between
        bool hasIf = false;
        bool hasEndIf = false;
        int elseIndex = -1;
        //find the string that only has end
        for(int i = 0;i<lineVector.size();i++){
                if(lineVector.at(i).find("else")!=std::string::npos && lineVector.at(i).find("elseif")==std::string::npos){
                        elseIndex = i;
                        //break or else elseIndex becomes the latest else occurance in a line
                        break;
                }
        }
        //check the firstIndex and lastIndex of the if and endif
        std::string firstIndex;
        if(lineVector.size()>0){
                firstIndex = lineVector.at(0);
        }
        //check if firstIndex has "if"
        if(firstIndex.find("if")!=std::string::npos){
                hasIf = true;
        }

        std::string lastIndex;
        if(lineVector.size()>0){
                lastIndex= lineVector.at(lineVector.size()-1);
        }
        //check if lastIndex has "endif"
        if(lastIndex.find("endif")!=std::string::npos){
                hasEndIf = true;
        }

        if(hasIf && hasEndIf){
                std::vector<std::string> ifVector;
                std::vector<std::vector<std::string>> elseIfVector;
                std::vector<std::string> elseVector;
                ifVector.push_back(lineVector.at(0).substr(3));
                bool ifThenGoing = true;
                int currentIndex = 1;
                while(ifThenGoing){
                        //if the currentIndex has then, push it to the if vector
                        if(!(lineVector.at(currentIndex).find("then")!=std::string::npos)){
                                ifThenGoing = false;
                        } else {
                                ifVector.push_back(lineVector.at(currentIndex).substr(5));
                        }
                        currentIndex++;
                }
                if(elseIndex == -1){
                        elseIndex = lineVector.size()-1;
                }
                for(int i = currentIndex -1;i<=elseIndex;i++){
                        std::vector<std::string> elseIfVectorCurrentItr;
                        if (lineVector.at(i).find("elseif") != std::string::npos){
                                elseIfVectorCurrentItr.push_back(lineVector.at(i).substr(7));
                                bool elseIfGoing = true;
                                int j = i + 1;
                                while(elseIfGoing){
                                        if(lineVector.at(j).find("then")!=std::string::npos){
                                                elseIfVectorCurrentItr.push_back(lineVector.at(j).substr(5));
                                        } else {
                                                elseIfVector.push_back(elseIfVectorCurrentItr);
                                                elseIfGoing = false;
                                        }
                                        j++;
                                }
                        }
                }
                for(int i = elseIndex;i<lineVector.size();i++){
                        if(lineVector.at(i).find("else")!=std::string::npos){
                                bool elseGoing = true;
                                int j = i ;
                                while(elseGoing){
                                        if(lineVector.at(j).find("endif")!=std::string::npos){
                                                elseGoing = false;
                                                i=j;
                                        } else {
                                                elseVector.push_back(lineVector.at(j).substr(5));
                                        }
                                        j++;
                                }
                        }
                }
                //run the first command of the ifVector, if that is true then run everything else in the if vector
                //if the first command is false, then check the elseifs. iF any elseif is true, do not run the rest
                //if no elseif is true, then run the else command
                //if there is no else command, then do nothing

                //print all the vectors
                // for(std::string s:ifVector){
                //         std::cout << s << std::endl;
                // }
                // int counter  = 0;
                // std::cout << "ElseIfVector size: " << elseIfVector.size() << std::endl;
                // for(std::vector<std::string> s:elseIfVector){
                //         std::cout << "ElseIfVector " << counter << std::endl;
                //         for(std::string s1:s){
                //                 std::cout << s1 << std::endl;
                //         }
                //         counter++;
                // }
                // //print else vector
                // std::cout << "ElseVector" << std::endl;
                // for(std::string s:elseVector){
                //         std::cout << s << std::endl;
                // }

                std::vector <std::string> firstIfVector;
                firstIfVector.push_back(ifVector.at(0));
                if(postPrepareLine(firstIfVector)){
                        std::vector<std::string> ifVectorThen;
                        for(int i = 1;i<ifVector.size();i++){
                                ifVectorThen.push_back(ifVector.at(i));
                        }
                        postPrepareLine(ifVectorThen);
                } else {
                        bool elseIfExecuted = false;
                        for(int i = 0;i<elseIfVector.size();i++){
                                std::vector<std::string> elseIfVectorCurrent;
                                elseIfVectorCurrent.push_back(elseIfVector.at(i).at(0));

                                if(postPrepareLine(elseIfVectorCurrent)){
                                        std::vector<std::string> elseIfVectorThen;
                                        for(int j = 1;j<elseIfVector.at(i).size();j++){
                                                elseIfVectorThen.push_back(elseIfVector.at(i).at(j));
                                        }
                                        postPrepareLine(elseIfVectorThen);
                                        elseIfExecuted = true;
                                        break;
                                }

                        }
                        if(!elseIfExecuted){
                                if(elseVector.size()>0){
                                        postPrepareLine(elseVector);
                                }
                        }
                }

        }
        else{
                return postPrepareLine(lineVector);
        }
        return true;
}

bool postPrepareLine(std::vector<std::string>& lineVector){
        //tokenize, apply aliases, substitute environment variables, glob wildcards, do pipes, and redirection on each line
        for(std::string line:lineVector){
		std::vector<std::string> tokens = tokenizeLine(line);

        	applyAliases(tokens);
                changeEnvVariables(tokens);
		if(WILDCARD_GLOBBING){
			globWildcards(tokens);
		}
		//handle the |, >, >>
        	int pipeIndex = hasArg(tokens,"|");
                int writeIndex = hasArg(tokens,">");
                int appendIndex = hasArg(tokens,">>");
                int stdErrorIndex = hasArg(tokens,"2>");
                int stdErrorAppendIndex = hasArg(tokens,"2>>");
                int ifINDEX = hasArg(tokens,"if");
                int thenINDEX = hasArg(tokens,"then");

                //if a line has a pipe or redirection, it won't need to be executed, because the pipe or redirect lines will handle it

                bool needsExecuted=true;
                //if pipeIndex
                //make another vector split on pipes
                //for each command -> determine if keyword,command,other
                //      make pipes array by 2x vector size, if keyword or command , end whole process if other
                //if final command has > or >>, prepare that pipe to the file to append or overwrite to WITH THE FUNCTION
        	if(pipeIndex){
                        needsExecuted=false;
                        std::vector<std::vector<std::string>> pipedCommands = splitTokensByPipe(tokens);
                        int size = pipedCommands.size();
                        bool isCommand[size];
                        int i =0;
                        //look ahead to process if each command is either a command,keyword, or other
                        //command will not execute if there is a keyword at any point in the chain
                        for(std::vector<std::string> command:pipedCommands){
                                std::string result = map.search(command.at(0));
                                if(result == "Internal commands"){
                                        isCommand[i]=true;
                                } else if (result == "Keywords"){
                                        std::cout<<"Error: Keyword " << command.at(0) << " cannot be the name of a function!";
                                        return false;
                                } else {
                                        isCommand[i]=false;
                                }
                                i++;
                        }
                        int pipesArray[size][2];
                        i=0;
                        for(std::vector<std::string> command:pipedCommands){
                                if(isCommand[i]){
                                        //internal command
                                        pipe(pipesArray[i]);
                                        bool ranOK=runBuiltinCommand(command,pipesArray[i][1]);
                                        if(!ranOK){
                                                close(pipesArray[i-1][0]);
                                                close(pipesArray[i][1]);
                                                close(pipesArray[i-1][1]);
                                                close(pipesArray[i][0]);
                                                return false;
                                        }
                                        close(pipesArray[i][1]);
                                        if(i==size-1){
                                                close(pipesArray[i-1][0]);
                                                close(pipesArray[i][1]);
                                                close(pipesArray[i-1][1]);
                                                close(pipesArray[i][0]);
                                        } else if (i!=0){
                                                close(pipesArray[i-1][0]);
                                                close(pipesArray[i][1]);
                                        }
                                } else {
                                        //linux command
                                        pipe(pipesArray[i]);
                                        if(i==0){
                                                //first command, pipe in stdin
                                                bool ranOK = forkLinux(command,STDIN_FILENO,pipesArray[i][1]);
                                                if(!ranOK && STOP_ON_ERROR){
                                                        return ranOK;
                                                }
                                                close(pipesArray[i][1]);
                                        } else if (i==size-1){
                                                //final command, pipe out stdout

                                                //redirection has to happen here, check if its needed or execute as normal
                                                if(!writeIndex && !appendIndex){
                                                        bool ranOK = forkLinux(command,pipesArray[i-1][0],STDOUT_FILENO);
                                                        if(!ranOK && STOP_ON_ERROR){
                                                                return ranOK;
                                                        }
                                                        close(pipesArray[i-1][0]);
                                                        close(pipesArray[i][1]);
                                                        close(pipesArray[i-1][1]);
                                                        close(pipesArray[i][0]);
                                                } else {
                                                        std::vector<std::string> newCommand;
                                                        for(std::string token : command){
                                                                if(token == ">" || token == ">>"){
                                                                        break;
                                                                } else {
                                                                        newCommand.push_back(token);
                                                                }
                                                        }
                                                        if(!(forkLinux(newCommand,pipesArray[i-1][0],pipesArray[i][1]))){
                                                                close(pipesArray[i-1][0]);
                                                                close(pipesArray[i][1]);
                                                                close(pipesArray[i-1][1]);
                                                                close(pipesArray[i][0]);
                                                                return false;
                                                        }
                                                        int output_fd;
                                                        if(writeIndex){
                                                                bool exists = std::experimental::filesystem::exists(currentWorkingDirectory+"/"+tokens.at(writeIndex+1));
                                                                if(!ALLOW_OVERWRITE && exists){
                                                                        std::cout<<"Error! ALLOW_OVERWRITE is set to false,and file " << tokens.at(writeIndex+1) << " already exists!\n";
                                                                        return false;
                                                                } else {
                                                                        output_fd = open(tokens.at(writeIndex+1).c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                                                                }
                                                        } else {
                                                                output_fd = open(tokens.at(appendIndex+1).c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                                        }

                                                        if(output_fd == -1){
                                                                std::cerr << "Failed to open file: " << tokens.at(writeIndex+1) << std::endl;
                                                                return false;
                                                        } else {
                                                                char* buf[4096];
                                                                int numRead = read(pipesArray[i][0],buf,4096);
                                                                write(output_fd,buf,numRead);

                                                        }
                                                        close(pipesArray[i-1][0]);
                                                        close(pipesArray[i][1]);
                                                        close(pipesArray[i-1][1]);
                                                        close(pipesArray[i][0]);
                                                }

                                        } else {
                                                //middle pipe, just execute and pipe it
                                                if(!(forkLinux(command,pipesArray[i-1][0],pipesArray[i][1]))){
                                                        close(pipesArray[i-1][0]);
                                                        close(pipesArray[i][1]);
                                                        return false;
                                                };
                                                close(pipesArray[i-1][0]);
                                                close(pipesArray[i][1]);
                                        }
                                }
                                i++;
                        }
                        for(int i =0;i<size;i++){
                                close(pipesArray[i][0]);
                                close(pipesArray[i][1]);
                        }
                }

        	//if there was a pipe, redirection is handled in the pipe section
                if(!pipeIndex){
                        if(stdErrorIndex){
                                needsExecuted=false;
                                bool exists = std::experimental::filesystem::exists(currentWorkingDirectory+"/"+tokens.at(stdErrorIndex+1));
                                if(!ALLOW_OVERWRITE && exists){
                                        std::cout<<"Error! ALLOW_OVERWRITE is set to false,and file " << tokens.at(stdErrorIndex+1) << " already exists!\n";
                                        return false;
                                }
                                std::vector<std::string> leftArgs(tokens.begin(),tokens.begin()+stdErrorIndex);
                                std::vector<std::string> rightArgs(tokens.begin()+stdErrorIndex+1,tokens.end());
                                int output_fd = open(rightArgs[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                if(output_fd == -1){
                                        std::cerr << "Failed to open file: " << rightArgs[0] << std::endl;
                                        return false;
                                }
                                if(forkLinux(leftArgs,STDIN_FILENO,STDOUT_FILENO, output_fd)){
                                        close(output_fd);
                                }
                                else{
                                        close(output_fd);
                                        return false;
                                }

                        }
                        if(stdErrorAppendIndex){
                                needsExecuted=false;
                                std::vector<std::string> leftArgs(tokens.begin(),tokens.begin()+stdErrorAppendIndex);
                                std::vector<std::string> rightArgs(tokens.begin()+stdErrorAppendIndex+1,tokens.end());
                                int output_fd = open(rightArgs[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                if(output_fd == -1){
                                        std::cerr << "Failed to open file: " << rightArgs[0] << std::endl;
                                        return false;
                                }
                                if(forkLinux(leftArgs,STDIN_FILENO,STDOUT_FILENO, output_fd)){
                                        close(output_fd);
                                }
                                else{
                                        close(output_fd);
                                        return false;
                                }

                        }
                        if(writeIndex){
                                needsExecuted=false;
                                bool exists = std::experimental::filesystem::exists(currentWorkingDirectory+"/"+tokens.at(writeIndex+1));
                                if(!ALLOW_OVERWRITE && exists){
                                        std::cerr<<"Error! ALLOW_OVERWRITE is set to false,and file " << tokens.at(writeIndex+1) << " already exists!\n";
                                        return false;
                                }
                                std::vector<std::string> leftArgs(tokens.begin(),tokens.begin()+writeIndex);
                                std::vector<std::string> rightArgs(tokens.begin()+writeIndex+1,tokens.end());
                                int output_fd = open(rightArgs[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                if(output_fd == -1){
                                        std::cerr << "Failed to open file: " << rightArgs[0] << std::endl;
                                        return false;
                                }
                                if(map.search(leftArgs[0])=="Internal commands"){
                                        if(!runBuiltinCommand(leftArgs,output_fd)){
                                                close(output_fd);
                                                return false;
                                        } else {
                                                close(output_fd);
                                        }
                                } else {
                                        if(forkLinux(leftArgs,STDIN_FILENO,output_fd)){
                                                close(output_fd);
                                        }
                                        else{
                                                close(output_fd);
                                                return false;
                                        }
                                }
                        }
                        if(appendIndex){
                                needsExecuted=false;
                                std::vector<std::string> leftArgs(tokens.begin(),tokens.begin()+appendIndex);
                                std::vector<std::string> rightArgs(tokens.begin()+appendIndex+1,tokens.end());
                                int output_fd = open(rightArgs[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                if(output_fd == -1){
                                        std::cerr << "Failed to open file: " << rightArgs[0] << std::endl;
                                        return false;
                                }
                                if(map.search(leftArgs[0])=="Internal commands"){
                                        if(!runBuiltinCommand(leftArgs,output_fd)){
                                                close(output_fd);
                                                return false;
                                        } else {
                                                close(output_fd);
                                        }
                                } else {
                                        if(forkLinux(leftArgs,STDIN_FILENO,output_fd)){
                                                close(output_fd);
                                        }
                                        else{
                                                close(output_fd);
                                                return false;
                                        }
                                }
                        }

                }
		if(needsExecuted){
			if(!processLine(tokens)){
                                return false;
                        }
                }




	}
	return true;
}

bool processLine(std::vector<std::string> & args){
        if(args.size()>0){
                std::string command = args[0];
                std::string result = map.search(command);
                if(result == "Keywords"){
                //do nothing
                std::cout << "Keyword cannot be a command" << std::endl;
                return false;
                }
                else if (result == "Internal commands"){
                //call the function pointer
                        return runBuiltinCommand(args);
                }
                else{
                        //call the fork
                        return forkLinux(args);
                }
        }
        return true;
}
//helper function, writes string to output_fd; auto calcutlates size for write(int fd,string string,int size)
void write_fd(int output_fd,std::string writeString){
        write(output_fd,writeString.c_str(),writeString.size());
}
bool builtin_cd(std::vector<std::string> args,int output_fd){
        bool lArgPresent = false;
        if(hasArg(args,"-l")&&args.size()<4){
                lArgPresent = true;
                if(hasArg(args,"-l") == args.size()-1){
                        //last arg, list all history
                        for(int i =0;i<directoryHistory.size();i++){
                                if(output_fd==-1){
                                        std::cout<<directoryHistory.size()-i<<":"<<directoryHistory.at(i)<<"\n"<<std::flush;
                                } else {
                                        write_fd(output_fd,directoryHistory.size()-i+":"+directoryHistory.at(i)+"\n");
                                }
                        }
                } else {
                        //spot after arg spot is n to show history of
                        try{
                                if(directoryHistory.size()>0){
                                        int lastNHistory = stoi(args[2]);
                                        if(lastNHistory>directoryHistory.size()-1) lastNHistory = directoryHistory.size()-1;
                                        if(output_fd==-1){
                                                std::cout<<"Showing last " << lastNHistory << "directories of "; // << directoryHistory.size() << " total directoryies\n";
                                                for(int i = directoryHistory.size()-1;i>directoryHistory.size()-(1+lastNHistory);i--){
                                                        std::cout<<i+1<<directoryHistory[i]<<"\n";
                                                }
                                        } else {
                                                std::string writeString = "Showing last "+std::to_string(lastNHistory)+" directories:";
                                             write_fd(output_fd,writeString);
                                             for(int i = directoryHistory.size()-1;i>directoryHistory.size()-(1+lastNHistory);i--){
                                                        write_fd(output_fd,""+(i+1)+directoryHistory[i]+"\n");
                                             }
                                        }
                                }
                        } catch(std::invalid_argument e) {
                                std::cerr<<"Invalid argument for n in: \"-l n\". Make sure your numeric value follows -l.\n";
				return false;
                        } catch (std::out_of_range e){
                                std::cerr<<"Too large n for: -l n\n";
				return false;
                        } catch (...) {
                                std::cerr<<"Something went wrong in cd\n";
                	        return false;
			}
                }
        }
        //now that -l has been checked, cd has max # of args of 2
        if(args.size()>=3 && !lArgPresent){
                std::cerr<<"cd: too many args. See \"cd -h\" or \"cd -H\" for proper usage\n";
		return false;
        } else if(hasArg(args,"-h")){
                //simple help
                printHelpPage("cdShort",output_fd);
        } else if(hasArg(args,"-H")){
                //full help
                printHelpPage("cd",output_fd);
        } else if(hasArg(args,"-c")){
                //clean history
                directoryHistory.clear();
        } else if(hasArg(args,"-s")){
                //remove duplicate history entries
		if(directoryHistory.size()>=2){
                        for(int i = directoryHistory.size()-1;i>=0;i--){
                                for(int j =i-1;j>=0;j--){
                                        if(directoryHistory[i]==directoryHistory[j]){
                                                directoryHistory.erase(directoryHistory.begin()+j);
                                        }
                                }
                        }
                }
        } else if(hasArg(args,"-{n}")){
                //swap to nth directory in history

                int nHistory = hasArg(args,"-{n}");
                //bounds check
                if(nHistory > directoryHistory.size()){
                        std::cerr<<"cd: Your desired value of n for \"cd  n\" is too large. Please try again.\n";
			return false;
                } else if(directoryHistory.size()>0){
                        if(nHistory >= 0){
                                std::experimental::filesystem::current_path(directoryHistory[directoryHistory.size()-nHistory]);
                                currentWorkingDirectory = std::experimental::filesystem::current_path();
                        }
                }
        } else if (args.size()==2 && !lArgPresent){
                std::string tempSavePath = currentWorkingDirectory;if(args[1] == ".."){
                        try {
                                std::experimental::filesystem::current_path("..");
                                currentWorkingDirectory = std::experimental::filesystem::current_path();
                                directoryHistory.push_back(currentWorkingDirectory);
                        } catch (std::experimental::filesystem::filesystem_error e){
                                std::cerr<<"cd: no such directory "<<args[1] <<"\n";
                                std::experimental::filesystem::current_path(currentWorkingDirectory);
				return false;
                        }
                } else if (args[1].find("..") == 0){
                        try {
                                std::experimental::filesystem::current_path("..");
                                currentWorkingDirectory = std::experimental::filesystem::current_path();
                                std::experimental::filesystem::current_path(currentWorkingDirectory+args[1].substr(2));
                                currentWorkingDirectory = std::experimental::filesystem::current_path();
                                directoryHistory.push_back(currentWorkingDirectory);
                        } catch (std::experimental::filesystem::filesystem_error e){
                                std::cerr<<"cd: no such directory "<<args[1] <<"\n";
                                std::experimental::filesystem::current_path(currentWorkingDirectory);
				return false;
			}
                } else {
                        try {
                                std::experimental::filesystem::current_path(currentWorkingDirectory+args[1]);
                                currentWorkingDirectory = std::experimental::filesystem::current_path();
                                directoryHistory.push_back(currentWorkingDirectory);
                        } catch (std::experimental::filesystem::filesystem_error e){
                                std::cerr<<"cd: no such directory "<<args[1] <<"\n";
                                currentWorkingDirectory = std::experimental::filesystem::current_path();
				return false;
                        }
                }
        } else if (args.size()==1){
                if(output_fd==-1){
                        std::cout<<currentWorkingDirectory<<"\n";
                } else {
                        write_fd(output_fd,currentWorkingDirectory+"\n");
                }
        }
	return true;
}
bool builtin_alias(std::vector<std::string> args, int output_fd){
        //load from permanent alias files?
        if(hasArg(args,"-H")){
                printHelpPage("alias",output_fd);
        }
        if(hasArg(args,"-h")){
                printHelpPage("aliasShort",output_fd);
        }
        //print every alias
        if(hasArg(args,"-p")||args.size()==1){
                if(output_fd==-1){
                        std::cout<<"Aliases:\n";
                        for(auto& it:aliases){
                                std::cout<<it.first<<"="<<it.second<<"\n";
                        }
                        std::cout<<"\n";
                } else {
                        write_fd(output_fd,"Aliases:\n");
                        for(auto& it:aliases){
                                std::string writeString = it.first+"="+it.second+"\n";
                                write_fd(output_fd,writeString.c_str());
                        }
                        write_fd(output_fd,"\n");
                }
        }
        //loop over every arg except for the first
        for(int i=1;i<args.size();i++){
                //dont want to alias args
                if(args.at(i).find("-")==0){
                        continue;
                }
                int equalPos = args.at(i).find("=");
                //no equal sign OR the equal sign is in a point that prevents creating a key/value pair (very start or end of arg): look up the arg for an alias
                if(equalPos==-1 || equalPos==0 || equalPos==args.at(i).length()-1){
                        //see if alias is set
                        auto it = aliases.find(args.at(i));
                        if(it!=aliases.end()){
                                if(output_fd==-1){
                                        std::cout<<"alias: "<<args.at(i)<<"="<<it->second<<"\n";
                                } else {
                                        write_fd(output_fd,"alias: "+args.at(i)+"="+it->second+"\n");
                                }
                        } else {
                                if(output_fd==-1){
                                        std::cout<<"alias:" << args.at(i) << " is not aliased!\n";
                                } else {
                                        write_fd(output_fd,"alias:" +args.at(i) +" is not aliased!\n");
                                }

                        }
                        continue;
                } else if(equalPos!=0&& equalPos!=args.at(i).length()-1){
                        //there is an equal sign with potential for a key/value pair for a new alias
                        std::string key = args.at(i).substr(0,equalPos);
                        std::string value=args.at(i).substr(equalPos+1);
                        aliases.insert_or_assign(key,value);
                }
        }
	return true;
}
bool builtin_unalias(std::vector<std::string> args, int output_fd){
        if(hasArg(args,"-h")){
                printHelpPage("unaliasShort",output_fd);
        }
        if(hasArg(args,"-H")){
                printHelpPage("aliasShort",output_fd);
        }
        if(hasArg(args,"-a")){
                aliases.clear();
        } else {
                for(int i =0;i<args.size();i++){
                        auto it = aliases.find(args.at(i));
                        if(it!=aliases.end()){
                                aliases.erase(it);
                        }
                }
        }
	return true;
}

void loadHelpPages(){
        std::ifstream helpPagesFile;
        helpPagesFile.open("helpPages.txt");
        if(!helpPagesFile.is_open()){
                std::cout<<"Error opening helpPages.txt\n";
        } else {
                std::string line;
                std::string helpPageName;
                std::string helpPageString;
                bool ignoreEmptyLines = false;
                while(!helpPagesFile.eof()){
                        getline(helpPagesFile,line);
                        if(line.find("HELPPAGENAME:")!=-1){
                                helpPageName=line.substr(line.find(":")+1);
                                helpPageString="";
                                ignoreEmptyLines=false;
                        }else if(line.find("ENDHELPPAGE")!=-1){
                                helpPages.insert_or_assign(helpPageName,helpPageString);
                                ignoreEmptyLines=true;
                        }else{
                                if(ignoreEmptyLines){
                                        if(line.size()!=0){
                                                helpPageString = helpPageString + line +"\n";
                                        }
                                } else {
                                        helpPageString = helpPageString + line +"\n";
                                }
                        }
                        if(line.find("HELPPAGETERMINATE")!=-1){
                                break;
                        }
                }
        }
        helpPagesFile.close();
}

void printHelpPage(std::string page,int output_fd){
        auto it = helpPages.find(page);
        if(output_fd==-1){
                if(it!=helpPages.end()){
                        std::cout<<it->second<<"\n"<<std::flush;
                } else {
                std::cout<< "Error printing helpPage: requested page doesn't exist!\n";
                }
        } else {
                if(it!=helpPages.end()){
                        write_fd(output_fd,it->second+"\n");

                } else {
                        write_fd(output_fd,"Error printing helpPage: requested page doesn't exist!\n");
                }
        }
}

bool builtin_help(std::vector<std::string> args,int output_fd){
        if(args.size()==1){
                if(output_fd==-1){
                        std::cout<<"Commands to call help on: cd, alias, unalias, history, set, help\n";
                } else {
                        write_fd(output_fd,"Commands to call help on: cd, alias, unalias, history, set, help\n");
                }
        } else if(hasArg(args,"-s")){
                for(int i =1;i<args.size();i++){
                        //exclude command line options
                        if(args.at(i).find("-")!=0){
                                printHelpPage(args.at(i)+"Short",output_fd);
                        }
                }
        } else {
                for(int i =1;i<args.size();i++){
                        //exclude command line options
                        if(args.at(i).find("-")!=0){
                                printHelpPage(args.at(i),output_fd);
                        }
                }
        }
	return true;
}
bool builtin_history(std::vector<std::string> args, int output_fd){
        if(hasArg(args,"-h")){
                printHelpPage("historyShort",output_fd);
        }
        if(hasArg(args,"-H")){
                printHelpPage("history",output_fd);
        }
        if(hasArg(args,"-c")){
                commandHistory.clear();
        }
        int historySize = commandHistory.size();
        for(int i =0;i<historySize;i++){
                if(output_fd==-1){
                        std::cout<<historySize-i<<" " <<commandHistory.at(i)<<"\n";
                } else {
                        write_fd(output_fd,""+std::to_string((historySize-i))+" "+commandHistory.at(i)+"\n");
                }
        }
	return true;
}
bool builtin_set(std::vector<std::string> args,int output_fd){
        bool unsetMode = false;
        if(hasArg(args,"-h")){
                printHelpPage("setShort",output_fd);
        }
        if(hasArg(args,"-H")){
                printHelpPage("set",output_fd);
        }
        if(hasArg(args,"-e")&&!INTERACTIVEMODE){
                STOP_ON_ERROR = !STOP_ON_ERROR;
        }
        if(hasArg(args,"-f")){
                WILDCARD_GLOBBING = !WILDCARD_GLOBBING;
        }
        if (hasArg(args,"-u")){
                unsetMode = true;
        }
        if (hasArg(args,"-uA")){
                environmentVariables.clear();
        }
        if (hasArg(args,"-C")){
                ALLOW_OVERWRITE = !ALLOW_OVERWRITE;
        }
        if(hasArg(args,"-z")){
                if(output_fd==-1){
                        std::cout<<"STOP_ON_ERROR: "<<STOP_ON_ERROR <<"\nWILDCARD_GLOBBING: "<<WILDCARD_GLOBBING<<"\nALLOW_OVERWRITE: "<<ALLOW_OVERWRITE<<"\n";
                } else {
                        write_fd(output_fd,"STOP_ON_ERROR: "+std::to_string(STOP_ON_ERROR) +"\nWILDCARD_GLOBBING: "+std::to_string(WILDCARD_GLOBBING)+"\nALLOW_OVERWRITE: "+std::to_string(ALLOW_OVERWRITE)+"\n");
                }
        }
        if(args.size()==1){
                if(output_fd==-1){
                        for(auto& it:environmentVariables){
                                std::cout<<it.first<<"="<<it.second<<"\n";
                        }
                } else {
                        for(auto& it:environmentVariables){
                                write_fd(output_fd,it.first+"="+it.second+"\n");
                        }
                }
        } else {
                for(int i = 1;i<args.size();i++){
                        int dollarIndex = args.at(i).find("$");
                        int equalIndex = args.at(i).find("=");
                        if(dollarIndex==0 && equalIndex>2){
                                std::string arg = args.at(i);
                                //inserts into environment variables left of equal sign as key, right of equal sign as value
                                if(unsetMode){
                                        auto it = environmentVariables.find(arg.substr(0,equalIndex));
                                        if(it!=environmentVariables.end()){
                                                environmentVariables.erase(arg.substr(0,equalIndex));
                                        }
                                } else {
                                        environmentVariables.insert_or_assign(arg.substr(0,equalIndex),arg.substr(equalIndex+1));
                                }
                        }
                }
        }
	return true;
}
