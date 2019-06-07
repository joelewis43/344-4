Operating Systems 1 - Program 4

Encrypt and Decrypt a text based file containing capitals and the space character
using mod 27 encryption.


otp_enc_d.c
 - A daemon process used to encrypt a given file with a given key
 - Execution
   - ./program <port number> &
 - This program waits on the given port for a connection
 - When the conenction is received, a child is forked and uses the socket returned from accept() to communicate with the client.
 - Returns the client the encrypted text based on the provided plain text and key


otp_enc.c
 - Used to communicate with the encryption daemon
 - Execution
   - ./program <plaintext file> <key file> <port number>
 - Opens each file and ensures the key is at least the same length as the plain text
 - Also ensures that no bad characters are contained in the files (non capitals/space)
 - Prints the encrypted text returned from the daemon to STDOUT


otp_dec_d.c
 - If this description sounds familliar, thats because this code is the exact same as the encryption daemon. Differences include
   - mod27 function decrpyts the text
   - functions are renamed to reflect decryption
   - authentication constants are renamed to reflect decryption
 - A daemon process used to decrypt a given file with a given key
 - Execution
   - ./program <port number> &
 - This program waits on the given port for a connection
 - When the conenction is received, a child is forked and uses the socket returned from accept() to communicate with the client.
 - Returns the client the decrypted text based on the provided plain text and key


otp_dec.c
 - If this description sounds familliar, thats because this code is the exact same as the encryption program. Differences incude
   - functions are renamed to reflect decryption
   - authentication constants are renamed to reflect decryption
 - Used to communicate with the decryption daemon
 - Execution
   - ./program <encrpyted plaintext file> <key file> <port number>
 - Opens each file and ensures the key is at least the same length as the plain text
 - Also ensures that no bad characters are contained in the files (non capitals/space)
 - Prints the decrypted text returned from the daemon to STDOUT
