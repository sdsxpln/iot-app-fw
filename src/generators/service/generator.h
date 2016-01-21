/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Intel Corporation nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SERVICE_GENERATOR_H__
#define __SERVICE_GENERATOR_H__

#include <syslog.h>

#include <iot/common/macros.h>
#include <iot/common/log.h>
#include <iot/common/debug.h>
#include <iot/common/list.h>
#include <iot/common/json.h>

#include "jmpl/jmpl.h"

/* External helper we try to exec(3) for mounting PATH_APPS. */
#ifndef MOUNT_HELPER
#    define MOUNT_HELPER "/usr/libexec/iot-app-fw/mount-apps"
#endif

#ifndef PATH_TEMPLATE
#    define PATH_TEMPLATE "/usr/libexec/iot-app-fw/service.jmpl"
#endif

/* Absolute path to systemd-nspawn. */
#define PATH_NSPAWN "/usr/bin/systemd-nspawn"

/* Directory where applications are installed. */
#define PATH_APPS "/apps"

/* Top directory under which we stitch together container images. */
#define PATH_CONTAINER "/run/systemd/machines"

/* Directory to drop files into for systemd-sysusers. */
#define PATH_SYSUSERS "/usr/lib/sysusers.d"

/* Maximum allowed manifest file size. */
#define MANIFEST_MAXSIZE (16 * 1024)

/* Forward declarations for global types. */
typedef enum   mount_type_e   mount_type_t;
typedef struct mount_s        mount_t;
typedef struct generator_s    generator_t;
typedef struct service_s      service_t;


/*
 * Generator runtime context.
 *
 * This is used to collect and pass around all the necessary runtime data
 * for discovering applications and generating service files for them.
 */
struct generator_s {
    const char     **env;                /* environment variables */
    const char      *argv0;              /* argv[0], our binary */
    const char      *dir_normal;         /* systemd 'normal' service dir */
    const char      *dir_early;          /* systemd 'early' service dir */
    const char      *dir_late;           /* systemd 'late' service dir */
    const char      *dir_apps;           /* application top directory */
    const char      *dir_service;        /* service (output) directory */
    const char      *path_template;      /* template file path */
    const char      *log_path;           /* where to log to */
    int              dry_run : 1;        /* just a dry-run, don't generate */
    int              premounted : 1;     /* whether dir_apps was mounted */
    int              status;             /* service generation status */
    jmpl_t          *template;           /* service template */
    iot_list_hook_t  services;           /* generated service( file)s */
};


/*
 * Logging/debugging macros... just simple wrappers around common.
 */
#define log_open(_path) iot_log_set_target(_path)
#define log_close() do { } while (0)

#define log_error iot_log_error
#define log_warn  iot_log_warning
#define log_info  iot_log_info
#define log_debug iot_debug


/*
 * Configuration handling.
 */
int config_parse_cmdline(generator_t *g, int argc, char *argv[], char *env[]);


/*
 * Filesystem layout, paths, mounting, etc.
 */
char *fs_mkpath(char *path, size_t size, const char *fmt, ...);
int fs_mkdirp(mode_t mode, const char *fmt, ...);
int fs_same_device(const char *path1, const char *path2);
int fs_mount(const char *path);
int fs_umount(const char *path);
int fs_symlink(const char *path, const char *dst);
char *fs_service_path(service_t *s, char *path, size_t size);
char *fs_service_link(service_t *s, char *path, size_t size);

#define fs_accessible(_path, _mode) (access((_path), (_mode)) == 0)
#define fs_readable(_path) fs_accessible(_path, R_OK)
#define fs_writable(_path) fs_accessible(_path, W_OK)
#define fs_execable(_path) fs_accessible(_path, X_OK)


/*
 * Application discovery.
 */
int application_mount(generator_t *g);
int application_umount(generator_t *g);
int application_discover(generator_t *g);


/*
 * Template handling.
 */
int template_load(generator_t *g);
int template_eval(service_t *s);

iot_json_t *manifest_read(const char *path);


/*
 * Service file generation.
 */

/*
 * A systemd service file.
 *
 * Data structure used for collecting the necessary data about an
 * application for generating its systemd service file. The primary
 * source of information for this is the application manifest.
 */
struct service_s {
    iot_list_hook_t  hook;               /* to list of generated services */
    generator_t     *g;                  /* context back pointer */
    char            *provider;           /* application provider */
    char            *app;                /* application name */
    char            *appdir;             /* application directory */
    iot_json_t      *m;                  /* application manifest */
    iot_json_t      *data;               /* template configuration data */
    char            *output;             /* generated service file content */
    int              fd;                 /* service file */
    const char      *user;               /* user to run service as */
    const char      *group;              /* group to run service as */
    iot_json_t      *argv;               /* user command to execute */
    int              autostart : 1;      /* whether to start on boot */
};


service_t *service_create(generator_t *g, const char *provider, const char *app,
                          const char *dir, const char *src,
                          iot_json_t *manifest);
void service_abort(service_t *s);
int service_generate(generator_t *g);
int service_write(generator_t *g);

#endif /* __SERVICE_GENERATOR_H__ */