#include    <stdio.h> 
#include    <stdlib.h>
#include    <errno.h>
#include    <time.h>
#include    <string.h>

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
void parseCLA(int len_cla, char **CLA, int *length) {

    if (len_cla != 2) {
        printError("Bad Usage: ./keygen <length>", 1);
    }
    
    // parse length
    *length = atoi(CLA[1]);

    // check the length
    if (*length <= 0) {
        printError("Bad length!", 1);
    }

}

void allocate(char *array, int length) {
    
    // allocate enough space for length + 1 characters
    array = malloc(sizeof(char) * (length + 1));

    // clear the array
    memset(array, '\0', sizeof(array));

}


/******************** KEY GEN ********************/
int randomChar() {

    int num = (rand() % 27);

    if (num == 26)
        num = 32;
    else
        num += 65;

    return num;

}

void getKey(char *array, int length) {

    // set rand seed
    srand(time(0));

    int i;
    for (i=0; i<length; i++) {

        // store a random char in the current array index
        array[i] = randomChar();

    }

    // add a newline to the end of the array
    array[length] = '\n';

}




int main(int argc, char **argv) {
    
    char *key;
    int length;

    // get the length from the arguments
    parseCLA(argc, argv, &length);
    
    // allocate enough space for the length + \n + \0
    key = malloc(sizeof(char) * (length + 2));
    memset(key, '\0', sizeof(key));

    // fill the key with random characters (end with \n)
    getKey(key, length);
    
    // output the key to STOUD
    printf("%s", key);

    // free the array
    free(key);
    
    return 0;
}