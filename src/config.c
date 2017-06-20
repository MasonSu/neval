#define _BSD_SOURCE

#include "config.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

struct nevalServer server;

static void loadServerConfigFromString(char *config);
static char *strtrim(char *str, const char *set);
static void strsplit(char *str, char *key, char *value);

void loadServerConfig(char *filename, char *options) {
  char config[MAX_BUFLEN] = {0}, buf[MAX_LINE + 1];

  if (filename) {
    FILE *fp = fopen(filename, "r");
    check_exit(fp != NULL, "Fatal error, can't open config file %s", filename);

    while (fgets(buf, MAX_LINE + 1, fp) != NULL)
      strncat(config, buf, strlen(buf));

    fclose(fp);
  }
  /* Append the additional options */
  if (options) {
    strncat(config, "\n", 1);
    strncat(config, options, strlen(options));
  }

  loadServerConfigFromString(config);

  return;
}

void loadServerConfigFromString(char *config) {
  char *str = strdup(config);
  char *tofree = str, *token, *errstr;
  int linenum = 0;

  while ((token = strsep(&str, "\n")) != NULL) {
    linenum++;
    token = strtrim(token, " \t\r\n");
    // debug("token is %s", token);
    /* Skip comments and blank lines */
    if (*token == '#' || *token == '\0')
      continue;

    char key[MAX_LINE], value[MAX_LINE];
    strsplit(token, key, value);

    if (!strncasecmp(key, "root", 4)) {
      strcpy(server.root, value);
    } else if (!strncasecmp(key, "port", 4)) {
      server.port = atoi(value);
      if (server.port < 0 || server.port > 65535) {
        errstr = "Invalid port";
        goto err;
      }
    } else if (!strncasecmp(key, "timeout", 7)) {
      server.timeout = atoi(value);
      if (server.timeout < 0) {
        errstr = "Invalid timeout value";
        goto err;
      }
    }
  }

  free(tofree);

  return;

err:
  fprintf(stderr, "\n*** FATAL CONFIG FILE ERROR ***\n");
  fprintf(stderr, "Reading the configuration file, at line %d\n", linenum);
  fprintf(stderr, ">>> '%s'\n", token);
  fprintf(stderr, "%s\n", errstr);
  exit(1);
}

char *strtrim(char *str, const char *set) {
  char *sp, *start, *ep, *end;

  sp = start = str;
  ep = end = str + strlen(str) - 1;

  while (sp <= end && strchr(set, *sp))
    sp++;
  while (ep > sp && strchr(set, *ep))
    ep--;

  size_t len = (ep > sp) ? ep - sp + 1 : 0;
  if (str != sp)
    memmove(str, sp, len);
  str[len] = '\0';
  return str;
}

void strsplit(char *str, char *key, char *value) {
  char *keyStart, *valueStart, *sp, *set = " \t\t\n";

  keyStart = sp = str;
  while (*sp && strchr(set, *sp) == NULL)
    sp++;
  *sp++ = '\0';
  strcpy(key, keyStart);

  while (*sp && strchr(set, *sp))
    sp++;

  valueStart = sp;
  while (*sp && strchr(set, *sp) == NULL)
    sp++;
  strcpy(value, valueStart);

  return;
}