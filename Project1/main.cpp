#include<stdio.h>
#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<vector>

const int MAX_CHARACTERS = 1024;
const int MAX_ARGS = 20;

void CorrectInput(char input[], char correctedInput[]){
    int inputPosition = 0;
    int newInputPosition = 0;

    while(input[inputPosition] != '\0'){
        if(input[inputPosition] == '<' || input[inputPosition] == '>' || input[inputPosition] == '|'){
            //If we don't have a space before one of the redirection/pipe symbols, add a space
            if(input[inputPosition - 1] != ' '){
                correctedInput[newInputPosition] = ' ';
                newInputPosition++;
            }
            correctedInput[newInputPosition] = input[inputPosition];
            inputPosition++;
            newInputPosition++;
            //If we don't have a space directly after the redirection/pipe symbol, add a space
            if(input[inputPosition] != ' '){
                correctedInput[newInputPosition] = ' ';
                newInputPosition++;
            }
        }
        else{
            correctedInput[newInputPosition] = input[inputPosition];
            newInputPosition++;
            inputPosition++;
        }
    }
    //Handle the '\0' character at the end of input and put into newInput
    correctedInput[newInputPosition] = input[inputPosition];
}

void ParseInput(char input[], char* inputArguments[], int& readRedirection, int& writeRedirection, int& pipeCall){
    int i = 0;
    int argPosition = 0;
    int argPositionStart = 0;

    while(input[i] != '\0'){
        if(input[i] == '>'){
            writeRedirection = argPosition;
            input[i] = '\0';
        }
        else if(input[i] == '<'){
            readRedirection = argPosition;
            input[i] = '\0';
        }
        else if(input[i] == '|'){
            pipeCall = argPosition;
        }
        else if(input[i] == ' '){
            input[i] = '\0';
            inputArguments[argPosition] = input + argPositionStart;

            argPosition++;
            argPositionStart = i + 1;
        }
        i++;
    }
    //Handle final argument that ends with the original '\0'
    inputArguments[argPosition] = input + argPositionStart;
    argPosition++;
}

void CreateReadRedirection(char* inputArguments[], char* readRedirectionValue){
    int readFD;
    pid_t pid;

    switch(pid = fork()){
        case -1:
            std::cerr << "Could not fork. Exiting..." << std::endl;
            exit(2);
        case 0:
            if( (readFD = open( readRedirectionValue, O_RDONLY ) ) < 0 ){
                std::cerr << "could not open file " << readRedirectionValue << ". Exiting..." << std::endl;
                exit(2);
            }
            //Close the std::in and copy readFD into what used to point to std::in, then close
            //readFD as it is no longer needed after the dup2
            if(dup2(readFD, STDIN_FILENO) == -1 || close(readFD) == -1){
                std::cerr << "couldn't dup readFD, couldn't close std::in, or couldn't close readFD. Exiting..." << std::endl;
                exit(2);
            }

            if(execvp(inputArguments[0], inputArguments) == -1){
                std::cerr << "could not execute program " << inputArguments[0] << ". Exiting..." << std::endl;
                exit(2);
            }
    }
    while(pid = wait( (int *) 0 ) != -1)
        ;
}

void CreateWriteRedirection(char* inputArguments[], char* writeRedirectionValue){
    int writeFD;
    pid_t pid;
    switch(pid = fork()){
        case -1:
            std::cerr << "Could not fork. Exiting..." << std::endl;
            exit(2);
        case 0:
            if( (writeFD = open( writeRedirectionValue, O_WRONLY | O_CREAT ) ) < 0 ){
                std::cerr << "could not open file " << writeRedirectionValue << ". Exiting..." << std::endl;
                exit(3);
            }
            //Close the std::out and copy writeFD into what used to point to std::out, then close
            //writedFD as it is no longer needed after the dup2
            if(dup2(writeFD, STDOUT_FILENO) == -1 || close(writeFD) == -1){
                std::cerr << "couldn't dup writeFD, couldn't close std::out, or couldn't close writeFD. Exiting..." << std::endl;
                exit(3);
            }

            if(execvp(inputArguments[0], inputArguments) == -1){
                std::cerr << "could not execute program " << inputArguments[0] << ". Exiting..." << std::endl;
                exit(3);
            }
    }
    while(pid = wait( (int *) 0 ) != -1)
        ;
}

void CreateReadAndWriteRedirection(char* inputArguments[], char* readRedirectionValue, char* writeRedirectionValue){
    int readFD;
    int writeFD;
    pid_t pid;

    switch(pid = fork()){
        case -1:
            std::cerr << "Could not fork. Exiting..." << std::endl;
            exit(4);
        case 0:
            if( (readFD = open( readRedirectionValue, O_RDONLY ) ) < 0 ){
                std::cerr << "could not open file 1: " << readRedirectionValue << ". Exiting..." << std::endl;
                exit(4);
            }
            //Close the std::in and copy readFD into what used to point to std::in, then close
            //readFD as it is no longer needed after the dup2
            if(dup2(readFD, STDIN_FILENO) == -1 || close(readFD) == -1){
                std::cerr << "couldn't dup readFD, couldn't close std::in, or couldn't close readFD. Exiting..." << std::endl;
                exit(4);
            }

            if( (writeFD = open(writeRedirectionValue, O_WRONLY | O_CREAT, 0777) ) < 0){
                std::cerr << "could not open file 2: " << writeRedirectionValue << ". Exiting..." << std::endl;
                exit(4);
            }
            //Close the std::out and copy writeFD into what used to point to std::out, then close
            //writeFD, as it is no longer needed after the dup2
            if(dup2(writeFD, STDOUT_FILENO) == -1 || close(writeFD) == -1){
                std::cerr << "couldn't dup writeFD, couldn't close std::in, or couldn't close writeFD. Exiting..." << std::endl;
                exit(4);
            }

            if(execvp(inputArguments[0], inputArguments) == -1){
                std::cerr << "could not execute program " << inputArguments[0] << ". Exiting..." << std::endl;
                exit(4);
            }
    }
    while(pid = wait( (int *) 0 ) != -1)
        ;
}

void CreatePipe(char* readArguments[], char* writeArguments[], int pfd[]){
    pid_t pid;

    if(pipe(pfd) == -1){
        std::cerr << "Could not create a pipe. Exiting..." << std::endl;
        exit(5);
    }
    //Perform fork for the write process of the pipe
    switch(pid = fork()){
        case -1:
            std::cerr << "fork for write end of pipe failed. Exiting..." << std::endl;
            exit(5);
        case 0:
            if(dup2(pfd[1], STDOUT_FILENO) == -1){
                std::cerr << "couldn't close stdout or dup pfd[1]. Exiting..." << std::endl;
                exit(5);
            }
            if(close(pfd[0]) == -1 || close(pfd[1]) == -1){
                std::cerr << "couldn't close either pfd read or pfd write 1. Exiting..." << std::endl;
                exit(5);
            }
            //child process one takes the read arguments, since the read arguments of the original
            //command become the write arguments that is being read from the pipe
            if(execvp(readArguments[0], readArguments) == -1){
                std::cerr << "child could not execute program " << readArguments[0] << ". Exiting..." << std::endl;
                exit(5);
            }
    }
    //Perform fork for the read process of the pipe
    switch(pid = fork()){
        case -1:
            std::cerr << "fork for read end of pipe failed. Exiting..." << std::endl;
            exit(5);
        case 0:
            if(dup2(pfd[0], STDIN_FILENO) == -1){
                std::cerr << "couldn't close stdout or dup pfd[1]. Exiting..." << std::endl;
                exit(5);
            }
            if(close(pfd[0]) == -1 || close(pfd[1]) == -1){
                std::cerr << "couldn't close either pfd read or pfd write 2. Exiting..." << std::endl;
                exit(5);
            }
            if(execvp(writeArguments[0], writeArguments) == -1){
                std::cerr << "child could not execute program " << writeArguments[0] << ". Exiting..." << std::endl;
                exit(5);
            }
    }
    if(close(pfd[0]) == -1 || close(pfd[1]) == -1){
        std::cerr << "parent couldn't close either pfd or pfd write. Exiting..." << std::endl;
        exit(5);
    }
    while(pid = wait( (int *) 0 ) != -1)
        ;
}

int main(int argc, char* argv[]){
    char input[MAX_CHARACTERS];
    char correctedInput[MAX_CHARACTERS];
    char* inputArguments[MAX_ARGS];    //collection of all individual arguments
    std::string exitFlag = "\0";
    int childStatus;
    pid_t pid;
    int pfd[2];
    int readRedirection = -1;
    int writeRedirection = -1;
    int pipeCall = -1;

    //set all values within inputArguments to null in case they are initialized to something else
    for(int i = 0; i < MAX_ARGS; i++){
        inputArguments[i] = NULL;
    }

    std::cout << "% ";

    std::cin.getline(input, MAX_CHARACTERS, '\n');

    while(input != exitFlag){
        //Update input to include spaces if there are any non-space characters between pipe or redirection
        //symbols, and then parse the input to store all input arguments from input into inputArguments
        CorrectInput(input, correctedInput);

        ParseInput(correctedInput, inputArguments, readRedirection, writeRedirection, pipeCall);

        //If there was a read redirection found within input, set up the necessary piping for a read redirection
        if(readRedirection != -1 && writeRedirection == -1){
            char* readArguments[MAX_ARGS];
            int i = 0;
            while(i < readRedirection){
                readArguments[i] = inputArguments[i];
                i++;
            }
            readArguments[i] = NULL;
            CreateReadRedirection(readArguments, inputArguments[readRedirection + 1]);
            readRedirection = -1;
        }
        //If there was a write redirection found within input, set up the necessary piping for a write redirection
        else if(writeRedirection != -1 && readRedirection == -1){
            char* writeArguments[MAX_ARGS];
            int i = 0;
            while(i < writeRedirection){
                writeArguments[i] = inputArguments[i];
                i++;
            }
            writeArguments[i] = NULL;
            CreateWriteRedirection(writeArguments, inputArguments[writeRedirection + 1]);
            writeRedirection = -1;
        }
        //If there was both a read and a write redirection found within input, then set up necessary redirections
        else if(readRedirection != -1 && writeRedirection != -1){
            char* readWriteArguments[MAX_ARGS];
            char* readRedirectionValue = new char;
            char* writeRedirectionValue = new char;
            int position = 0;
            int i = 0;
            int length = std::max(readRedirection, writeRedirection);
            while(i < length){
                if(i != readRedirection && i != readRedirection + 1 && i != writeRedirection && i != writeRedirection + 1){
                    readWriteArguments[position] = inputArguments[i];
                    position++;
                }
                i++;
            }

            readWriteArguments[position] = NULL;
            readRedirectionValue = inputArguments[readRedirection + 1];
            writeRedirectionValue = inputArguments[writeRedirection + 1];

            CreateReadAndWriteRedirection(readWriteArguments, readRedirectionValue, writeRedirectionValue);
            readRedirection = -1;
            writeRedirection = -1;
        }
        //If there was a pipe symbol found within input, set up the necessary piping for a proper pipeline
        else if(pipeCall != -1){
            char* readArguments[MAX_ARGS];
            char* writeArguments[MAX_ARGS];
            int i = 0;
            int readPosition = 0;
            int writePosition = 0;
            while(inputArguments[i] != NULL){
                if(i < pipeCall){
                    readArguments[readPosition] = inputArguments[i];
                    readPosition++;
                }
                else if(i > pipeCall){
                    writeArguments[writePosition] = inputArguments[i];
                    writePosition++;
                }
                i++;
            }
            readArguments[readPosition] = NULL;
            writeArguments[writePosition] = NULL;
            CreatePipe(readArguments, writeArguments, pfd);
            pipeCall = -1;
        }
        //Otherwise, fork and process the command as normal
        else{
            switch(pid = fork()){
                case -1:
                    std::cerr << "could not fork. Exiting.." << std::endl;
                    exit(1);
                case 0:
                    if(execvp(inputArguments[0], (char**)inputArguments) == -1){
                        std::cerr << "child could not execute program " << inputArguments[0] << ". Exiting..." << std::endl;
                        exit(1);
                    }
            }
            waitpid( pid, &childStatus, WUNTRACED );

        }

        for(int i = 0; i < MAX_CHARACTERS; i++){
            input[i] = '\0';
            correctedInput[i] = '\0';
        }

        for(int i = 0; i < MAX_ARGS; i++){
            inputArguments[i] = NULL;
        }

        std::cout << "% ";

        std::cin.getline(input, MAX_CHARACTERS, '\n');
    }

    return 0;
}
