#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>

const char *progname = "nis-notify";
const char *pidfile = "/var/run/nis-notify.pid";

void check_running_daemon() {
  pid_t childpid = -1;
  FILE *f = fopen(pidfile, "r");
  if (f != NULL) {
    fscanf(f, "%d", &childpid);
    fclose(f);
    if (childpid > 0 && kill(childpid, 0) == 0) {
      fprintf(stderr, "%s already running!\n", progname);
      exit(1);
    }
  }
}

void finalize(int sig) {
  remove("/var/run/nis-notify.pid");
  syslog(LOG_INFO, "Daemon stopped.");
  exit(0);
}

int main(int argc, char *argv[]) {
  const char *filename = "/etc";
  size_t bufsiz = sizeof(struct inotify_event) + PATH_MAX + 1;
  struct inotify_event* event = malloc(bufsiz);
  int inotfd, watch_desc;
  pid_t childpid = -1;

  int success = chdir("/var/yp");

  openlog("nis-notify", LOG_PID, LOG_DAEMON);

  if (success == -1) {
    syslog(LOG_ERR, "/var/yp does not exist! "
                    "Daemon not started!");
    exit(1);
  }

  check_running_daemon();

  childpid = fork();
  if (childpid == -1) {
    syslog(LOG_CRIT, "Fork failed! Daemon not started!");
    exit(1);
  }

  if (childpid > 0) {
    FILE *f = fopen(pidfile, "w");
    fprintf(f, "%d\n", childpid);
    fclose(f);
    exit(0);
  }

  close(0);
  close(1);
  close(2);

  inotfd = inotify_init();
  watch_desc = inotify_add_watch(inotfd, filename,
                                 IN_CREATE|IN_MODIFY|IN_MOVED_TO);

  syslog(LOG_INFO, "Daemon running. Changes in /etc/passwd, "
                   "/etc/group and /etc/shadow will cause the"
                   " NIS database to automatically update.");

  signal(SIGTERM, finalize);
  signal(SIGINT, finalize);

  while(read(inotfd, event, bufsiz) > 0) {
    event->name[PATH_MAX-1] = '\0';
    if (strcmp(event->name,"passwd") == 0 ||
        strcmp(event->name,"group") == 0 ||
        strcmp(event->name,"shadow") == 0) {
      syslog(LOG_INFO, "File /etc/%s changed (%d,%d). "
                       "Running cd /var/yp && make.",
             event->name, event->mask, event->cookie);
      system("/usr/bin/make");
    }
  }

}
