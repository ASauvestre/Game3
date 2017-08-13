#include <stdlib.h>
#include <string.h>

#include "parsing.h"

// Right-side inclusive
String cut_until_char(char c, String * string) {
    String left;
    left.data  = string->data;
    left.count = 0;

    while(string->count > 0) {
        if((*string)[0] == c) break;
        push(string);
        left.count += 1;
    }

    return left;
}

String cut_until_space(String * string) {

    String left = cut_until_char(' ', string);

    cut_spaces(string);

    return left;

}

char * to_c_string(String string) { // @Incomplete take pointer here ?
    char * c = (char *) malloc(string.count + 1);
    memcpy(c, string.data, string.count);
    c[string.count] = 0;

    return c;
}

void push(String * string, int amount) { // @Default amount = 1
    string->data  += amount;
    string->count -= amount;
}

void cut_spaces(String * string) {
    while((*string)[0] == ' ') push(string);
}

String bump_to_next_line(String * string) {
    cut_spaces(string);

    String line;
    line.data  = string->data;
    line.count = 0;

    // Empty line / EOF check.
    {
        if((*string)[0] == '\0') {
            line.count = -1; // @Temporary, this signals that we should stop getting new lines from the buffer, we should probably find a better way to do it.
            return line; // EOF
        }

        if((*string)[0] == '\r') {
            push(string, 2); // CLRF
            return line;
        }

        if((*string)[0] == '\n') {
            push(string);
            return line;
        }
    }

    while(true) {

        line.count  += 1;
        push(string);

        // End of line/file check
        {
            if((*string)[0] == '\0') {
                break; // EOF
            }

            if((*string)[0] == '\n') {
                push(string);
                break;
            }

            if((*string)[0] == '\r') {
                push(string, 2);
                break;
            }
        }
    }

    return line;
}

char * find_char_from_right(char c, char * string) {
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

char * find_char_from_left(char c, char * string) {
    char * cursor = string;

    while(*cursor != '\0') {
        if(*cursor == c) {
            return cursor + 1;
        }
        cursor += 1; // Move to next character
    }

    return NULL;
}
