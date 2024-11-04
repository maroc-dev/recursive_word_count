#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#define BUFSIZE 8 
#define INITIAL_CAPACITY 10

typedef struct {
    int frequency;
    char *text;
} Word;

typedef struct {
    Word *words;
    size_t size;
    size_t capacity;
} WordArrayList;

WordArrayList dictionary;

void init_dictionary() {
    dictionary.size = 0;
    dictionary.capacity = INITIAL_CAPACITY;
    dictionary.words = malloc(INITIAL_CAPACITY * sizeof(Word));
    if (dictionary.words == NULL) {
        char *message = "Failed to allocate memory for dictionary.\n";
        write(2, message, strlen(message));
        exit(EXIT_FAILURE);
    }
}

void free_dictionary() {
    for (size_t i = 0; i < dictionary.size; i++) {
        free(dictionary.words[i].text);
    }
    free(dictionary.words);
}

void print_dictionary() {
    for (size_t i = 0; i < dictionary.size; i++) {
        
        // if this ever overflows then god help you with the amount of words you are counting.
        char freq_str[20];

        write(1, dictionary.words[i].text, strlen(dictionary.words[i].text));
        sprintf(freq_str, " %d\n", dictionary.words[i].frequency); //converts int to str
        write(1, freq_str, strlen(freq_str));
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

void count_word(char *str_word) {
    // Search for the word in the dictionary
    for (size_t i = 0; i < dictionary.size; i++) {
        if (strcmp(dictionary.words[i].text, str_word) == 0) {
            dictionary.words[i].frequency++;
            return;
        }
    }

    // If the word is not found, add it to the dictionary
    if (dictionary.size == dictionary.capacity) {
        dictionary.capacity *= 2;
        dictionary.words = realloc(dictionary.words, dictionary.capacity * sizeof(Word));
        if (dictionary.words == NULL) {
            char* message = "Failed to reallocate memory for dictionary.\n";
            write(2, message, strlen(message));
            exit(EXIT_FAILURE);
        }
    }

    dictionary.words[dictionary.size].text = str_copy(str_word);
    dictionary.words[dictionary.size].frequency = 1;
    dictionary.size++;
}

void get_words(int fd) {
    int bytes, wordstart, pos;
    int bufsize = BUFSIZE;
    char *buf = malloc(BUFSIZE);
    pos = 0;
    wordstart = 0;
    while ((bytes = read(fd, buf + pos, bufsize - pos)) > 0) {
        int bufend = pos + bytes;
        for (; pos < bufend; pos++) {
            if (!isalpha(buf[pos]) && buf[pos] != '\'' && buf[pos] != '-') {
                buf[pos] = '\0';
                char *word = buf + wordstart;

                if (word[0] == '-' && isalpha(word[1])) {
                    word++;
                }

                if (pos > wordstart && buf[pos - 1] == '-') {
                    buf[pos - 1] = '\0';
                }

                if (wordstart < pos && !(pos == wordstart + 1 && buf[wordstart] == '-')) {
                    count_word(word);
                }

                wordstart = pos + 1;
            }
        }
        if (wordstart == pos) {
            pos = 0;
        } else if (wordstart > 0) {
            int seglength = pos - wordstart;
            memmove(buf, buf + wordstart, seglength);
            pos = seglength;
        } else if (pos == bufsize) {
            bufsize *= 2;
            buf = realloc(buf, bufsize);
        }
        wordstart = 0;
    }

    if (wordstart < pos) {
        if (pos == bufsize) {
            buf = realloc(buf, bufsize + 1);
        }
        buf[pos] = '\0';
        count_word(buf + wordstart);
    }
    free(buf);
}

char* create_path(const char* dir, const char* filename){
    // Calculating how much space we need for the complete path, the +2 is because of the '/' and the '\0'
    size_t path_length = strlen(dir) + strlen(filename) + 2;

    char* path = malloc(path_length);
    if (path == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    
    // Creating the path
    strcpy(path, dir);         
    strcat(path, "/");         
    strcat(path, filename); 

    return path;

}

void dir_traverse(char *file) {
    struct stat path_stat;

    if (stat(file, &path_stat) != 0) {
        const char *error_message = strerror(errno);

        // Write the error message corresponding to errno if stat has failed, we have to do this because only write() is permitted
        write(2, error_message, strlen(error_message));
        write(2, "\n", 1); 
        return;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        DIR *dir = opendir(file);

        if (dir == NULL) {
            perror("opendir failed");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char *path = malloc(strlen(file) + strlen(entry->d_name) + 2);
            sprintf(path, "%s/%s", file, entry->d_name);
            if (path == NULL) {
                perror("malloc failed");
                closedir(dir);
                exit(EXIT_FAILURE);
            }

            dir_traverse(path);

            free(path);
        }

        closedir(dir);
    }

    int fd = open(file, O_RDONLY);

    if (fd < 0) {
        perror(file);
        exit(EXIT_FAILURE);
    }

    get_words(fd);
    close(fd);
}

int compare_words_lexicographically(Word *a, Word *b){
    return strcmp(a->text, b->text);
}

int compare_words(const void *a, const void *b){
    Word *word_a = (Word*) a;
    Word *word_b = (Word*) b;

    // compares by frequency first, if they are equal then compares the words lexicographically
    if (word_a->frequency == word_b->frequency){
        return compare_words_lexicographically(word_a, word_b);
    }

    return word_b->frequency-word_a->frequency;

}


// TODO: Fix it so errors all use write(), make a function to do this. 
int main(int argc, char **argv) {
    if (argc < 2) {
        char* message = "Usage: ./words <path>\n";
        write(2, message, strlen(message));
        return EXIT_FAILURE;
    }

    init_dictionary();

    for (int  i = 1; i < argc; i++) {
        dir_traverse(argv[i]);
    }


    qsort(dictionary.words, dictionary.size, sizeof(Word), compare_words);

    print_dictionary();

    free_dictionary();

    return EXIT_SUCCESS;
}
