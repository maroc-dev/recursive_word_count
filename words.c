#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define BUFSIZE 8

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
                    printf(" %s ", word);
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
        // use_line(arg, buf + wordstart);
        printf(" %s ", buf + wordstart);
    }
    free(buf);
}

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

    return EXIT_SUCCESS;
}
