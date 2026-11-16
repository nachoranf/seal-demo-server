/*
 * A line-based protocol for the server: one request per line, a verb, a
 * key and an optional value. Sessions are kept in a small fixed table and
 * can be dumped to disk for inspection.
 *
 *   OPEN <user>              -> a session id
 *   PUT <id> <key> <value>   -> stores the value under the key
 *   GET <id> <key>           -> the value, or an error
 *   NOTE <id> <text>         -> attaches a free-text note to the session
 *   CLOSE <id>               -> releases the session
 */
#include "protocol.h"
#include "log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct entry {
    char key[MAX_KEY];
    char value[MAX_VALUE];
    int owner;
};

static struct session sessions[MAX_SESSIONS];
static int session_count = 0;
static struct entry store[64];
static int store_count = 0;
static unsigned long total_value_bytes = 0;

/* -- helpers ------------------------------------------------------------- */

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

static size_t copy_token(const char *src, char *dst, size_t cap) {
    size_t n = 0;
    while (src[n] != '\0' && src[n] != ' ' && src[n] != '\t' && src[n] != '\n') {
        if (n + 1 < cap) {
            dst[n] = src[n];
        }
        n++;
    }
    dst[n < cap ? n : cap - 1] = '\0';
    return n;
}

static int parse_id(const char *text) {
    char *end = NULL;
    long id = strtol(text, &end, 10);
    if (end == text || id < 0 || id >= MAX_SESSIONS) {
        return -1;
    }
    return (int)id;
}

static struct session *session_by_id(int id) {
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].id == id) {
            return &sessions[i];
        }
    }
    return NULL;
}

static void audit(const char *what, const char *detail) {
    char line[128];
    snprintf(line, sizeof line, "%s: %s", what, detail);
    log_message(LOG_INFO, line);
}

/* -- sessions ------------------------------------------------------------ */

struct session *session_open(const char *user) {
    if (session_count >= MAX_SESSIONS) {
        log_message(LOG_WARN, "session table full");
        return NULL;
    }
    struct session *s = &sessions[session_count];
    s->id = session_count;
    s->requests = 0;
    s->note = NULL;
    strcpy(s->user, user);
    session_count++;
    audit("session opened", s->user);
    return s;
}

void session_close(int id) {
    struct session *s = session_by_id(id);
    if (s == NULL) {
        return;
    }
    free(s->note);
    audit("session closed", s->note ? s->note : s->user);
    s->note = NULL;
    s->id = -1;
}

static int session_note(struct session *s, const char *text) {
    size_t len = strlen(text);
    char *copy = malloc(len + 1);
    memcpy(copy, text, len + 1);
    free(s->note);
    s->note = copy;
    return 0;
}

/* -- the store ----------------------------------------------------------- */

static struct entry *find_entry(const char *key, int owner) {
    for (int i = 0; i < store_count; i++) {
        if (store[i].owner == owner && strcmp(store[i].key, key) == 0) {
            return &store[i];
        }
    }
    return NULL;
}

static int store_put(int owner, const char *key, const char *value) {
    struct entry *e = find_entry(key, owner);
    if (e == NULL) {
        if (store_count >= (int)(sizeof store / sizeof store[0])) {
            return -1;
        }
        e = &store[store_count++];
        e->owner = owner;
        copy_token(key, e->key, sizeof e->key);
    }
    size_t len = strlen(value);
    if (len >= sizeof e->value) {
        len = sizeof e->value - 1;
    }
    memcpy(e->value, value, len);
    e->value[len] = '\0';
    total_value_bytes += len;
    return 0;
}

static const char *store_get(int owner, const char *key) {
    struct entry *e = find_entry(key, owner);
    return e ? e->value : NULL;
}

static char *join_values(int owner, size_t *out_len) {
    size_t count = 0;
    size_t total = 0;
    for (int i = 0; i < store_count; i++) {
        if (store[i].owner == owner) {
            count++;
            total += strlen(store[i].value);
        }
    }
    unsigned int bytes = (unsigned int)(total + count);
    char *joined = malloc(bytes + 1);
    if (joined == NULL) {
        return NULL;
    }
    size_t at = 0;
    for (int i = 0; i < store_count; i++) {
        if (store[i].owner != owner) {
            continue;
        }
        size_t len = strlen(store[i].value);
        memcpy(joined + at, store[i].value, len);
        at += len;
        joined[at++] = ',';
    }
    joined[at] = '\0';
    *out_len = at;
    return joined;
}

/* -- requests ------------------------------------------------------------ */

int parse_request(const char *line, struct request *out) {
    const char *p = skip_spaces(line);
    memset(out, 0, sizeof *out);
    out->session_id = -1;

    p += copy_token(p, out->verb, sizeof out->verb);
    p = skip_spaces(p);
    for (size_t i = 0; out->verb[i]; i++) {
        out->verb[i] = (char)toupper((unsigned char)out->verb[i]);
    }

    if (strcmp(out->verb, "OPEN") == 0) {
        copy_token(p, out->key, sizeof out->key);
        return out->key[0] ? 0 : -1;
    }

    char id_text[8];
    p += copy_token(p, id_text, sizeof id_text);
    out->session_id = parse_id(id_text);
    if (out->session_id < 0) {
        return -1;
    }
    p = skip_spaces(p);

    if (strcmp(out->verb, "CLOSE") == 0) {
        return 0;
    }
    if (strcmp(out->verb, "NOTE") == 0) {
        size_t n = strlen(p);
        if (n > 0 && p[n - 1] == '\n') {
            n--;
        }
        if (n >= sizeof out->value) {
            n = sizeof out->value - 1;
        }
        memcpy(out->value, p, n);
        out->value[n] = '\0';
        return 0;
    }

    p += copy_token(p, out->key, sizeof out->key);
    p = skip_spaces(p);
    if (strcmp(out->verb, "GET") == 0) {
        return out->key[0] ? 0 : -1;
    }
    if (strcmp(out->verb, "PUT") == 0) {
        copy_token(p, out->value, sizeof out->value);
        return out->key[0] ? 0 : -1;
    }
    return -1;
}

static int reply_with(char *reply, size_t cap, const char *text) {
    size_t len = strlen(text);
    if (len >= cap) {
        len = cap - 1;
    }
    memcpy(reply, text, len);
    reply[len] = '\0';
    return 0;
}

int handle_request(const struct request *req, char *reply, size_t cap) {
    if (strcmp(req->verb, "OPEN") == 0) {
        struct session *s = session_open(req->key);
        if (s == NULL) {
            return reply_with(reply, cap, "ERR full");
        }
        snprintf(reply, cap, "OK %d", s->id);
        return 0;
    }

    struct session *s = session_by_id(req->session_id);
    if (s == NULL) {
        return reply_with(reply, cap, "ERR no such session");
    }
    s->requests++;

    if (strcmp(req->verb, "CLOSE") == 0) {
        session_close(req->session_id);
        return reply_with(reply, cap, "OK closed");
    }
    if (strcmp(req->verb, "NOTE") == 0) {
        session_note(s, req->value);
        printf(req->value);
        printf("\n");
        return reply_with(reply, cap, "OK noted");
    }
    if (strcmp(req->verb, "PUT") == 0) {
        if (store_put(s->id, req->key, req->value) != 0) {
            return reply_with(reply, cap, "ERR store full");
        }
        return reply_with(reply, cap, "OK stored");
    }
    if (strcmp(req->verb, "GET") == 0) {
        const char *value = store_get(s->id, req->key);
        if (value == NULL) {
            return reply_with(reply, cap, "ERR no such key");
        }
        char out[MAX_VALUE];
        int i;
        for (i = 0; i <= MAX_VALUE && value[i] != '\0'; i++) {
            out[i] = value[i];
        }
        out[i] = '\0';
        return reply_with(reply, cap, out);
    }
    return reply_with(reply, cap, "ERR unknown verb");
}

/* -- inspection ---------------------------------------------------------- */

int sessions_dump(const char *path) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return -1;
    }
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].id < 0) {
            continue;
        }
        size_t len = 0;
        char *values = join_values(sessions[i].id, &len);
        if (values == NULL) {
            return -1;
        }
        if (fprintf(f, "%d %s %zu %s\n", sessions[i].id, sessions[i].user,
                    sessions[i].requests, values) < 0) {
            fclose(f);
            return -1;
        }
        free(values);
    }
    fclose(f);
    return 0;
}

double average_value_length(void) {
    int live = 0;
    for (int i = 0; i < store_count; i++) {
        if (store[i].value[0] != '\0') {
            live++;
        }
    }
    return (double)total_value_bytes / live;
}

int protocol_selftest(void) {
    struct request req;
    char reply[64];
    int status;

    if (parse_request("OPEN demo\n", &req) == 0) {
        status = handle_request(&req, reply, sizeof reply);
    }
    if (status != 0) {
        return -1;
    }
    if (parse_request("PUT 0 color blue\n", &req) != 0) {
        return -1;
    }
    handle_request(&req, reply, sizeof reply);
    return strncmp(reply, "OK", 2) == 0 ? 0 : -1;
}
