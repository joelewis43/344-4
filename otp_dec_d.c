#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <errno.h>
#include    <unistd.h>
#include    <netdb.h>

#define     MAX_CHILDREN    10
#define     CLIENT_AUTH     "otp_dec"
#define     SERVER_AUTH     "otp_dec_d"

/********************* SETUP *********************/
void printError(char *msg, int exitValue) {
    if (errno) {
        printf("ERROR: %s\n", strerror(errno));
    }
    else {
        printf("ERROR: %s\n", msg);
    }
    exit(exitValue);
}

void parseCLA(char **CLA, int *listenPort) {

    int tempPort = atoi(CLA[1]);

    if (tempPort <= 0) {
        printError("Bad port input!", 1);
    }
    else if (tempPort > 65535) {
        printError("Bad port input!", 1);
    }
    else {
        *listenPort = tempPort;
    }

}

void setListener(int port, int *listener) {

    int tempSock;
    struct sockaddr_in server_address;

    // set up the servers address info
    memset(&server_address, '0', sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    if ((tempSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printError("SOCKET ERROR!", 1);
    }

    if (bind(tempSock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        printError("BIND ERROR!", 1);
    }

    if (listen(tempSock, 10) < 0) {
        printError("LISTEN ERROR!", 1);
    }

    *listener = tempSock;

}

int acceptClient(int listenSocket, int *clientSocket) {

    struct sockaddr_storage client_addr;
    int addr_size = sizeof client_addr;

    *clientSocket = -1;

    // accept the clients connection
    *clientSocket = accept(listenSocket, (struct sockaddr*)&client_addr, &addr_size);

    // error accepting client
    if (*clientSocket == -1) {
        printf("ACCEPT\n");
        return 0;
    }

    // successfully accepted client
    return 1;


}


/****************** FILE HANDLING ****************/
FILE * openFile(char *name, char *type) {
    
    // this function opens a file with the processes
    // pid appended, that way files are unique amongs
    // client requests

    char *tempName = malloc(64 * sizeof(char));
    char *tempAdr = tempName;

    // clear string
    memset(tempName, '\0', sizeof tempName);

    // copy in the provided name
    strcpy(tempName, name);

    // move pointer to the end of current string
    tempName += strlen(tempName);

    // copy in the pid
    sprintf(tempName, "%d", getpid());

    // return pointer to head of string
    tempName = tempAdr;

    // open the file
    FILE * tmpFile = fopen(tempName, type);

    free(tempName);
    return tmpFile;
}

void removeFiles() {

    char names[3][64] = {"decrypted", "plaintext", "key"};
    char *fullName = malloc(64 * sizeof(char));
    char *addr = fullName;

    int i;
    for (i=0; i<3; i++) {

        // clear string
        memset(fullName, '\0', sizeof fullName);
        
        // copy in name
        strcpy(fullName, names[i]);

        // advance pointer
        fullName += strlen(fullName);

        // add pid
        sprintf(fullName, "%d", getpid());

        // return pointer
        fullName = addr;

        // remove file
        remove(fullName);

    }

    free(fullName);

}


/**************** HELPER FUNCTIONS ***************/
void removeColon(char *buffer) {

    // the colon will be the last character
    // replace it with null terminator
    buffer[strlen(buffer)-1] = '\0';


}


/************** SOCKET COMMUNICATION *************/
int validate(int s) {

    char buffer[64];
    memset(buffer, '\0', sizeof buffer);

    // wait for client to send validation message
    recv(s, buffer, sizeof buffer, 0);

    // valid client
    if (strcmp(buffer, CLIENT_AUTH) == 0) {

        // respond to the client
        strcpy(buffer, SERVER_AUTH);
        send(s, buffer, strlen(buffer), 0);
        
        return 1;
    }

    // invalid client
    else {
        
        // send validation error
        memset(buffer, '\0', sizeof buffer);
        strcpy(buffer, "Validation Error!");
        send(s, buffer, strlen(buffer), 0);

        // report to STDERR

        return 0;
    }

}

void recvData(int s, char *fileName) {

    // setup recv buffer
    char buffer[256];
    memset(buffer, '\0', sizeof buffer);

    // open file to write text into
    FILE * fp = openFile(fileName, "w+");

    // tell the client we are ready to recv
    send(s, "OK", 2, 0);

    // start receiving file
    recv(s, buffer, sizeof buffer, 0);

    // keep receiving until we hit :
    while (!strstr(buffer, ":")) {

        // write to file
        fputs(buffer, fp);

        // recv next message
        memset(buffer, '\0', sizeof buffer);
        recv(s, buffer, sizeof buffer, 0);

    }

    // there is text still in the buffer
    if (strcmp(buffer, ":") != 0) {
        removeColon(buffer);
        fputs(buffer, fp);
    }

    // close the file
    fclose(fp);

}

void sendDecrpyt(int s) {

    // setup send buffer
    char line[64];
    memset(line, '\0', sizeof line);

    // open file to read from
    FILE * fp = openFile("decrypted", "r+");

    while (fgets(line, sizeof line, fp)) {
        send(s, line, strlen(line), 0);
        memset(line, '\0', sizeof line);
    }

    // send seperator
    send(s, ":", 1, 0);

    fclose(fp);

}


/************* CHILD PROCESS FUNCTIONS ***********/
int mod27dec (int text_char, int key_char) {

    // check the plain text characted
    if (text_char == 32) { // space
        text_char = 26;
    }
    else { // capital
        text_char -= 65;
    }

    // check the key character
    if (key_char == 32) { // space
        key_char = 26;
    }
    else { // capital
        key_char -= 65;
    }

    // calculate the cipher value (A[0] - Z[25] and space[26])
    int result = (text_char - key_char) % 27;

    // check for negative result
    if (result < 0) {
        result += 27;
    }

    // convert from 0-26 back to ascii
    if (result == 26) {
        result = 32;
    }
    else {
        result += 65;
    }

    return result;

}

void decrpyt() {

    // open files
    FILE *de_file, *k_file, *pt_file;
    de_file = openFile("decrypted", "w+");
    k_file = openFile("key", "r+");
    pt_file = openFile("plaintext", "r+");

    // ints for storage
    int result, text_char, key_char;
    
    // assumes all input is either capital letter or space
    do {

        // get next character
        text_char = fgetc(pt_file);
        key_char = fgetc(k_file);

        // check for end of plain text file
        if (EOF == text_char) { break; }

        // check if newline character
        else if (text_char == 10) { continue; }
        
        // calculates new value
        result = mod27dec(text_char, key_char);

        // write to file
        fputc(result, de_file);

    } while (1);

    // close the files
    fclose(de_file);
    fclose(k_file);
    fclose(pt_file);

}

void childProcess(int s) {
         
    // validate client
    if(validate(s)) {

        // receive plain text file
        recvData(s, "plaintext");

        // receive key file
        recvData(s, "key");

        // decrypt the plain text
        decrpyt();

        // send the decrypted text to the client
        sendDecrpyt(s);

        // clean up temp files
        removeFiles();

    }

    // close the communication socket
    close(s);

}


/*************** PROCESS MANAGEMENT **************/
void removeChild(int pid, int *array) {

    int i;

    for (i=0; i<MAX_CHILDREN; i++) {
        if (array[i] == pid) {
            array[i] = 0;
        }
    }

}

void addChild(int pid, int array[MAX_CHILDREN]) {

    int i, flag = 0;

    // loop over child array looking for open slot
    for (i=0; i<MAX_CHILDREN; i++) {
        if (array[i] == 0) {
            array[i] = pid;
            break;
        }
    }

}

void waitChildren(int array[MAX_CHILDREN]) {
    
    int i, status;
    int finishedPID;
    int count = 0;

    for (i=0; i<MAX_CHILDREN; i++) {

        // try to wait on child
        finishedPID = waitpid(array[i], &status, WNOHANG);

        // nonzero means process completed
        if (finishedPID != 0) {
            array[i] = 0; //
        }

        // check if current slot is occupied
        if (array[i] != 0) {
            count++;
        }

    }

    // all slots full
    if (count == MAX_CHILDREN) {

        // block server until slot opens
        finishedPID = wait(&status);
        removeChild(finishedPID, array);

    }

}

void forkProcess(int activeChildren[MAX_CHILDREN], int clientSocket) {

    int childPID;
    int numberOfChildren;

    // does two things
    //  Checks if there are children to be removed
    //  Checks if there is space for a new child
    waitChildren(activeChildren);
    // following execution, we are certain there is at least one open slot

    childPID = fork();

    switch(childPID) {

        case -1:
            // is this in the parent bc fork failed to make a child?
            printError("FORK ERROR", 1);
            break;
        
        case 0: // child

            // perform the child process
            childProcess(clientSocket);
            
            // terminate the child
            exit(0);
            break;

        default: // parent
            
            // add child to list of children
            addChild(childPID, activeChildren);

            break;
    }


}





int main(int argc, char **argv) {

    int listenPort;
    int listenSocket;
    int clientSocket;
    int activeChildren[MAX_CHILDREN] = {0};

    // parse CLA for the port to listen to
    parseCLA(argv, &listenPort);
    
    // Listen to the port
    setListener(listenPort, &listenSocket);

    // repeat forever
    while (1) {

        // wait for a client and accept them
        if(acceptClient(listenSocket, &clientSocket)) {

            // fork the child
            forkProcess(activeChildren, clientSocket);

        }

    }

    // close the listener
    close(listenSocket);

    return 0;
}