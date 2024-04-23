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
#include "builtin_functions.h"

extern void builtinSourceBatchMode(std::vector<std::string> args);
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
        } else if(command == "source"){
                return builtin_source(args,output_fd);  
        } else if (command == "shift"){
                return builtin_shift(args,output_fd);    
        }else{
                std::cout << "Internal command not implemented" << std::endl;
                return false;
        }
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
                                                        write_fd(output_fd,""+std::to_string(i+1)+directoryHistory[i]+"\n");
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
bool builtin_source(std::vector<std::string> args,int output_fd){
    if(args.size()>=2){
        std::ifstream file;
        file.open(args.at(1));
        if(file.is_open()){
            builtinSourceBatchMode(args);
        } else {
            std::cerr<<"source: no file " << args.at(1) << " found!\n";
        }
        return true;
    } else {
        std::cerr<<"source: not enough arguments!\n";
        return false;
    }
    return true;
}
bool builtin_shift(std::vector<std::string> args, int output_fd){
    std::cout<<"in shift\n";
    int shiftcount = 0;
    //int shiftCount = stoi(args.at(1));
    int paramCount = 0;
    bool findingParams = true;
    std::cout<<"Finding positional parameters\n";
    while(findingParams){
        if(environmentVariables.find("$"+std::to_string(paramCount))!=environmentVariables.end()){
            std::cout<<"paramCount: " + std::to_string(paramCount);
            paramCount++;
        } else {
            findingParams=false;
        }
    }   
    std::cout<<"Total postitional parameters: " <<std::to_string(paramCount)<<"\n";
    std::cout<<"Shifting postional parameters\n";
    for(int i=0;i<paramCount;i++){
        std::cout <<"shifting positional parameter " << std::to_string(i+paramCount) << " to $"<<std::to_string(i)<<"\n"; 
        environmentVariables.insert_or_assign("$"+std::to_string(i),environmentVariables.find("$"+std::to_string(i+paramCount))->second);
    }
    std::cout<< "SHIFT NOT IMPLEMENTED YET\n";
    return true;
}