#pragma once
#include <sys/types.h>

/* Background downloads: each job is a curl child process writing to
 * "<dest>.part", renamed to <dest> on success. Polled from the UI loop. */
typedef struct {
    pid_t pid;          /* 0 = idle */
    char url[1024];
    char dest[512];
    char part[520];
    int insecure;       /* retried with -k after a certificate error */
    int exit_code;      /* curl exit code of the last finished run */
    long bytes;         /* bytes downloaded so far (updated by job_poll) */
} Job;

enum { JOB_RUNNING = 0, JOB_OK = 1, JOB_FAIL = -1 };

int net_init(void);                 /* finds curl; returns -1 if missing */
const char *net_curl_path(void);
int job_start(Job *j, const char *url, const char *dest);
int job_poll(Job *j);               /* JOB_RUNNING / JOB_OK / JOB_FAIL */
int job_busy(const Job *j);
void job_cancel(Job *j);
const char *net_error(int curl_exit);
void url_escape_path(char *out, int outlen, const char *in);
