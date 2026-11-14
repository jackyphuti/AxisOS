#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <archive.h>
#include <archive_entry.h>
#include <openssl/evp.h>

#define DEFAULT_ROOT "/"
#define DEFAULT_STATE_DIR "/var/lib/axis"
#define DEFAULT_CACHE_DIR "/var/lib/axis/cache"
#define DEFAULT_DB_FILE "/var/lib/axis/installed.json"
#define DEFAULT_INDEX_FILE "/var/lib/axis/repo-index.json"
#define DEFAULT_REPO_URL "https://raw.githubusercontent.com/jackyphuti/AxisOS/main/pkg-mgr/examples"

static const char *g_root_dir = DEFAULT_ROOT;

static void safe_strcpy(char *dst, const char *src, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static int compute_file_sha256(const char *path, char *out_hex) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) { fclose(f); return -1; }

    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);

    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        EVP_DigestUpdate(mdctx, buf, n);
    }
    fclose(f);

    unsigned char md_val[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    EVP_DigestFinal_ex(mdctx, md_val, &md_len);
    EVP_MD_CTX_free(mdctx);

    for (unsigned int i = 0; i < md_len; ++i) {
        sprintf(out_hex + (i * 2), "%02x", md_val[i]);
    }
    out_hex[md_len * 2] = '\0';
    return 0;
}

static size_t write_file_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static int download_file(const char *url, const char *dest_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    FILE *f = fopen(dest_path, "wb");
    if (!f) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AxisOS-axis/2.0 (Pure-C)");

    CURLcode res = curl_easy_perform(curl);
    fclose(f);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || (http_code >= 400 && http_code < 600)) {
        unlink(dest_path);
        return -1;
    }
    return 0;
}

static void ensure_directories(void) {
    mkdir("/var", 0755);
    mkdir("/var/lib", 0755);
    mkdir(DEFAULT_STATE_DIR, 0755);
    mkdir(DEFAULT_CACHE_DIR, 0755);
}

static struct json_object *load_installed_db(void) {
    if (access(DEFAULT_DB_FILE, R_OK) == 0) {
        return json_object_from_file(DEFAULT_DB_FILE);
    }
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "packages", json_object_new_object());
    return obj;
}

static void save_installed_db(struct json_object *db) {
    ensure_directories();
    json_object_to_file_ext(DEFAULT_DB_FILE, db, JSON_C_TO_STRING_PRETTY);
}

static int do_update(void) {
    ensure_directories();
    printf("\033[94m[+] Updating package repositories...\033[0m\n");

    char index_url[512];
    snprintf(index_url, sizeof(index_url), "%s/repo-index.json", DEFAULT_REPO_URL);

    printf("    Fetching %s ...\n", index_url);
    if (download_file(index_url, DEFAULT_INDEX_FILE) != 0) {
        fprintf(stderr, "\033[91m[-] Error: Failed to download repository index.\033[0m\n");
        return 1;
    }

    struct json_object *idx = json_object_from_file(DEFAULT_INDEX_FILE);
    if (!idx) {
        fprintf(stderr, "\033[91m[-] Error: Invalid repo-index.json format.\033[0m\n");
        return 1;
    }

    struct json_object *pkgs = NULL;
    int count = 0;
    if (json_object_object_get_ex(idx, "packages", &pkgs)) {
        count = json_object_object_length(pkgs);
    }
    json_object_put(idx);

    printf("\033[92m[✓] Repository updated successfully! (%d packages available)\033[0m\n", count);
    return 0;
}

static int extract_package_archive(const char *axis_file, const char *dest_root,
                                   struct json_object **out_meta, struct json_object **out_files) {
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    struct archive *ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT);

    if (archive_read_open_filename(a, axis_file, 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        archive_write_free(ext);
        return -1;
    }

    *out_files = json_object_new_array();
    *out_meta = NULL;

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(entry);

        if (strcmp(name, "metadata.json") == 0 || strcmp(name, "./metadata.json") == 0) {
            la_int64_t size = archive_entry_size(entry);
            if (size > 0 && size < 10485760) {
                char *buf = malloc((size_t)size + 1);
                if (buf) {
                    la_ssize_t rb = archive_read_data(a, buf, (size_t)size);
                    if (rb >= 0) {
                        buf[rb] = '\0';
                        *out_meta = json_tokener_parse(buf);
                    }
                    free(buf);
                }
            }
            archive_read_data_skip(a);
            continue;
        }

        // Extract files starting with rootfs/ or ./rootfs/
        const char *sub = NULL;
        if (strncmp(name, "rootfs/", 7) == 0) {
            sub = name + 7;
        } else if (strncmp(name, "./rootfs/", 9) == 0) {
            sub = name + 9;
        }

        if (sub && *sub) {
            char target_path[1024];
            snprintf(target_path, sizeof(target_path), "%s/%s", dest_root, sub);

            archive_entry_set_pathname(entry, target_path);
            archive_write_header(ext, entry);

            const void *buff;
            size_t size;
            la_int64_t offset;
            while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                archive_write_data_block(ext, buff, size, offset);
            }
            archive_write_finish_entry(ext);

            json_object_array_add(*out_files, json_object_new_string(target_path));
        } else {
            archive_read_data_skip(a);
        }
    }

    archive_read_free(a);
    archive_write_free(ext);
    return 0;
}

static int do_install(const char *pkg_name_or_file) {
    ensure_directories();
    char local_file[1024] = {0};

    if (access(pkg_name_or_file, R_OK) == 0) {
        safe_strcpy(local_file, pkg_name_or_file, sizeof(local_file));
    } else {
        // Look up in repo-index.json
        struct json_object *idx = json_object_from_file(DEFAULT_INDEX_FILE);
        if (!idx) {
            fprintf(stderr, "\033[91m[-] No repository index found. Please run 'axis update' first.\033[0m\n");
            return 1;
        }

        struct json_object *pkgs = NULL;
        struct json_object *pkg_info = NULL;
        if (!json_object_object_get_ex(idx, "packages", &pkgs) ||
            !json_object_object_get_ex(pkgs, pkg_name_or_file, &pkg_info)) {
            fprintf(stderr, "\033[91m[-] Package '%s' not found in repository index.\033[0m\n", pkg_name_or_file);
            json_object_put(idx);
            return 1;
        }

        struct json_object *url_obj = NULL;
        struct json_object *sha_obj = NULL;
        struct json_object *fn_obj = NULL;
        json_object_object_get_ex(pkg_info, "url", &url_obj);
        json_object_object_get_ex(pkg_info, "sha256", &sha_obj);
        json_object_object_get_ex(pkg_info, "filename", &fn_obj);

        const char *url_str = url_obj ? json_object_get_string(url_obj) : NULL;
        const char *expected_sha = sha_obj ? json_object_get_string(sha_obj) : NULL;
        const char *fn_str = fn_obj ? json_object_get_string(fn_obj) : "pkg.axis";

        snprintf(local_file, sizeof(local_file), "%s/%s", DEFAULT_CACHE_DIR, fn_str);

        char full_url[1024];
        if (url_str && strstr(url_str, "://")) {
            safe_strcpy(full_url, url_str, sizeof(full_url));
        } else {
            snprintf(full_url, sizeof(full_url), "%s/%s", DEFAULT_REPO_URL, url_str ? url_str : fn_str);
        }

        printf("\033[94m[+] Downloading %s ...\033[0m\n", full_url);
        if (download_file(full_url, local_file) != 0) {
            fprintf(stderr, "\033[91m[-] Download failed.\033[0m\n");
            json_object_put(idx);
            return 1;
        }

        if (expected_sha && *expected_sha) {
            char computed_sha[65] = {0};
            compute_file_sha256(local_file, computed_sha);
            if (strcasecmp(expected_sha, computed_sha) != 0) {
                fprintf(stderr, "\033[91m[-] Checksum verification failed!\nExpected: %s\nGot:      %s\033[0m\n",
                        expected_sha, computed_sha);
                unlink(local_file);
                json_object_put(idx);
                return 1;
            }
            printf("\033[92m[✓] SHA-256 Checksum verified.\033[0m\n");
        }
        json_object_put(idx);
    }

    printf("\033[94m[+] Extracting package files into %s ...\033[0m\n", g_root_dir);
    struct json_object *meta = NULL;
    struct json_object *files_arr = NULL;

    if (extract_package_archive(local_file, g_root_dir, &meta, &files_arr) != 0 || !meta) {
        fprintf(stderr, "\033[91m[-] Failed to extract archive or read metadata.json\033[0m\n");
        if (meta) json_object_put(meta);
        if (files_arr) json_object_put(files_arr);
        return 1;
    }

    struct json_object *name_obj = NULL;
    struct json_object *ver_obj = NULL;
    json_object_object_get_ex(meta, "name", &name_obj);
    json_object_object_get_ex(meta, "version", &ver_obj);

    const char *p_name = name_obj ? json_object_get_string(name_obj) : pkg_name_or_file;
    const char *p_ver = ver_obj ? json_object_get_string(ver_obj) : "1.0.0";

    // Update installed.json
    struct json_object *db = load_installed_db();
    struct json_object *pkgs = NULL;
    if (!json_object_object_get_ex(db, "packages", &pkgs)) {
        pkgs = json_object_new_object();
        json_object_object_add(db, "packages", pkgs);
    }

    json_object_object_add(meta, "installed_files", files_arr);
    json_object_object_add(pkgs, p_name, meta);
    save_installed_db(db);
    json_object_put(db);

    printf("\033[92m[✓] Successfully installed %s (%s)\033[0m\n", p_name, p_ver);
    return 0;
}

static int do_remove(const char *pkg_name) {
    struct json_object *db = load_installed_db();
    struct json_object *pkgs = NULL;
    struct json_object *pkg_info = NULL;

    if (!json_object_object_get_ex(db, "packages", &pkgs) ||
        !json_object_object_get_ex(pkgs, pkg_name, &pkg_info)) {
        fprintf(stderr, "\033[91m[-] Package '%s' is not installed.\033[0m\n", pkg_name);
        json_object_put(db);
        return 1;
    }

    printf("\033[94m[+] Removing %s ...\033[0m\n", pkg_name);
    struct json_object *files_arr = NULL;
    if (json_object_object_get_ex(pkg_info, "installed_files", &files_arr)) {
        int n = json_object_array_length(files_arr);
        for (int i = n - 1; i >= 0; --i) {
            struct json_object *item = json_object_array_get_idx(files_arr, i);
            const char *path = json_object_get_string(item);
            if (path && access(path, F_OK) == 0) {
                unlink(path);
                rmdir(path); // in case it was a directory
            }
        }
    }

    json_object_object_del(pkgs, pkg_name);
    save_installed_db(db);
    json_object_put(db);

    printf("\033[92m[✓] Package %s removed successfully.\033[0m\n", pkg_name);
    return 0;
}

static int do_list(void) {
    struct json_object *db = load_installed_db();
    struct json_object *pkgs = NULL;
    if (!json_object_object_get_ex(db, "packages", &pkgs) || json_object_object_length(pkgs) == 0) {
        printf("No .axis packages currently installed.\n");
        json_object_put(db);
        return 0;
    }

    printf("\033[1mInstalled .axis packages:\033[0m\n");
    json_object_object_foreach(pkgs, key, val) {
        struct json_object *ver = NULL;
        struct json_object *desc = NULL;
        json_object_object_get_ex(val, "version", &ver);
        json_object_object_get_ex(val, "description", &desc);
        printf("  \033[92m%-20s\033[0m %-10s  %s\n",
               key,
               ver ? json_object_get_string(ver) : "",
               desc ? json_object_get_string(desc) : "");
    }
    json_object_put(db);
    return 0;
}

static int do_search(const char *query) {
    struct json_object *idx = json_object_from_file(DEFAULT_INDEX_FILE);
    if (!idx) {
        printf("No repository index found. Run 'axis update' first.\n");
        return 1;
    }

    struct json_object *pkgs = NULL;
    if (!json_object_object_get_ex(idx, "packages", &pkgs)) {
        printf("No packages found in index.\n");
        json_object_put(idx);
        return 0;
    }

    printf("\033[1mSearch results for '%s':\033[0m\n", query);
    int matches = 0;
    json_object_object_foreach(pkgs, key, val) {
        struct json_object *desc_obj = NULL;
        json_object_object_get_ex(val, "description", &desc_obj);
        const char *desc = desc_obj ? json_object_get_string(desc_obj) : "";

        if (strcasestr(key, query) || strcasestr(desc, query)) {
            struct json_object *ver_obj = NULL;
            json_object_object_get_ex(val, "version", &ver_obj);
            printf("  \033[94m%-20s\033[0m %-10s  %s\n",
                   key,
                   ver_obj ? json_object_get_string(ver_obj) : "",
                   desc);
            matches++;
        }
    }

    if (matches == 0) {
        printf("  No matching packages found.\n");
    }
    json_object_put(idx);
    return 0;
}

static void print_usage(const char *prog) {
    printf("AxisOS Package Manager (`axis`) - Pure C11 Edition\n");
    printf("Usage: %s <command> [arguments]\n\n", prog);
    printf("Commands:\n");
    printf("  update             Refresh package indexes from remote repository\n");
    printf("  install <pkg>      Install a remote package or local .axis archive\n");
    printf("  remove <pkg>       Remove an installed package and its files\n");
    printf("  list               List all installed .axis packages\n");
    printf("  search <query>     Search available packages in repository\n");
    printf("  version            Display version information\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "update") == 0) {
        return do_update();
    } else if (strcmp(cmd, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'install' requires a package name or file path.\n");
            return 1;
        }
        return do_install(argv[2]);
    } else if (strcmp(cmd, "remove") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'remove' requires a package name.\n");
            return 1;
        }
        return do_remove(argv[2]);
    } else if (strcmp(cmd, "list") == 0) {
        return do_list();
    } else if (strcmp(cmd, "search") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'search' requires a search term.\n");
            return 1;
        }
        return do_search(argv[2]);
    } else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        printf("axis (AxisOS Package Manager) 2.0.0 (Pure C11)\n");
        return 0;
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
}
