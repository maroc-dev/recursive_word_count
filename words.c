#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define foreach(X) for (; (X) != NULL; (X) = (X)->next)

#define BUFSIZE 8 


typedef struct word {
    int frequency;
    char* text;
    struct word *next;
} word;


word *dictionary = NULL;

void print_dic(){

    word *current = dictionary;
    
    foreach(current){
        printf("%s %d\n", current->text, current->frequency);   
    }
}

char *str_copy(const char *str) {
    size_t len = strlen(str) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}


void count_word(char *str_word){
    if (dictionary == NULL){

        dictionary = malloc(sizeof(word));

        if (dictionary == NULL){
            //something went wrong
            fprintf(stderr, "Word_dic failed to malloc\n");

            exit(EXIT_FAILURE);
        }

        dictionary->frequency = 1;
        dictionary->text = str_copy(str_word);

        return;
    }
    word *current = dictionary;

    // search for the word, if found then increase the frequency by 1. If it reaches the end then it adds the word.
    foreach(current){
        if (strcmp(str_word, current->text) == 0){
            current->frequency +=1;
            return;
        }
        else if(current->next == NULL){

            current->next = malloc(sizeof(word));

            if (current->next == NULL){
                fprintf(stderr, "Malloc failed when attempting to add a new word\n");

                exit(EXIT_FAILURE);
            }
            // printf("%s\n", str_word);

            current->next->frequency = 1;
            current->next->text = str_copy(str_word);
            return;
        }
    }
}

void get_words(int fd)
{
    int bytes, wordstart, pos;
    int bufsize = BUFSIZE;
    char *buf = malloc(BUFSIZE);
    pos = 0;
    wordstart = 0;
    while ((bytes = read(fd, buf + pos, bufsize - pos)) > 0)
    {
        // printf("read %d bytes\n", bytes);
        int bufend = pos + bytes;
        for (; pos < bufend; pos++)
        {
            // printf("%d/%d/%d: '%c'\n", wordstart, pos, bufend,
            //        buf[pos]);

            if (!isalpha(buf[pos]) && buf[pos] != '\'' && buf[pos] != '-')
            {
                // printf("line in bytes %d to %d\n", wordstart, pos);
                buf[pos] = '\0';

                // handle cases where the word contains a leading hyphen
                char *word = buf + wordstart;
                if (word[0] == '-' && isalpha(word[1]))
                {
                    word++;
                }

                // handle cases where the word contains a trailing hyphen
                if (pos > wordstart && buf[pos - 1] == '-')
                {
                    buf[pos - 1] = '\0';
                }

                // if the word is not just a hyphen then process it
                if (wordstart < pos && !(pos == wordstart + 1 && buf[wordstart] == '-'))
                {
                    // printf(" %s ", word);
                    count_word(word);
                }

                wordstart = pos + 1;
            }
        }
        if (wordstart == pos)
        {
            // read a complete line
            pos = 0;
        }
        else if (wordstart > 0)
        {
            // move partial line at end of buffer to start of buffer
            int seglength = pos - wordstart;
            memmove(buf, buf + wordstart, seglength);
            pos = seglength;
        }
        else if (pos == bufsize)
        {
            // line is larger than buffer
            bufsize *= 2;
            buf = realloc(buf, bufsize);
        }
        wordstart = 0;
    }

    if (wordstart < pos)
    {
        // incomplete line at end of file/before error
        if (pos == bufsize)
        {
            buf = realloc(buf, bufsize + 1);
        }
        buf[pos] = '\0';
        // printf(" %s ", buf + wordstart);
        count_word(buf+wordstart);
    }
    free(buf);
}

// TODO : Only read .txt if going through a directory. Do not read anything that begins with a "." 
void dir_traverse(char *file)
{
    struct stat path_stat;

    if (stat(file, &path_stat) != 0)
    {
        perror(file);
        return;
    }

    if (S_ISDIR(path_stat.st_mode))
    {
        // printf("\n%s is a directory.\n", file);

        DIR *dir = opendir(file);

        if (dir == NULL)
        {
            perror("opendir failed");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;

        // Read entries from the directory
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char *path = malloc(strlen(file) + strlen(entry->d_name) + 2);
            sprintf(path, "%s/%s", file, entry->d_name);
            if (path == NULL)
            {
                perror("malloc failed");
                closedir(dir);
                exit(EXIT_FAILURE);
            }

            dir_traverse(path);

            free(path);
            // printf("Path: %s\n", file);
        }

        closedir(dir);
    }
    int fd = open(file, O_RDONLY);

    if (fd < 0)
    {
        perror(file);
        exit(EXIT_FAILURE);
    }

    get_words(fd);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++)
        dir_traverse(argv[i]);

    print_dic();

    return EXIT_SUCCESS;
}
