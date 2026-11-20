#include "net.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static char curl_path[256];
static int force_insecure; /* sticky once a cert error has been seen */

int net_init(void)
{
    const char *env = getenv("HBS_CURL");
    const char *cands[] = {env, "/mnt/SDCARD/.tmp_update/bin/curl", "/usr/bin/curl", "/bin/curl",
                           "/usr/local/bin/curl", "/opt/homebrew/opt/curl/bin/curl", NULL};
    for (int i = 0; i < 6; i++) {
        if (cands[i] && access(cands[i], X_OK) == 0) {
            snprintf(curl_path, sizeof curl_path, "%s", cands[i]);
            return 0;
        }
    }
    return -1;
}

const char *net_curl_path(void) { return curl_path; }

int job_busy(const Job *j) { return j->pid > 0; }

static int spawn(Job *j)
{
    unlink(j->part);
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int nul = open("/dev/null", O_RDWR);
        if (nul >= 0) { dup2(nul, 0); dup2(nul, 1); dup2(nul, 2); }
        const char *argv[24];
        int n = 0;
        argv[n++] = curl_path;
        argv[n++] = "-s";
        argv[n++] = "-L";
        argv[n++] = "-f";
        argv[n++] = "--connect-timeout";
        argv[n++] = "15";
        argv[n++] = "--max-time";
        argv[n++] = "900";
        argv[n++] = "-A";
        argv[n++] = "HomebrewStore/1.0 (Miyoo Mini homebrew browser)";
        if (j->insecure) argv[n++] = "-k";
        argv[n++] = "-o";
        argv[n++] = j->part;
        argv[n++] = j->url;
        argv[n] = NULL;
        execv(curl_path, (char *const *)argv);
        _exit(127);
    }
    j->pid = pid;
    j->bytes = 0;
    return 0;
}

int job_start(Job *j, const char *url, const char *dest)
{
    if (j->pid > 0) return -1;
    snprintf(j->url, sizeof j->url, "%s", url);
    snprintf(j->dest, sizeof j->dest, "%s", dest);
    snprintf(j->part, sizeof j->part, "%s.part", dest);
    j->insecure = force_insecure;
    j->exit_code = 0;
    return spawn(j);
}

int job_poll(Job *j)
{
    if (j->pid <= 0) return j->exit_code == 0 ? JOB_OK : JOB_FAIL;
    struct stat st;
    if (stat(j->part, &st) == 0) j->bytes = (long)st.st_size;
    int status;
    pid_t r = waitpid(j->pid, &status, WNOHANG);
    if (r == 0) return JOB_RUNNING;
    if (r < 0 && errno == EINTR) return JOB_RUNNING;
    j->pid = 0;
    int code = (r > 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : 255;
    /* 60/77/35/51/58: certificate or TLS trouble. The handheld often has no CA
     * bundle and a wrong clock, so retry without verification (as Onion's
     * own updater does). */
    if ((code == 60 || code == 77 || code == 35 || code == 51 || code == 58) && !j->insecure) {
        j->insecure = 1;
        force_insecure = 1;
        if (spawn(j) == 0) return JOB_RUNNING;
    }
    j->exit_code = code;
    if (code == 0 && rename(j->part, j->dest) == 0) return JOB_OK;
    if (code == 0) j->exit_code = 23; /* write error */
    unlink(j->part);
    return JOB_FAIL;
}

void job_cancel(Job *j)
{
    if (j->pid > 0) {
        kill(j->pid, SIGTERM);
        waitpid(j->pid, NULL, 0);
        j->pid = 0;
        unlink(j->part);
    }
}

const char *net_error(int c)
{
    switch (c) {
    case 0: return "OK";
    case 5: case 6: return "No internet. Is Wi-Fi on?";
    case 7: return "Can't reach the server. Is Wi-Fi on?";
    case 22: return "File not found on the server";
    case 23: return "Couldn't write to the SD card";
    case 28: return "Connection timed out";
    case 127: return "curl couldn't start";
    }
    static char buf[48];
    snprintf(buf, sizeof buf, "Download failed (curl error %d)", c);
    return buf;
}

void url_escape_path(char *out, int outlen, const char *in)
{
    static const char hex[] = "0123456789ABCDEF";
    int o = 0;
    for (const unsigned char *p = (const unsigned char *)in; *p && o < outlen - 4; p++) {
        unsigned c = *p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            out[o++] = (char)c;
        } else {
            out[o++] = '%';
            out[o++] = hex[c >> 4];
            out[o++] = hex[c & 15];
        }
    }
    out[o] = 0;
}
