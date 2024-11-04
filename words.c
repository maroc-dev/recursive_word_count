#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#define bool int
#define true 1
#define false 0

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

void print_error(char* message) {
    write(2, message, strlen(message));
    write(2, "\n", 1);
    exit(EXIT_FAILURE);
}

void init_dictionary() {
    dictionary.size = 0;
    dictionary.capacity = INITIAL_CAPACITY;
    dictionary.words = malloc(INITIAL_CAPACITY * sizeof(Word));
    if (dictionary.words == NULL) {
        print_error("Failed to allocate memory for dictionary.");
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
            free_dictionary();
            print_error("Failed to reallocate memory for dictionary.");
        }
    }

    dictionary.words[dictionary.size].text = str_copy(str_word);
    dictionary.words[dictionary.size].frequency = 1;
    dictionary.size++;
}

void get_words(int fd) {
    int bytes, wordstart, pos;
    int bufsize = BUFSIZE;
    char *buf = malloc(bufsize);
    if (buf == NULL) {
        print_error("Buffer allocation failed");
        return;
    }
    
    pos = 0;
    wordstart = 0;
    
    while ((bytes = read(fd, buf + pos, bufsize - pos)) > 0) {
        int bufend = pos + bytes;
        for (; pos < bufend; pos++) {
            // Handle hyphen cases
            if (buf[pos] == '-') {
                // Check if hyphen is between letters
                if (pos > 0 && pos < bufend - 1 && 
                    isalpha(buf[pos - 1]) && isalpha(buf[pos + 1])) {
                    // Continue processing if hyphen is in a word
                    continue;
                }
                // Invalid hyphen then treat as word boundary
                buf[pos] = '\0';
                if (wordstart < pos) {
                    count_word(buf + wordstart);
                }
                wordstart = pos + 1;
            }
            // Ending word
            else if (!isalpha(buf[pos]) && buf[pos] != '\'') {
                buf[pos] = '\0';
                if (wordstart < pos) {
                    count_word(buf + wordstart);
                }
                wordstart = pos + 1;
            }
        }
        
        // Buffer management
        if (wordstart == pos) {
            pos = 0;
        } else if (wordstart > 0) {
            int seglength = pos - wordstart;
            memmove(buf, buf + wordstart, seglength);
            pos = seglength;
        } else if (pos == bufsize) {
            // Keep the hyphen incase it is part of a word in the next buffer
            if (pos > 0 && buf[pos-1] == '-') {
                pos--;  // Keep the hyphen in the buffer
            }
            bufsize *= 2;
            char *newbuf = realloc(buf, bufsize);
            if (newbuf == NULL) {
                free(buf);
                print_error("Buffer reallocation failed");
                return;
            }
            buf = newbuf;
        }
        wordstart = 0;
    }
    
    // Handle last word if any
    if (wordstart < pos) {
        if (pos == bufsize) {
            char *newbuf = realloc(buf, bufsize + 1);
            if (newbuf == NULL) {
                free(buf);
                print_error("Buffer reallocation failed");
                return;
            }
            buf = newbuf;
        }
        buf[pos] = '\0';
        if (pos > wordstart) {
            count_word(buf + wordstart);
        }
    }
    
    free(buf);
}

char* create_path(const char* dir, const char* filename){
    // Calculating how much space we need for the complete path, the +2 is because of the '/' and the '\0'
    size_t path_length = strlen(dir) + strlen(filename) + 2;

    char* path = malloc(path_length);
    if (path == NULL) {
        print_error("malloc failed");
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
        char *error_message = strerror(errno);

        // Write the error message corresponding to errno if stat has failed, we have to do this because only write() is permitted
        print_error(error_message);
    }

    if (S_ISDIR(path_stat.st_mode)) {
        DIR *dir = opendir(file);

        if (dir == NULL) {
            print_error("opendir failed");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {

            // Skip directories starting with a dot '.', except the initial directory given
            if (entry->d_name[0] == '.') {
                continue;
            }

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char *path = create_path(file, entry->d_name);
            if (path == NULL) {
                closedir(dir);
                print_error("malloc failed");
            }

            dir_traverse(path);

            free(path);
        }

        closedir(dir);
    } else {
        // Process file if it has a ".txt" extention
        size_t len = strlen(file);
        if (len > 4 && strcmp(file + len - 4, ".txt") == 0) {
            int fd = open(file, O_RDONLY);
            if (fd < 0) {
                char *error_message = strerror(errno);
                print_error(error_message);
            }
            get_words(fd);
            close(fd);
        }
    }

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


int main(int argc, char **argv) {
    if (argc < 2) {
        print_error("Usage: ./words <path>");
    }

    init_dictionary();

    struct stat path_stat;
    for (int  i = 1; i < argc; i++) {
        if (stat(argv[i], &path_stat) != 0) {
            char *error_message = strerror(errno);

            // Write the error message corresponding to errno if stat has failed, we have to do this because only write() is permitted
            print_error(error_message);
        }

        // if what is given is a directory then process it, otherwise process it as a file
        if (S_ISDIR(path_stat.st_mode))
            dir_traverse(argv[i]);
        else {
            int fd = open(argv[i], O_RDONLY);

            if (fd < 0) {
                char *error_message = strerror(errno);
                print_error(error_message);
            }
            get_words(fd);
            close(fd);
        }

    }

    qsort(dictionary.words, dictionary.size, sizeof(Word), compare_words);

    print_dictionary();

    free_dictionary();

    return EXIT_SUCCESS;
}
