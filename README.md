# Encryptor
Project for CS OS course

## Description
Encryptor is a C-based utility for the Operating Systems course. It secures text files by shuffling the characters within every word using a unique permutation key for each word. The program utilizes `fork()` to process word transformations in separate child processes.

## Compilation
Compile the program with debug information enabled:

    gcc -g main.c -o encryptor

## Usage

### 1. Encryption
Run the program with a single argument to encrypt a file. This generates `output_encrypted.txt` and saves the cipher keys to `default_key.txt`.

    ./encryptor input.txt

### 2. Decryption
Run the program with the encrypted file and the key file to restore the original text. This generates `Decrypted_Message.txt`.

    ./encryptor output_encrypted.txt default_key.txt

## Output Files
* output_encrypted.txt: The scrambled text file.
* default_key.txt: Stores the permutation sequence for each word.
* Decrypted_Message.txt: The restored original text.
