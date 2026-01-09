#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void GenerateDefaultKeyFile(const char *key_file)
{
    FILE *file = fopen(key_file, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error creating default key file: %s\n", key_file);
        exit(EXIT_FAILURE);
    }
    unsigned char key[32];
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom)
    {
        // Read 32 random bytes directly from the system
        fread(key, 1, sizeof(key), urandom);
        fclose(urandom);
    }
    else
    {
        // Handle error
        fprintf(stderr, "Failed to open /dev/urandom\n");
    }
    // Print to file in hex
    for (size_t i = 0; i < sizeof(key); ++i)
    {
        fprintf(file, "%02x", key[i]);
    }
    fclose(file);
    printf("Default key file created: %s\n", key_file);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input_file> [key_file]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *input_file = argv[1];
    const char *key_file = (argc >= 3) ? argv[2] : "default_key.txt";
    int use_default_key = (argc < 3);
    if (use_default_key)
        GenerateDefaultKeyFile(key_file);
    printf("Input file: %s\n", input_file);
    printf("Key file: %s\n", key_file);

    return 0;
}