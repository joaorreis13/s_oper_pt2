#pragma once

int create_fifo_request(const char *name);
int create_fifo_anspid(const char *name);
int close_fifo(const char *fifo, int fd);