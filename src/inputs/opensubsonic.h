#ifndef __OPENSUBSONIC_H__
#define __OPENSUBSONIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

struct opensubsonic_status
{
  bool enabled;
  bool connected;
  char server_url[256];
  char username[128];
  bool has_connection;
};

struct opensubsonic_backend
{
  int (*init)(void);
  void (*deinit)(void);
  int (*connect)(const char *server_url, const char *username, const char *password, const char **errmsg);
  void (*disconnect)(void);
  void (*status_get)(struct opensubsonic_status *status);
};

int
opensubsonic_init(void);

void
opensubsonic_deinit(void);

int
opensubsonic_connect(const char *server_url, const char *username, const char *password, const char **errmsg);

void
opensubsonic_disconnect(void);

void
opensubsonic_status_get(struct opensubsonic_status *status);

#ifdef __cplusplus
}
#endif

#endif /* !__OPENSUBSONIC_H__ */