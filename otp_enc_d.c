#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <errno.h>
#include    <unistd.h>
#include    <netdb.h>

#define     MAX_CHILDREN    10

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

    //printf("Server open on %d\n", port);

}

void acceptClient(int listenSocket, int *clientSocket) {

    struct sockaddr_storage client_addr;
    int addr_size = sizeof client_addr;

    *clientSocket = -1;

    // accept the clients connection

    *clientSocket = accept(listenSocket, (struct sockaddr*)&client_addr, &addr_size);

    if (*clientSocket == -1) {
        printError("ACCEPT", 1);
    }

    //printf("\nConnecting to client\n");

}


/****************** FILE HANDLING ****************/
FILE * openFile(char *name, char *type) {
    
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

    char names[3][64] = {"encrypted", "plaintext", "key"};
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


/************** SOCKET COMMUNICATION *************/
int validate(int s) {

    char buffer[64];
    memset(buffer, '\0', sizeof buffer);

    // receive clients validation message
    recv(s, buffer, sizeof buffer, 0);

    // valid client
    if (strcmp(buffer, "otp_enc") == 0) {
        
        //printf("Valid Client\n");

        // respond to the client
        //memset(buffer, '\0', sizeof buffer);
        strcpy(buffer, "otp_enc_d");
        send(s, buffer, strlen(buffer), 0);
        
        return 1;
    }

    // invalid client
    else {
        //printf("Invalid Client\n");
        memset(buffer, '\0', sizeof buffer);
        strcpy(buffer, "Incompatable Server");
        send(s, buffer, strlen(buffer), 0);

        return 0;
    }

}

void recvData(int s, char *fileName) {

    // setup recv buffer
    char buffer[512];
    memset(buffer, '\0', sizeof buffer);

    // open file to write text into
    FILE * fp;
    fp = openFile(fileName, "w+");

    recv(s, buffer, sizeof buffer, 0);

    // while receiving text, write to file
    while (!strstr(buffer, ":")) {

        fputs(buffer, fp);
        memset(buffer, '\0', sizeof buffer);
        recv(s, buffer, sizeof buffer, 0);

    }

    // close the file
    fclose(fp);

}

void getData(int s) {

    recvData(s, "plaintext");

    recvData(s, "key"); // NOT WRITING TO FILE

}

void sendEncrpyt(int s) {

    char line[64];
    memset(line, '\0', sizeof line);
    FILE * fp = openFile("encrypted", "r+");

    while (fgets(line, sizeof line, fp)) {
        send(s, line, strlen(line), 0);
        memset(line, '\0', sizeof line);
    }

    fclose(fp);

}


/************* CHILD PROCESS FUNCTIONS ***********/
int mod27enc (int text_char, int key_char) {

    if (text_char == '\n') { return -1; }

    // check the plain text_char character
    if (text_char == 32) { // space
        text_char = 26;
    }
    else { // capital
        text_char -= 65;
    }

    // check the key_char character
    if (key_char == 32) { // space
        key_char = 26;
    }
    else { // capital
        text_char -= 65;
    }

    // calculate the cipher value (A[0] - Z[25] and space[26])
    int result = (text_char + key_char) % 27;

    if (result == 26) {
        result = 32;
    }
    else {
        result += 65;
    }

    return result;

}

void encrpyt() {

    // open files
    FILE *e_file, *k_file, *pt_file;
    e_file = openFile("encrypted", "w+");
    k_file = openFile("key", "r+");
    pt_file = openFile("plaintext", "r+");

    // ints for storage
    int result, text_char, key_char;
    
    // assumes all input is either capital letter or space
    do {
        text_char = fgetc(pt_file);
        key_char = fgetc(k_file);

        if (EOF == text_char) { break; }
        
        // calculates new value
        result = mod27enc(text_char, key_char);

        if (result < 0) {
            continue;
        }
        else {
            fputc(result, e_file);
        }

    } while (1);

    fclose(e_file);
    fclose(k_file);
    fclose(pt_file);

}

void childProcess(int s) {
         
    // validate client
    if(validate(s)) {

        getData(s);

        encrpyt();

        sendEncrpyt(s);

        removeFiles();

    }

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
            printError("FORK ERROR", 1);
            break;
        
        case 0: // child

            // perform the child process
            childProcess(clientSocket);
            
            //printf("Child closing\n");
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
        acceptClient(listenSocket, &clientSocket);

        // fork the child
        forkProcess(activeChildren, clientSocket);

    }

    // close the listener
    close(listenSocket);

    return 0;
}