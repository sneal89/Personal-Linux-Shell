#include <iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <algorithm>
#include <string>

using namespace std;

char buff[512];
string startCommandPath = getcwd(buff, 512);
vector<int> backgroundPIDS;
bool runAsBackground = false;
string directoryN = "/home/sneal/PA4";
string directoryOld = "/home/sneal/PA4";

string trim (string input){
    int i=0;
    while (i < input.size() && input [i] == ' ')
        i++;
    if (i < input.size())
        input = input.substr (i);
    else{
        return "";
    }
    
    i = input.size() - 1;
    while (i>=0 && input[i] == ' ')
        i--;
    if (i >= 0)
        input = input.substr (0, i+1);
    else
        return "";
    
    return input;
    

}

string replaceCharacter(string replaceable, char replaceMe, char withMe)
{
    for (int i = 0; i < replaceable.length(); i++) 
    {
        if (replaceable[i] == replaceMe)
            replaceable[i] = withMe;
    }
    return replaceable;
}

vector<string> split (string line, string separator=" "){
    //handles quotes inside args as well as symbols 
    //like echo "print this" <-- shouldn't print quotes
    for(int i = 0; i < line.length(); i++)
    {

        if(line[i] == '\'' || line[i] == '\"')
        {
            int j = i;
            while((line[j] != '\'' || line[j] != '\"' ) && j < line.length()) 
            {
                if(line[j] == ' ')
                    line[j] = '?';
                if(line[j] == '|' && (line.find("echo") != string::npos))
                    line[j] = '!';
                if(line[j] == '>')
                    line[j] = '~';
                if(line[j] == '<')
                    line[j] = '%';
                j++;
            }
            i = j;
        }
    }
    vector<string> result;
    while (line.size()){
        size_t found = line.find(separator);
        if (found == string::npos){
            string lastpart = trim (line);
            if (lastpart.size()>0){
                result.push_back(lastpart);
            }
            break;
        }
        string segment = trim (line.substr(0, found));
        //cout << "line: " << line << "found: " << found << endl;
        line = line.substr (found+1);

        //cout << "[" << segment << "]"<< endl;
        if (segment.size() != 0)
        {
            result.push_back (segment);
        } 
           
        /*for(int i = 0; i < result.size(); i++)
        {
            cout << "this is the result of index i: " << result[i] << endl;
        }*/
        //cout << line << endl;
    }

    for(int j = 0; j < result.size(); j++)
    {
        //cout << result[0] << " <-- zero " << endl;
        result[j] = replaceCharacter(result[j], '?', ' ');
        result[j] = replaceCharacter(result[j], '!', '|');
        result[j] = replaceCharacter(result[j], '~', '>');
        result[j] = replaceCharacter(result[j], '%', '<');
        while(result[j].find('\"') != string::npos){//check for double-quotes
                result[j].erase(result[j].begin() + result[j].find('\"'));
                }
        if(!(result[j].find("awk") != string::npos)) //trying to not erase '' when awk command 
        {
            while(result[j].find('\'') != string::npos){//check for double-quotes
                result[j].erase(result[j].begin() + result[j].find('\''));
                }
        }
        
    }
    /*
    cout << result.size() << endl;
    for(int i = 0; i < result.size(); i++)
    {
        cout << "result num " << i << " : " << result[i] << endl;
    }
    */
    return result;
}

char** vec_to_char_array (vector<string> parts){
    char ** result = new char * [parts.size() + 1]; // add 1 for the NULL at the end
    for (int i=0; i<parts.size(); i++){
        // allocate a big enough string
        result [i] = new char [parts [i].size() + 1]; // add 1 for the NULL byte
        strcpy (result [i], parts[i].c_str());
    }
    result [parts.size()] = NULL;
    return result;
}

void execute (string command){
    vector<string> argstrings = split (command, " "); // split the command into space-separated parts
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*
    execvp (args[0], args);
    return;
} //change directory and similar things using only system calls - like theres a system call for changing directory - just for cd

string redirectOut(string part)
{
    //cout << part << " <- this is part " << endl;
    vector<string> partSplit = split (part, ">");
    //cout << "this is part 0: " << partSplit[0] << "   this is part 1: " << partSplit[1] << /* "this is part 2: " << partSplit[2] << */ endl;
    //redirect fd of partSplit 1 to partSplit2
    int fdTemp2 = open(partSplit[1].c_str(), O_CREAT | O_WRONLY | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(fdTemp2, 1);
    close(fdTemp2);
    return partSplit[0];
}


string redirectIn(string part)
{
    vector<string> partSplit = split (part, "<");
    //cout << "this is part 0: " << partSplit[0] << "   this is part 1: " << partSplit[1] << /* "this is part 2: " << partSplit[2] << */ endl;
    //redirect fd of partSplit 1 to partSplit2
    int fdTemp1 = open(partSplit[1].c_str(), O_CREAT | O_RDONLY);
    dup2(fdTemp1, 0);
    close(fdTemp1);
    return partSplit[0];
}

void cdFun(string commandL)
{
    char buff[512];
    commandL = trim(commandL);

    if (commandL == "-")
    {
        commandL = directoryOld;
        directoryOld = getcwd(buff, 512);
        const char* c = commandL.c_str();
        if (chdir(c) != 0)
            perror("chdir() failed"); 
        directoryN = getcwd(buff, 512);     
    }
    else if (commandL == "/home")
    {
        const char* c = commandL.c_str();
        if (chdir(c) != 0)
            perror("chdir() failed");
        directoryN = getcwd(buff, 512);
    }
    else
    {
        commandL = directoryN + "/" + commandL;
        const char* c = commandL.c_str();
        if (chdir(c) != 0)
            perror("chdir() failed");
        directoryN = getcwd(buff, 512);
    }
}

void jobsCom()
{
    //cout << "processes: " << endl;
    for (int i = 0; i < backgroundPIDS.size(); i++)
    {
        //print out process number and then the PID of the process
        cout << "Process #" << i + 1 << " - " << backgroundPIDS[i] << endl;
    }
}

int main (){
    //string str = "/home/sneal/PA4/foo.txt";
    //string strIO = "";
    //const char* c =str.c_str();
    //cout << c << endl;

    //int lengthIO = 0;
    //bool seperateFile = false;

    int zero = dup(0);
    int one = dup(1);
    char bufff[512];
    directoryN = getcwd(bufff, 512);




    while (true){ // repeat this loop until the user presses Ctrl + C
        

        char buf[512]; //if it breaks, resize it
        string commandline = ""; 
        string filepathing = getcwd(buf, 512);
        cout << filepathing + " >shell:: ";
        getline(cin,commandline);
        //if(commandline == "") continue;


        //get from STDIN, e.g., "ls  -la |   grep Jul  | grep . | grep .cpp" 
        //cin >> commandline;
        //old way? 
        //char commandLine[200];
        //cin.getline(commandLine, 200);
        //commandline = commandLine;
        
        //cout << commandline << endl;
        // split the command by the "|", which tells you the pipe levels

        if (commandline == "jobs")
            jobsCom();

        size_t cdCom = commandline.find("cd");
        string direct = " ";
        if(cdCom != string::npos)
        {
            direct = trim(commandline);
            direct = commandline.substr(2, commandline.length()-2);
            cdFun(direct);
        }

        int ampFound = commandline.find("&");
        if(ampFound != string::npos)
        {
            commandline.pop_back();
            runAsBackground = true;
        }

        vector<string> tparts = split (commandline, "|");

        //commandline = redirectIn(tparts);
        //commandline = redirectOut(tparts);

        for (int i = 0; i < backgroundPIDS.size(); i++)
        {
            int checkPid = waitpid(backgroundPIDS[i], 0, WNOHANG);
            if (checkPid == 0) 
            {
                cout << backgroundPIDS[i] << " is not done" << endl;
            }
            else if (checkPid > 0) 
            {
                wait(&backgroundPIDS[i]);
                backgroundPIDS.erase(backgroundPIDS.begin() + i);
            }
        }


        for(int i = 0; i < tparts.size(); i++)
        {
            //check if redirection is needed in cur section of tparts
            //cout << "tparts[i] = " << tparts[i] << endl;
            int Com1 = tparts[i].find(">");
            int Com2 = tparts[i].find("<");
            if(Com1 != string::npos && !(tparts[i].find("echo") != string::npos))
                tparts[i] = redirectOut(tparts[i]);
            if(Com2 != string::npos && !(tparts[i].find("echo") != string::npos))
                tparts[i] = redirectIn(tparts[i]);
        }


        // for each pipe, do the following: 
        for (int i=0; i<tparts.size(); i++){
            //cout << "tparts[i] is : " << tparts[i] << endl;
            int fd[2];
            //make pipe
            if(pipe(fd) < 0)
            {
                printf(" Pipe could not be created \n");
                break; 
            }

            //You have to extract redirection symbol and the target file beforehand, filter those out, and then call the execute (command).
            pid_t kiddo = fork();
			if (!kiddo){
                // redirect output to the next level
              

                // unless this is the last level
                //cout << "deeper" << endl;
                
                //cout << ( "Child PID: %d\n", getpid());
                //close (fd[1]); //delete
                //printf( "Child PID: %d\n", getpid());
                //cout << tparts.size() << " <- This is tparts.size()" << endl; 
                if(runAsBackground)
                {
                    int piddy = getpid();
                    backgroundPIDS.push_back(piddy);
                }
                

                if (i < tparts.size() - 1){ 
                    
                    //close(fd[0]);
                    dup2(fd[1], 1);
                    close(fd[1]);
                
                    //printf( "Child PID: %d\n", getpid());
                    //execlp ("ls", "ls","-l", NULL);
                    // redirect STDOUT to fd[1], so that it can write to the other side
                    //close (fd[1]);   // STDOUT already points fd[1], which can be closed
                    
                }
                //execute function that can split the command by spaces to 
                //find out all the arguments, see the definition
                //take command
                
                execute (tparts[i]); // this is where you execute
            }else{
                
                if(runAsBackground)
                {
                    runAsBackground = false;
                    backgroundPIDS.push_back(kiddo);
                }
                else
                {
                    wait(0);
                }
                //printf( "Parent PID: %d\n", getpid());
                // wait for the child process running the current level command
				//conditional, check for &, if & dont wait finish on own time, if 
                //dup2(fd[0], 0); //commenting this out allows for multiple commands per running of ./shell
                //TODO FIX THE END OF THE WHILE LOOP TO ALLOW FOR MORE COMMANDS
                /*fd[0]*/
                dup2(fd[0], 0);
                close(fd[0]);
                //close(fd[0]);
                //close(fd[1]);
                //redirect the input from the child process
                // then do other redirects
                
            }
            close(fd[1]);
        }

        dup2(zero, 0);
        dup2(one, 1);
        
    }
}



//cd .. works change cd - to cd .. they do the same thing
//dont call wait directly
//make wait conditional to if ...command does not have 
//add PID of child process to vector of PID
//waitpid function rather than wait function 
//if returns successfully reaped zombie process(es)
//awk command treats file as bunch of rows and columns, can decide to keep only certain columns (like an excel file)
//before calling split, the arguments must stay together spaces in single quotes are not split by
//dont split by spaces in single quotes


//every bar encounteres = set up a new pipe
/*
if you call pipe , fd 3 and 4 are created, looping 3 and 4

//every bar encounteres = set up a new pipe
fork after every pipe 

in parsing account for extra white spaces in commands or input 
trailing, leading or in the middle

everything will go thorugh exec exept change directory

*/

//only the command that is to be printed out to the new text file goes into execute
//

/*

if background process add to vector through pid of fork - waitpid(fork) whenever you know its a background process (inside for loop but still vague) it shouldnt matter what current command is, jk you can do it before the forloop, check if any are done, kill/execute/wait(specific pid) (reap the resources) and remove from list, then go into forloop
waitpid just checks if that pid is done
else just do what code currently does
if background process 
vector<int*> backgroundPIDS apparently can do ints instead of int*

ECHO - remove single quotes as well

AWK remove single quotes i think
*/