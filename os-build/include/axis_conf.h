#ifndef AXIS_CONF_H
#define AXIS_CONF_H

/**
 * AxisOS Configuration File & INI Engine (axis_conf.h)
 * Strictly Pure C11 (-std=gnu11 / -std=c17)
 * Adapted from reference confreader patterns.
 *
 * Lightweight, zero-dependency key-value and section parser supporting:
 * - [sections]
 * - key=value pairs
 * - # and ; comment lines
 * - In-memory query, update, and serialized file saving
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct AxisConfEntry {
    char *key;
    char *val;
    struct AxisConfEntry *next;
} AxisConfEntry;

typedef struct AxisConfSection {
    char *name;
    AxisConfEntry *entries;
    struct AxisConfSection *next;
} AxisConfSection;

typedef struct {
    AxisConfSection *sections;
} AxisConf;

static inline char *axis_conf_trim(char *s) {
    if (!s) return NULL;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static inline AxisConf *axis_conf_new(void) {
    AxisConf *conf = (AxisConf *)calloc(1, sizeof(AxisConf));
    return conf;
}

static inline AxisConfSection *axis_conf_find_or_create_section(AxisConf *conf, const char *name) {
    const char *sec_name = name ? name : "";
    AxisConfSection *sec = conf->sections;
    AxisConfSection *last = NULL;
    while (sec) {
        if (strcmp(sec->name, sec_name) == 0) return sec;
        last = sec;
        sec = sec->next;
    }
    AxisConfSection *new_sec = (AxisConfSection *)calloc(1, sizeof(AxisConfSection));
    new_sec->name = strdup(sec_name);
    if (last) last->next = new_sec;
    else conf->sections = new_sec;
    return new_sec;
}

static inline void axis_conf_set(AxisConf *conf, const char *section, const char *key, const char *val) {
    if (!conf || !key) return;
    AxisConfSection *sec = axis_conf_find_or_create_section(conf, section);
    AxisConfEntry *ent = sec->entries;
    AxisConfEntry *last = NULL;
    while (ent) {
        if (strcmp(ent->key, key) == 0) {
            free(ent->val);
            ent->val = val ? strdup(val) : strdup("");
            return;
        }
        last = ent;
        ent = ent->next;
    }
    AxisConfEntry *new_ent = (AxisConfEntry *)calloc(1, sizeof(AxisConfEntry));
    new_ent->key = strdup(key);
    new_ent->val = val ? strdup(val) : strdup("");
    if (last) last->next = new_ent;
    else sec->entries = new_ent;
}

static inline const char *axis_conf_get(AxisConf *conf, const char *section, const char *key, const char *defval) {
    if (!conf || !key) return defval;
    const char *sec_name = section ? section : "";
    AxisConfSection *sec = conf->sections;
    while (sec) {
        if (strcmp(sec->name, sec_name) == 0) {
            AxisConfEntry *ent = sec->entries;
            while (ent) {
                if (strcmp(ent->key, key) == 0) {
                    return ent->val;
                }
                ent = ent->next;
            }
            break;
        }
        sec = sec->next;
    }
    return defval;
}

static inline int axis_conf_get_int(AxisConf *conf, const char *section, const char *key, int defval) {
    const char *v = axis_conf_get(conf, section, key, NULL);
    if (!v) return defval;
    return atoi(v);
}

static inline AxisConf *axis_conf_load(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return NULL;

    AxisConf *conf = axis_conf_new();
    char line[1024];
    char current_section[256] = "";

    while (fgets(line, sizeof(line), fp)) {
        char *p = axis_conf_trim(line);
        if (*p == '\0' || *p == '#' || *p == ';') continue;

        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, p + 1, sizeof(current_section) - 1);
                current_section[sizeof(current_section) - 1] = '\0';
                axis_conf_trim(current_section);
                continue;
            }
        }

        char *eq = strchr(p, '=');
        if (eq) {
            *eq = '\0';
            char *key = axis_conf_trim(p);
            char *val = axis_conf_trim(eq + 1);
            axis_conf_set(conf, current_section, key, val);
        }
    }

    fclose(fp);
    return conf;
}

static inline int axis_conf_save(AxisConf *conf, const char *filepath) {
    if (!conf || !filepath) return -1;
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;

    // First write default / root section (empty string)
    AxisConfSection *sec = conf->sections;
    while (sec) {
        if (strcmp(sec->name, "") == 0) {
            AxisConfEntry *ent = sec->entries;
            while (ent) {
                fprintf(fp, "%s=%s\n", ent->key, ent->val);
                ent = ent->next;
            }
            if (sec->entries) fprintf(fp, "\n");
            break;
        }
        sec = sec->next;
    }

    // Next write named sections
    sec = conf->sections;
    while (sec) {
        if (strcmp(sec->name, "") != 0) {
            fprintf(fp, "[%s]\n", sec->name);
            AxisConfEntry *ent = sec->entries;
            while (ent) {
                fprintf(fp, "%s=%s\n", ent->key, ent->val);
                ent = ent->next;
            }
            fprintf(fp, "\n");
        }
        sec = sec->next;
    }

    fclose(fp);
    return 0;
}

static inline void axis_conf_free(AxisConf *conf) {
    if (!conf) return;
    AxisConfSection *sec = conf->sections;
    while (sec) {
        AxisConfSection *next_sec = sec->next;
        free(sec->name);
        AxisConfEntry *ent = sec->entries;
        while (ent) {
            AxisConfEntry *next_ent = ent->next;
            free(ent->key);
            free(ent->val);
            free(ent);
            ent = next_ent;
        }
        free(sec);
        sec = next_sec;
    }
    free(conf);
}

#endif /* AXIS_CONF_H */
