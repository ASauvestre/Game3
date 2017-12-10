#pragma once

#include "string.h"
#include "array.h"

struct Vector2;
struct Vector2f;

struct String { // @ Cleanup, move out of here.
    char * data  = NULL;
    int    count = 0;

    inline char& operator[](int i) {
        return this->data[i];
    }

    String& operator=(String & string) {
        this->count = string.count;
        this->data  = string.data;

        return *this;
    }

    bool operator==(String & b) {
        if(this->count != b.count) return false;
        return (memcmp(this->data, b.data, b.count) == 0);
    }

    bool operator==(char * b) {
        int len = strlen(b);

        if(this->count != len) return false;
        return (memcmp(this->data, b, len) == 0);

    }

    bool operator!=(char * b) {
        int len = strlen(b);

        if(this->count != len) return true;
        return (memcmp(this->data, b, len) != 0);

    }

};

bool string_compare(String s1, String s2);

String cut_until_char(char c, String * string);
String cut_until_space(String * string);

int    cut_spaces(String * string);
int    cut_trailing_spaces(String * string);

Array<String> strip_comments_from_file(String string, char * comment_marker = "//");

void skip_empty_lines(String * string);
String bump_to_next_line(String * string);
void   push(String * string, int amount = 1);

bool string_to_int(String string, int * result);
bool string_to_float(String string, float * result);

bool string_to_v2 (String string, Vector2 * result);
bool string_to_v2f(String string, Vector2f * result);

char * to_c_string(String string);
String to_string(char * c_string);

String find_char_from_right(char c, String string);
String find_char_from_left (char c, String string);
