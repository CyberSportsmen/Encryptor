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
        {
            fprintf(f_key, "%d", p[i]);
            if (i < len - 1)
                fprintf(f_key, " ");
        }
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

    const char *key_file = (argc >= 3) ? argv[2] : "default_key.txt";
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }
    struct stat s;
    fstat(fd, &s);
    char *map = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    int mode = (argc < 3); // 1 for encrypt, 0 for decrypt

    FILE *f_main_out = NULL;
    FILE *f_key_ptr = NULL;

    // Only open the relevant output file to avoid ACCIDENTALY deleting input
    if (mode == 1)
    {
        f_main_out = fopen("output_encrypted.txt", "w");
        f_key_ptr = fopen(key_file, "w");
    }
    else
    {
        f_main_out = fopen("Decrypted_Message.txt", "w");
        f_key_ptr = fopen(key_file, "r");
    }

    if (!f_key_ptr || !f_main_out)
    {
        perror("Error opening files");
        return EXIT_FAILURE;
    }

    char word[1024];

    int i = 0;
    int wordcount = 0;
    while (i < s.st_size)
    {
        if (!isalpha(map[i]))
        {
            fputc(map[i], f_main_out);
            i++;
            continue;
        }

        int j = 0;
        while (i < s.st_size && isalpha(map[i]))
        {
            if (j < 1023)
                word[j++] = map[i];
            i++;
        }
        word[j] = '\0';
        wordcount++;

        if (mode == 1)
            StartChildEncryptor(word, f_main_out, f_key_ptr);
        else
            StartChildDecryptor(word, wordcount, f_main_out, f_key_ptr);
    }

    close(fd);
    fclose(f_main_out);
    fclose(f_key_ptr);
    munmap(map, s.st_size);
    return 0;
}