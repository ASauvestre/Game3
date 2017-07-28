#pragma once

static char * find_char_from_right(char c, char * string) {
    char * cursor = string;
    char * last_char_occurence = NULL;

    while(*cursor != '\0') {
        if(*cursor == c) {
            last_char_occurence = cursor + 1;
        }
        cursor += 1; // Move to next character
    }

    return last_char_occurence;
}

static char * find_char_from_left(char c, char * string) {
    char * cursor = string;

    while(*cursor != '\0') {
        if(*cursor == c) {
            return cursor + 1;
        }
        cursor += 1; // Move to next character
    }

    return NULL;
}
