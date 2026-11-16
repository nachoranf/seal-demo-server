# seal-demo-server

A small C program that connects to a host, says hello and leaves. It exists so
that [SEAL](https://github.com/nachoranf/SEAL) has something real to check a
pull request against: a ticket in Jira, a branch named after it, and the graph
of this repository built on every push to `main`.

Eleven files, five hundred lines. Nothing here is meant to run in production.

| File | What it does |
|---|---|
| `main.c` | Loads the configuration, connects with retries, sends `HELLO` |
| `net.c` | `connect_once`, `connect_with_retry`, `send_all` |
| `config.c` | Parses host and port, reads the log level |
| `log.c` | Levels and one `log_message` |
| `buffer.c` | A bounded buffer with `append` |
| `protocol.c` | A line protocol: sessions, a key-value store, and a dump to disk |
