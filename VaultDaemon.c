#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <curl/curl.h>
#include <string.h>
#include "jsmn.h"

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if(s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
  strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

int main(int argc, char *argv[]) {
  /* Our process ID and Session ID */
  pid_t pid, sid;
  /* Fork off the parent process */
  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }
  /* If we got a good PID, then
  we can exit the parent process. */
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  /* Change the file mode mask */
  umask(0);
  /* Create a new SID for the child process */
  sid = setsid();
  if (sid < 0) {
    /* Log the failure */
    exit(EXIT_FAILURE);
  }
  /* Change the current working directory */
  if ((chdir("/")) < 0) {
    /* Log the failure */
    exit(EXIT_FAILURE);
  }
  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  /* Daemon-specific initialization goes here */
  /* Open the log file */
  openlog ("VaultDaemon", LOG_PID, LOG_DAEMON);
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  /* The Big Loop */
  while (1) {
    if(curl) {
      struct string json;
      init_string(&json);
      struct curl_slist *chunk = NULL;
      char auth[200];
      sprintf(auth, "X-VAULT-TOKEN: %s", argv[1]);
      chunk = curl_slist_append(chunk, auth);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);

      res = curl_easy_perform(curl);
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

      int i;
      int r;
      jsmn_parser p;
      jsmntok_t t[128];

      jsmn_init(&p);
      r = jsmn_parse(&p, json.ptr, json.len, t, sizeof(t)/sizeof(t[0]));
      if(r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
      }
      char res_data[200];
      for(i=1; i<r; i++) {
        if(jsoneq(json.ptr, &t[i], "data") == 0) {
          sprintf(res_data, "%.*s\n", t[i+1].end-t[i+1].start, json.ptr+t[i+1].start);
          i++;
        }
      }
      syslog (LOG_NOTICE, "VaultDaemon just got the secret data.");
      syslog (LOG_NOTICE, "%s\n", res_data);
      /* Begin - Use this section to do whatever you want with the res_data(secret) */
      
      /* End - Use this section to do whatever you want with the res_data(secret) */
      sleep(30);
      free(json.ptr);
    }
  }
  curl_easy_cleanup(curl);
  syslog (LOG_NOTICE, "Bye from VaultDaemon");
  closelog();
  exit(EXIT_SUCCESS);
}
