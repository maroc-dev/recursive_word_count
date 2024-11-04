# Project II: Word occurrence count

Author: maf407 Marco Ferreira

This programs mission is to create a program that scans files/directories given and outputs a sorted lists of words by their number of occurances and then lexographically.

## Requirements
This is a list of requirements our program had to take into consideration.

* Count the frequency of words appearing in directories/files given
* Sort the output based on the frequency, then lexicographically
* Recursively traverse through directories
  * Do not read any files starting with a "." or do not end with the ".txt" extention when traversing
* Handle n number of argument files given to words for processing.
* Handle hyphes
  * -foo -> foo
  * foo- -> foo
  * foo-bar -> foo-bar
* Handle apostrophes
  * Apostrophes are counted as words and characters for simplicity
* Numbers
  * Numbers are not words so a2z -> a, z
* Error handling

My program requires all files given to be present.

## Testing Plan
The general plan was to test each individual feature with a small test case
I created a series of files to test my c application's features:

```bash
tests
├── apostrophe_handling
│   └── a.txt
├── basic_word_counting
│   └── a.txt
├── capitalization_handling
│   └── a.txt
├── files
│   ├── dir_traverse
│   │   ├── a.txt
│   │   ├── b
│   │   └── dir1
│   │       ├── a.txt
│   │       └── dir2
│   │           ├── a.txt
│   │           └── dir3
│   └── should_not_print
│       ├── a
│       ├── b
│       ├── c
│       ├── d.c
│       ├── e.json
│       └── g.txt
├── hyphens
│   ├── advanced.txt
│   └── basic.txt
├── mixed
│   └── a.txt
├── number_separation
│   └── a.txt
└── punctuation
    └── a.txt
```

Each directoy tests a specific feature.

**apostrophe_handling**
```
don't can't I'm
'hello' 'world'
it's it's it's
'''''''''' '''' '''''' ''''
```
checks if apostrophes are handles properly

**basic_word_counting**
```
hello world hello
```
Tests basic word counting and sorting.

**capitalization_handling**
```
Hello hello HELLO hElLo
```
Tests to see if words with different capitalizations count as different words, also tests to see if they are sorted correctly.

**files**
This directory contains 2 tests

*should_not_print* - tests if only files with ".txt" extentions get processed.
*dir_traverse* - Tests the traversal through sub directories.

**hyphens**
This section also contains multiple tests
*basic* - tests basic rules for hyphens
```
foo-bar          
foo--bar         
foo-             
-bar             
super-cool-word
```
*advanced* - Tests to see with a short buffer the hyphen rules still work
```
Don't-worry, be-happy!
super-duper-long-hyphenated-word
this-is-a-very-long-hyphenated-word-that-might-span-multiple-buffers
```

**mixed**
```
'''
--- 
a-'b'-c

word    word
word
    word
word		word 

test-test----test-test-test-test----test-test
'''''''-'-'-'-'-''''-'
```
A test to see if multiple requirements work together

**number_separation**
```
word1word
super23cool
abc123def
```
Simple tests to make sure that words with numbers between them are counted as different words.

**punctuation**
```
"Hello, world!"
Hello...world
word?word!word
```
A test to ensure that punctuation doesnt effect the word processing.