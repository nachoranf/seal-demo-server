#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

#define MAX_SESSIONS 16
#define MAX_KEY 32
#define MAX_VALUE 256

struct session {
    int id;
    char user[MAX_KEY];
    char *note;
    size_t requests;
};

struct request {
    char verb[8];
    char key[MAX_KEY];
    char value[MAX_VALUE];
    int session_id;
};

int parse_request(const char *line, struct request *out);
int handle_request(const struct request *req, char *reply, size_t cap);
struct session *session_open(const char *user);
void session_close(int id);
int sessions_dump(const char *path);
double average_value_length(void);

#endif
