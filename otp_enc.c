#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <errno.h>
#include    <unistd.h>
#include    <netdb.h>

#define     CLIENT_AUTH     "otp_enc"
#define     SERVER_AUTH     "otp_enc_d"


/**************** ERROR HANDLING *****************/
void printError(char *msg, int exitValue) {
    if (errno) {
        printf("ERROR: %s\n", strerror(errno));
    }
    else {
        printf("ERROR: %s\n", msg);
    }
    exit(exitValue);
}


/********************* SETUP *********************/
void parseCLA(char **CLA, char *textFileName, char* keyFileName, int *port) {

    // parse file names
    strcpy(textFileName, CLA[1]);
    strcpy(keyFileName, CLA[2]);

    // ensure filenames relate to existing files
    if (access(textFileName, F_OK) == -1) {
        printError("Bad key file!", 1);
    }
    else if (access(keyFileName, F_OK) == -1) {
        printError("Bad key file!", 1);
    }
    
    // parse port and check it
    *port = atoi(CLA[3]);
    if (*port <= 0) {
        printError("Bad port input!", 1);
    }
    else if (*port > 65535) {
        printError("Bad port input!", 1);
    }

}


/******************* VALIDATION ******************/
void checkCharacter(int c) {

    if (c == 32) { //s pace
        // do nothing
    }
    else if (c > 64 && c < 91) { // capital
        // do nothing
    }
    else if (c == 10) { // newline
        // ignore?
    }
    else {
        printError("Bad character", 1);
    }

}

int fileLength(char *name) {

    FILE * tmp;
    int len = 0;
    int ch;
    
    // open file
    if((tmp = fopen(name, "r+")) == NULL) {
        printError("Cant open file", 1);
    }

    // loop over file
    while (1) {
        ch = fgetc(tmp);        // get one char at a time
        if (ch == EOF) break;   // check for end of file
        checkCharacter(ch);     // check the current character
        len++;                  // increment length
    }

    // close file
    fclose(tmp);

    return len;

}

void validateFiles(char *textFileName, char* keyFileName) {

    // get the file lengths
    int textCount = fileLength(textFileName);
    int keyCount = fileLength(keyFileName);

    // make sure the plain text file is longer
    if (textCount > keyCount) {
        printError("Key too short", 1);
    }

}


/****************** SOCKET SETUP *****************/
void connectSocket(int *s, int p) {

    struct sockaddr_in serv_addr;
    struct hostent *host;

    // get host id
    if ((host = gethostbyname("os1")) == NULL)
        printError("GET HOST", 1);

    // create socket
    if((*s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printError("SOCKET", 1);

    // add server details
    serv_addr.sin_addr = *((struct in_addr *) host->h_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(p);

    // connect to server
    if (connect(*s, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printError("CONNECT", 1);

}

int validate(int s) {

    // setup buffer
    char buffer[64];
    memset(buffer, '\0', sizeof buffer);

    // send validation message
    strcpy(buffer, CLIENT_AUTH);
    send(s, buffer, strlen(buffer), 0);

    // wait for server response
    recv(s, buffer, sizeof buffer, 0);

    // server responded with error
    if (strcmp(buffer, SERVER_AUTH) != 0) {

        return 0;

    }

    return 1;

}


/****************** COMMUNICATION ****************/
void sendFile(char *fileName, int s) {

    // set up buffer
    char buffer[256];

    // wait for server to send ready message
    recv(s, buffer, sizeof buffer, 0);
    memset(buffer, '\0', sizeof buffer);

    // open file
    FILE * tmp = fopen(fileName, "r+");

    // loop over file, sending each line
    while (fgets(buffer, sizeof buffer, tmp)) {
        send(s, buffer, strlen(buffer), 0);
        memset(buffer, '\0', sizeof buffer);
    }

    // close file
    fclose(tmp);

    // send seperator
    send(s, ":", 1, 0);

}

void sendData(char *textFileName, char* keyFileName, int s) {

    // validate the server we are talking to
    if(validate(s)) {
        
        // send the plain text file
        sendFile(textFileName, s);

        // send the key file
        sendFile(keyFileName, s);

    }

    // validation fialed
    else {

        // close socket
        close(s);

        // report to STDERR
        printError("Invalid Server!", 2);
    }

}

void recvData(int s) {

    // setup recv buffer
    char buffer[256];
    memset(buffer, '\0', sizeof buffer);

    // recv encryption
    recv(s, buffer, sizeof buffer, 0);

    // output encrypted data to STDOUT
    printf("%s\n", buffer);

}





int main(int argc, char **argv) {

    char textFileName[64];
    char keyFileName[64];
    int socket;
    int port;

    // gets filenames and port number from cla and ensures they are valid
    parseCLA(argv, textFileName, keyFileName, &port);

    // tests files for length and character requirements
    validateFiles(textFileName, keyFileName);

    // connect to the host
    connectSocket(&socket, port);

    // send the text and key files to the server
    sendData(textFileName, keyFileName, socket);

    // recv the encrypted text
    recvData(socket);

    // close the socket
    close(socket);

}