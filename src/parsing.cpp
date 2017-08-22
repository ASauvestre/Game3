#include <stdlib.h>
#include <string.h>

#include "parsing.h"
#include "macros.h"

void skip_empty_lines(String * string) {
    String line;
    String prev_string;

    while(!line.count) {
        prev_string = *string;
        line = bump_to_next_line(string);

        if(line.count < 0) return;
    }

    *string = prev_string;

    return;
}

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

//@Speed we probably could make a faster version of this, but who cares?
bool string_to_int(String string, int * result) {
    for_array(string.data, string.count) {
        if(*it < '0' || *it > '9') return false;
    }

    char * c_string = to_c_string(string);

    *result = atoi(c_string);

    free(c_string);
    return true;
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

int cut_spaces(String * string) {
    int orig_count = string->count;
    while((*string)[0] == ' ') push(string);
    return orig_count - string->count;
}


int cut_trailing_spaces(String * string) {
    int orig_count = string->count;
    while((*string)[string->count - 1] == ' ') string->count -= 1;
    return orig_count - string->count;
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

    cut_trailing_spaces(&line);

    return line;
}

// Returns String right of char, non-inclusive
String find_char_from_right(char c, String string) {
    String rhs;
    rhs.data = string.data + string.count; // Pointer to end of string

    for(int i = string.count - 1; i >= 0; i--) {
        if(string[i] == c) break;

        rhs.data  -= 1;
        rhs.count += 1;
    }

    return rhs;
}

// Returns String right of char, non-inclusive
String find_char_from_left(char c, String string) {
    String rhs = string;

    for(int i = 0; i < string.count; i++) {
        rhs.data  += 1;
        rhs.count -= 1;

        if(string[i] == c) break;
    }

    return rhs;
}
