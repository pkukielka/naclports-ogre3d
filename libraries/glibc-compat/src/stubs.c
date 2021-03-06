/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <utime.h>

#define UNIMPLEMENTED_FATAL()                                            \
  fprintf(stderr,                                                        \
          "Function %s is not implemented in glibc-compat, aborting!\n", \
          __FUNCTION__);                                                 \
  abort();

#define UNIMPLEMENTED() \
  fprintf(stderr, "Function %s is not implemented in glibc-compat!\n", \
          __FUNCTION__);


mode_t umask(mode_t mask) __attribute__ ((weak));
mode_t umask(mode_t mask) {
  return 0x12; /* 022 */
}

int unlink(const char *pathname) __attribute__ ((weak));
int unlink(const char *pathname) {
  UNIMPLEMENTED_FATAL();
}

uid_t getuid(void) __attribute__ ((weak));
uid_t getuid(void) {
  return 100;
}

uid_t geteuid(void) __attribute__ ((weak));
uid_t geteuid(void) {
  return 100;
}

int setgid(gid_t gid) __attribute__ ((weak));
int setgid(gid_t gid) {
  UNIMPLEMENTED_FATAL();
}

int setegid(gid_t gid) __attribute__ ((weak));
int setegid(gid_t gid) {
  UNIMPLEMENTED_FATAL();
}

int setuid(uid_t uid) __attribute__ ((weak));
int setuid(uid_t uid) {
  UNIMPLEMENTED_FATAL();
}

gid_t getgid(void) __attribute__ ((weak));
gid_t getgid(void) {
  UNIMPLEMENTED_FATAL();
}

gid_t getegid(void) __attribute__ ((weak));
gid_t getegid(void) {
  UNIMPLEMENTED_FATAL();
}

int rmdir(const char *pathname) __attribute__ ((weak));
int rmdir(const char *pathname) {
  UNIMPLEMENTED_FATAL();
}

FILE *popen(const char *command, const char *type) __attribute__ ((weak));
FILE *popen(const char *command, const char *type) {
  UNIMPLEMENTED_FATAL();
}

int pclose(FILE *stream) __attribute__ ((weak));
int pclose(FILE *stream) {
  UNIMPLEMENTED_FATAL();
}

int system(const char *command) __attribute__ ((weak));
int system(const char *command) {
  UNIMPLEMENTED_FATAL();
}

int execl(const char *path, const char *arg, ...) __attribute__ ((weak));
int execl(const char *path, const char *arg, ...) {
  UNIMPLEMENTED_FATAL();
}

int execlp(const char *file, const char *arg, ...) __attribute__ ((weak));
int execlp(const char *file, const char *arg, ...) {
  UNIMPLEMENTED_FATAL();
}

int execv(const char *path, char *const argv[]) __attribute__ ((weak));
int execv(const char *path, char *const argv[]) {
  UNIMPLEMENTED_FATAL();
}

int pipe(int pipefd[2]) __attribute__ ((weak));
int pipe(int pipefd[2]) {
  UNIMPLEMENTED_FATAL();
}

int link(const char *oldpath, const char *newpath) __attribute__ ((weak));
int link(const char *oldpath, const char *newpath) {
  UNIMPLEMENTED_FATAL();
}

void openlog(const char *ident, int option, int facility)
    __attribute__ ((weak));
void openlog(const char *ident, int option, int facility) {
  UNIMPLEMENTED_FATAL();
}

void syslog(int priority, const char *format, ...) __attribute__ ((weak));
void syslog(int priority, const char *format, ...) {
  UNIMPLEMENTED_FATAL();
}

void closelog(void) __attribute__ ((weak));
void closelog(void) {
  UNIMPLEMENTED_FATAL();
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    __attribute__ ((weak));
ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  UNIMPLEMENTED_FATAL();
}

int initgroups(const char *user, gid_t group) __attribute__ ((weak));
int initgroups(const char *user, gid_t group) {
  UNIMPLEMENTED_FATAL();
}

int getgroups(int size, gid_t list[]) __attribute__ ((weak));
int getgroups(int size, gid_t list[]) {
  UNIMPLEMENTED_FATAL();
}

int setgroups(size_t size, const gid_t *list) __attribute__ ((weak));
int setgroups(size_t size, const gid_t *list) {
  UNIMPLEMENTED_FATAL();
}

struct passwd *getpwnam(const char *name) __attribute__ ((weak));
struct passwd *getpwnam(const char *name) {
  UNIMPLEMENTED_FATAL();
}

int utime(const char *filename, const struct utimbuf *times)
    __attribute__ ((weak));
int utime(const char *filename, const struct utimbuf *times) {
  UNIMPLEMENTED_FATAL();
}

int chdir(const char *path) __attribute__ ((weak));
int chdir(const char *path) {
  UNIMPLEMENTED_FATAL();
}

pid_t setsid(void) __attribute__ ((weak));
pid_t setsid(void) {
  UNIMPLEMENTED_FATAL();
}

int lstat(const char *path, struct stat *buf) __attribute__ ((weak));
int lstat(const char *path, struct stat *buf) {
  UNIMPLEMENTED_FATAL();
}

int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout) __attribute__ ((weak));
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout) {
  UNIMPLEMENTED_FATAL();
}

int socket(int domain, int type, int protocol) __attribute__ ((weak));
int socket(int domain, int type, int protocol) {
  UNIMPLEMENTED_FATAL();
}

int ioctl(int d, int request, ...) __attribute__ ((weak));
int ioctl(int d, int request, ...) {
  UNIMPLEMENTED_FATAL();
}

int kill(pid_t pid, int sig) __attribute__ ((weak));
int kill(pid_t pid, int sig) {
  UNIMPLEMENTED_FATAL();
}

int connect(int sockfd, const struct sockaddr *addr,
            socklen_t addrlen) __attribute__ ((weak));
int connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen) {
  UNIMPLEMENTED_FATAL();
}

int utimes(const char *filename, const struct timeval times[2])
    __attribute__ ((weak));
int utimes(const char *filename, const struct timeval times[2]) {
  UNIMPLEMENTED_FATAL();
}
