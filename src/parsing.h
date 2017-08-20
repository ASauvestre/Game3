#pragma once

#include "string.h"

struct String {
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
};

String cut_until_char(char c, String * string);
String cut_until_space(String * string);

int    cut_spaces(String * string);
int    cut_trailing_spaces(String * string);

String bump_to_next_line(String * string);
void   push(String * string, int amount = 1);

char * to_c_string(String string);

String find_char_from_right(char c, String string);
String find_char_from_left (char c, String string);
