/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ---------------------------------------------------------------------
 * 
 * Original Authors: Aetnaeus (https://github.com/lemnos)
 *
 */

#include <pty.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int in;
static int out;

//Collect a sequence of lines terminated by a blank newline from the provided
//file descriptor.
static int collect_lines(int fd, char ***_lines, size_t *_sz) {
    size_t lines_sz = 0, lines_cap = 0;
    char **lines = NULL;

    FILE *fh = fdopen(fd, "r");
    while(1) {
        ssize_t sz;
        size_t n;
        char *line;

        n = 0;
        line = NULL;

        sz = getline(&line, &n, fh);
        if(sz < 1) {
            *_lines = NULL;
            *_sz = 0;
            return -1;
        }
        line[sz-1]='\0';

        if(sz == 1) {
            *_lines = lines;
            *_sz = lines_sz;
            return 0;
        } else {
            lines_sz++;
            if(lines_sz > lines_cap) {
                lines_cap = lines_sz*3/2;
                lines = realloc(lines, lines_cap*sizeof(char*));
            }

            lines[lines_sz-1] = line;
        }

    }
    free(fh);
}

void on_term(int _) {
    fprintf(stderr, "ERROR: child process prematurely terminated\n");
    exit(0);
}

void *waiter(void *_) {
    wait(NULL);
    kill(getpid(), SIGTERM);
    return NULL;
}

void program_filter_init(const char *name) {
    struct termios ti;
    int fds[2];
    int master, slave;

    pipe(fds);
    in = fds[1];

    openpty(&master, &slave, NULL, NULL, NULL);
    out = master;

    if(fork()) {
        pthread_t t;
        signal(SIGTERM, on_term);
        pthread_create(&t, NULL, waiter, NULL);

        char *line = NULL;
        size_t sz = 0;
        FILE *fp = fdopen(out, "r");
        getline(&line, &sz, fp);

        if(strcmp(line, "TALARIA V1\n")) {
            kill(0, SIGTERM);
            fprintf(stderr, "ERROR: %s does not appear to be a valid talaria script\n", name);
            exit(1);
        }
        return;
    }

    setsid();
    dup2(fds[0], 0);
    dup2(slave, 1);

    tcgetattr(1, &ti);
    cfmakeraw(&ti);
    tcsetattr(1, TCSANOW, &ti);

    execl("/bin/sh", "/bin/sh", "-c", name, NULL);
    close(out);
}

size_t program_filter_select(char *sel, char **res) {
    size_t len = strlen(sel);
    const char s[] = "SELECT:";

    write(in, s, sizeof(s)-1);
    write(in, sel, len);
    write(in, "\n", 1);

    *res = sel;
    return len;
}

size_t program_filter_filter(const char *input, char ***items) {
    size_t items_sz;
    const char opt[] = "FILTER:";

    write(in, opt, sizeof(opt)-1);
    write(in, input, strlen(input));
    write(in, "\n", 1);

    collect_lines(out, items, &items_sz);
    return items_sz;
}
