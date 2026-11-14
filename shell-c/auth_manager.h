#ifndef AXIS_AUTH_MANAGER_H
#define AXIS_AUTH_MANAGER_H

#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <security/pam_appl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUTH_RESULT_SUCCESS,
    AUTH_RESULT_FAILED,
    AUTH_RESULT_ERROR,
    AUTH_RESULT_INFO
} AuthResult;

typedef void (*AuthStatusCallback)(AuthResult result, const char *message, gpointer user_data);

typedef struct {
    char *username;
    char *display_name;
    bool is_locked;
    bool is_authenticating;
    bool is_fingerprint_active;
    bool has_fingerprint;
    bool has_face_auth;

    GDBusConnection *system_bus;
    GDBusProxy *fprint_manager;
    GDBusProxy *fprint_device;
    char *fprint_device_path;

    AuthStatusCallback status_cb;
    gpointer status_user_data;

    GThread *auth_thread;
} AuthManager;

AuthManager *auth_manager_create(void);
void auth_manager_destroy(AuthManager *mgr);

void auth_manager_set_callback(AuthManager *mgr, AuthStatusCallback cb, gpointer user_data);
void auth_manager_set_username(AuthManager *mgr, const char *username);

void auth_manager_authenticate(AuthManager *mgr, const char *password);
void auth_manager_start_fingerprint(AuthManager *mgr);
void auth_manager_stop_fingerprint(AuthManager *mgr);

void auth_manager_lock(AuthManager *mgr);
void auth_manager_unlock(AuthManager *mgr);

#ifdef __cplusplus
}
#endif

#endif /* AXIS_AUTH_MANAGER_H */
