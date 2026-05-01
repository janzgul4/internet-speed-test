#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <curl/curl.h>
#include <getopt.h>
#include <cjson/cJSON.h>

#define TEST_DURATION 15

double start_time;

// ---------- TIME ----------
double get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1e6;
}

// ---------- DISCARD ----------
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

// ---------- BUFFER ----------
size_t buffer_write(void *ptr, size_t size, size_t nmemb, void *userdata) {
    strncat((char*)userdata, (char*)ptr,
            (size * nmemb > 4095 ? 4095 : size * nmemb));
    return size * nmemb;
}

// ---------- LOCATION ----------
char* get_location() {
    CURL *curl = curl_easy_init();
    if (!curl) return strdup("Unknown");

    char buffer[4096] = {0};

    curl_easy_setopt(curl, CURLOPT_URL, "http://ip-api.com/json");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    cJSON *json = cJSON_Parse(buffer);
    if (!json) return strdup("Unknown");

    cJSON *country = cJSON_GetObjectItem(json, "country");
    char *res = strdup(country ? country->valuestring : "Unknown");

    cJSON_Delete(json);
    return res;
}

// ---------- SERVER ----------
char* get_best_server() {
    FILE *f = fopen("speedtest_server_list.json", "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *data = malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);

    cJSON *json = cJSON_Parse(data);
    free(data);

    if (!json) return NULL;

    int count = cJSON_GetArraySize(json);
    if (count > 6) count = 6;

    char *best = NULL;

    printf("Selecting best server...\n");

    for (int i = 0; i < count; i++) {
        printf("Testing server %d...\n", i + 1);

        cJSON *srv = cJSON_GetArrayItem(json, i);
        cJSON *host = cJSON_GetObjectItem(srv, "host");

        if (!host) continue;

        char url[512];
        snprintf(url, sizeof(url),
                 "http://%s/speedtest/random4000x4000.jpg",
                 host->valuestring);

        CURL *curl = curl_easy_init();
        if (!curl) continue;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_RANGE, "0-2048");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            best = strdup(url);
            break;
        }
    }

    cJSON_Delete(json);

    if (!best) return NULL;

    printf("Server selected\n");
    return best;
}

// ---------- DOWNLOAD (FINAL FIXED VERSION) ----------
void download_test(char *server_url) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    double total_bytes = 0;

    printf("\nStarting download (15 sec)...\n");

    start_time = get_time();

    while ((get_time() - start_time) < TEST_DURATION) {

        // ✅ Reliable high-speed file
        curl_easy_setopt(curl, CURLOPT_URL,
            "http://ipv4.download.thinkbroadband.com/50MB.zip");

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        // 🔥 FIX: avoid connection reuse problem
        curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("Download error: %s\n", curl_easy_strerror(res));
            break;
        }

        curl_off_t bytes = 0;
        curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &bytes);

        total_bytes += bytes;
    }

    double end = get_time();

    double speed = (total_bytes * 8.0) /
                   (end - start_time) / (1024 * 1024);

    printf("Download Speed: %.2f Mbps\n", speed);

    curl_easy_cleanup(curl);
}

// ---------- UPLOAD ----------
void upload_test(char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char data[1024 * 1024];
    memset(data, 'A', sizeof(data));

    printf("\nStarting upload (15 sec)...\n");

    start_time = get_time();
    double total = 0;

    while ((get_time() - start_time) < TEST_DURATION) {

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, sizeof(data));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        if (curl_easy_perform(curl) != CURLE_OK) break;

        total += sizeof(data);
    }

    double end = get_time();

    double speed = (total * 8.0) /
                   (end - start_time) / (1024 * 1024);

    printf("Upload Speed: %.2f Mbps\n", speed);

    curl_easy_cleanup(curl);
}

// ---------- MAIN ----------
int main(int argc, char *argv[]) {

    curl_global_init(CURL_GLOBAL_ALL);

    int d = 0, u = 0, a = 0;

    static struct option opts[] = {
        {"download", 0, 0, 'd'},
        {"upload", 0, 0, 'u'},
        {"auto", 0, 0, 'a'},
        {0,0,0,0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "dua", opts, NULL)) != -1) {
        if (c == 'd') d = 1;
        else if (c == 'u') u = 1;
        else if (c == 'a') a = 1;
    }

    char *loc = get_location();
    char *srv = get_best_server();

    if (!srv) {
        printf("Server selection failed\n");
        return 1;
    }

    printf("\nLocation: %s\n", loc);
    printf("Server: %s\n", srv);

    if (a) {
        download_test(srv);
        upload_test("https://httpbin.org/post");
    } else {
        if (d) download_test(srv);
        if (u) upload_test("https://httpbin.org/post");
    }

    free(loc);
    free(srv);

    curl_global_cleanup();
    return 0;
}
