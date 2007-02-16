// strparse.h -- header for String_parse class

class String_parse {
public:
    char string[256];
    int pos;
    void init(char *s) {
        strcpy(string, s);
        pos = 0;
    }
    void skip_space();
    char peek();
    void get_nonspace_quoted(char *field);
};

void string_escape(char *result, char *str, char *quote);
