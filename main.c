#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

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

void StartChildEncryptor(const char *word, FILE *f_out, FILE *f_key)
{
    int len = strlen(word);
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) // child process
    {
        srand(time(NULL) ^ getpid());
        int *p = malloc(len * sizeof(int));
        char *encrypted = malloc((len + 1) * sizeof(char));
        for (int i = 0; i < len; i++)
            p[i] = i;

        // shuffle (Fisher-Yates)
        for (int i = len - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            int temp = p[i];
            p[i] = p[j];
            p[j] = temp;
        }
        // write key to file
        for (int i = 0; i < len; i++)
            fprintf(f_key, "%d ", p[i]);
        fprintf(f_key, "\n");

        // encrypt word and write to file
        for (int i = 0; i < len; i++)
            encrypted[i] = word[p[i]];
        encrypted[len] = '\0';
        fprintf(f_out, "%s", encrypted);

        free(p);
        free(encrypted);
        exit(0);
    }
    else
    { // parent proccess
        wait(NULL);
    }
}

void StartChildDecryptor(const char *word, int wordcount, FILE *f_out, FILE *f_key)
{
    int len = strlen(word);
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) // Child process
    {
        int *p = malloc(len * sizeof(int));
        char *decrypted = malloc((len + 1) * sizeof(char));

        // Reset file pointer to the start
        rewind(f_key);

        // skip (wordcount - 1) lines to reach current key
        char line[1024];
        for (int k = 0; k < wordcount - 1; k++)
        {
            if (!fgets(line, sizeof(line), f_key))
                break;
        }

        // parse
        if (fgets(line, sizeof(line), f_key))
        {
            int idx = 0;
            for (char *token = strtok(line, " \n"); token != NULL; token = strtok(NULL, " \n"))
            {
                if (idx < len)
                    p[idx++] = atoi(token);
            }
        }

        // The inverse of encrypted[i] = word[p[i]] is decrypted[p[i]] = word[i]
        for (int i = 0; i < len; i++)
        {
            decrypted[p[i]] = word[i];
        }
        decrypted[len] = '\0';

        // Write to output
        fprintf(f_out, "%s", decrypted);

        free(p);
        free(decrypted);
        exit(0);
    }
    else // Parent process
    {
        wait(NULL);
    }
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
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }
    struct stat s;
    fstat(fd, &s);
    char *map = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0); // place file in ram

    int mode = (argc < 3); // 0 for decrypt mode, 1 for encrypt mode

    FILE *f_out_ptr = fopen("output_encrypted.txt", "w");
    FILE *f_key_ptr = fopen(key_file, "rw");
    char word[1024];
    // encrypt
    int i = 0;
    int wordcount = 0;
    while (i < s.st_size)
    {
        // jump peste spatii goale
        if (!isalpha(map[i]))
        {
            fputc(map[i], f_out_ptr);
            i++;
            continue;
        }
        // cream cuvantul
        int j = 0;
        while (i < s.st_size && isalpha(map[i]))
        {
            if (j < 1023)
                word[j++] = map[i];
            i++;
        }
        word[j] = '\0'; // string done
        wordcount++;
        if (mode == 1)
            StartChildEncryptor(word, f_out_ptr, f_key_ptr);
        else
            StartChildDecryptor(word, wordcount, f_out_ptr, f_key_ptr);
    }

    // printf("Input file: %s\n", input_file);
    // printf("Key file: %s\n", key_file);
    close(fd);
    fclose(f_out_ptr);
    fclose(f_key_ptr);
    munmap(map, s.st_size);
    return 0;
}