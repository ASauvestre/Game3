#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parsing.h"
#include "macros.h"
#include "array.h"
#include "math_m.h" // Vector parsing

bool string_compare(String s1, String s2) {
    if(s1.count != s2.count) return false;

    return (memcmp(s1.data, s2.data, s1.count) == 0);
}

int get_file_version_number(Array<String> lines, char * file_name) {
    if(lines.count <= 0) {
        log_print("get_file_version_number", "Failed to parse %s. It's empty.", file_name);
    }

    String line = lines.data[0];

    String token = cut_until_space(&line);

    if(token != "VERSION") {
        log_print("get_file_version_number", "Failed to parse the \"VERSION\" token at the beginning of the file %s. Please make sure it is there.", file_name);

        return -1;
    }

    String version_token = cut_until_space(&line);


    if(!version_token.count) {
        log_print("get_file_version_number", "We parsed the \"VERSION\" token at the beginning of the file %s, but no version number was provided after it. Please make sure to put it on the same line.", file_name);
        return -1;
    }

    if(line.count) {
        char * c_line = to_c_string(line);
        scope_exit(free(c_line));

        log_print("get_file_version_number", "There is garbage left on the first line of file %s after the version number: %s", file_name, c_line);

        return -1;
    }


    if(version_token.data[0] != '<' || version_token.data[version_token.count - 1] != '>') {
        char * c_version_token = to_c_string(version_token);
        scope_exit(free(c_version_token));

        log_print("get_file_version_number", "The version number after \"VERSION\" in file %s needs to be surrounded with '<' and '>', instead, we got this : %s", file_name, c_version_token);

        return -1;
    }

    // Isolating version number
    push(&version_token);
    version_token.count -= 1;

    int version;
    bool success = string_to_int(version_token, &version);

    if(!success) {
        char * c_version_token = to_c_string(version_token);
        scope_exit(free(c_version_token));

        log_print("get_file_version_number", "We expected a version number after \"VERSION\" in file %s, but instead we got this: %s.", file_name, c_version_token);

        return -1;
    }

    return version;
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

Array<String> strip_comments_from_file(String string, char * comment_marker) { //@Default comment_marker = "//"

	String orig_string = string;
    Array<String> stripped_lines = {};

    while(true) {
        String line = bump_to_next_line(&string);

        if(line.count == -1) {
            stripped_lines.add(line);
            break; // EOF
        }

        if(line.count == 0) {
            stripped_lines.add(line);
            continue; // Empty line
        }

        String full_line = line;
        while(true) {
            cut_spaces(&line);
            String token = cut_until_space(&line);

            if(token.count == 0) break; // End of line.

            if(token == comment_marker) {
                full_line.count = (token.data - full_line.data);
				break;
            }
        }

		stripped_lines.add(full_line);
    }

	string = orig_string;
    return stripped_lines;
}

// @Speed we probably could make a faster version of this, but who cares?
bool string_to_int(String string, int * result) {
    for_array(string.data, string.count) {
        if(*it < '0' || *it > '9') return false;
    }

    char * c_string = to_c_string(string);

	if (!c_string)  return false;

    *result = atoi(c_string);

    free(c_string);
    return true;
}

// @Speed we probably could make a faster version of this, but who cares?
// @Incomplete Allow exponents to match C90 spec as well as INF
bool string_to_float(String string, float * result) {
    float sign = 1.0f;

    if(string[0] == '-') {
        push(&string);
        sign = -1.0f;
    }
    if(string[0] == '+') {
        push(&string);
    }

    bool found_dot = false;
    for_array(string.data, string.count) {
        if(*it == '.') {
            if(found_dot) return false;
            found_dot = true;
            continue;
        }

        if(*it < '0' || *it > '9') return false;
    }

    char * c_string = to_c_string(string);

    *result = atof(c_string) * sign;

    free(c_string);
    return true;
}

bool string_to_v2(String string, Vector2 * result) {
    String lhs = cut_until_space(&string);

    if(lhs.count    == 0 || !string_to_int(lhs,    &result->x)) return false;
    if(string.count == 0 || !string_to_int(string, &result->y)) return false;

    return true;
}

bool string_to_v2f(String string, Vector2f * result) {
    String lhs = cut_until_space(&string);

    if(lhs.count    == 0 || !string_to_float(lhs,    &result->x)) return false;
    if(string.count == 0 || !string_to_float(string, &result->y)) return false;

    return true;
}

char * to_c_string(String string) { // @Incomplete take pointer here ?
    if(!string.count) return NULL;

    char * c = (char *) malloc(string.count + 1);
    memcpy(c, string.data, string.count);
    c[string.count] = 0;

    return c;
}

String to_string(char * c_string) {
    String string;
    string.data = c_string;
    string.count = strlen(c_string); // @Think, should we copy here ?
    return string;
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
    String line;
    line.data  = string->data;
    line.count = cut_spaces(string);

    // Empty line / EOF check.
    {
        if(string->count == 0) {
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
           if(string->count == 0) {
                break; // EOF, will be handled next call (if first char is 0, we set count to -1)
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

    // cut_trailing_spaces(&line);

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
