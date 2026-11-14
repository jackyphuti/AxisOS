#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <archive.h>
#include <archive_entry.h>
#include <openssl/evp.h>
#include <json-c/json.h>

static int compute_sha256(const char *path, char *out_hex) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) { fclose(f); return -1; }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        fclose(f);
        return -1;
    }

    unsigned char buf[65536];
    size_t bytes;
    while ((bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        EVP_DigestUpdate(mdctx, buf, bytes);
    }
    fclose(f);

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    for (unsigned int i = 0; i < md_len; ++i) {
        sprintf(out_hex + (i * 2), "%02x", md_value[i]);
    }
    out_hex[md_len * 2] = '\0';
    return 0;
}

static char *extract_metadata_from_axis(const char *axis_path) {
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, axis_path, 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        return NULL;
    }

    struct archive_entry *entry;
    char *result = NULL;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *name = archive_entry_pathname(entry);
        if (strcmp(name, "metadata.json") == 0 || strcmp(name, "./metadata.json") == 0) {
            la_int64_t size = archive_entry_size(entry);
            if (size > 0 && size < 10485760) { // max 10MB metadata
                result = malloc((size_t)size + 1);
                if (result) {
                    la_ssize_t read_bytes = archive_read_data(a, result, (size_t)size);
                    if (read_bytes >= 0) {
                        result[read_bytes] = '\0';
                    } else {
                        free(result);
                        result = NULL;
                    }
                }
            }
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_free(a);
    return result;
}

static void scan_directory(const char *dir_path, const char *base_url, struct json_object *packages_obj) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_directory(full_path, base_url, packages_obj);
        } else if (S_ISREG(st.st_mode)) {
            size_t len = strlen(ent->d_name);
            if (len > 5 && strcmp(ent->d_name + len - 5, ".axis") == 0) {
                printf("[+] Found package: %s\n", full_path);

                char sha256_hex[65] = {0};
                if (compute_sha256(full_path, sha256_hex) != 0) {
                    fprintf(stderr, "[-] Warning: Failed to compute SHA256 for %s\n", full_path);
                    continue;
                }

                char *meta_json_str = extract_metadata_from_axis(full_path);
                if (!meta_json_str) {
                    fprintf(stderr, "[-] Warning: No valid metadata.json found in %s\n", full_path);
                    continue;
                }

                struct json_object *meta_obj = json_tokener_parse(meta_json_str);
                free(meta_json_str);

                if (!meta_obj) {
                    fprintf(stderr, "[-] Warning: Failed to parse metadata.json in %s\n", full_path);
                    continue;
                }

                // Add size and sha256 to metadata
                json_object_object_add(meta_obj, "sha256", json_object_new_string(sha256_hex));
                json_object_object_add(meta_obj, "size", json_object_new_int64(st.st_size));
                json_object_object_add(meta_obj, "filename", json_object_new_string(ent->d_name));

                char url[1024];
                if (base_url && *base_url) {
                    snprintf(url, sizeof(url), "%s/%s", base_url, ent->d_name);
                } else {
                    snprintf(url, sizeof(url), "%s", ent->d_name);
                }
                json_object_object_add(meta_obj, "url", json_object_new_string(url));

                struct json_object *name_obj = NULL;
                if (json_object_object_get_ex(meta_obj, "name", &name_obj)) {
                    const char *pkg_name = json_object_get_string(name_obj);
                    json_object_object_add(packages_obj, pkg_name, meta_obj);
                    printf("[*] Indexed %s (%s, %ld bytes)\n", pkg_name, sha256_hex, (long)st.st_size);
                } else {
                    json_object_put(meta_obj);
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("AxisOS Repository Builder (Pure C11)\n");
        printf("Usage: %s <packages_dir> [base_url] [output_file]\n", argv[0]);
        return 1;
    }

    const char *packages_dir = argv[1];
    const char *base_url = (argc > 2) ? argv[2] : "";
    const char *output_file = (argc > 3) ? argv[3] : "repo-index.json";

    printf("[+] Scanning for .axis packages in %s ...\n", packages_dir);

    struct json_object *root_obj = json_object_new_object();
    struct json_object *packages_obj = json_object_new_object();

    scan_directory(packages_dir, base_url, packages_obj);

    json_object_object_add(root_obj, "packages", packages_obj);

    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    json_object_object_add(root_obj, "generated_at", json_object_new_string(time_buf));

    if (json_object_to_file_ext(output_file, root_obj, JSON_C_TO_STRING_PRETTY) == 0) {
        printf("[+] Repository index generated: %s\n", output_file);
    } else {
        fprintf(stderr, "[-] Error: Failed to write %s\n", output_file);
        json_object_put(root_obj);
        return 1;
    }

    json_object_put(root_obj);
    return 0;
}
