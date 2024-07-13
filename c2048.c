#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>  // For sockaddr_in
#include <signal.h>     // For signal handling
#include <unistd.h>     // For pause

// Function pointers for the shared library
void (*setStrategy)(const char *);
void (*setUnzip)(int);
void (*setMaxTime)(int);
void (*setMaxDepth)(int);
void (*setEvalMultithreading)(int);
int (*loadTuplesDescriptor)(void);
void (*initializeTables)(void);
void (*initializeEvaluator)(void);
uint64_t (*bestAction)(uint64_t);

// Mutex for thread safety
pthread_mutex_t lock;

// Signal handler to catch termination signals
void handle_signal(int signal) {
    // Do nothing, just return to allow the process to terminate
}

// Initialize the AI library
void init_ailib() {
    void *handle = dlopen("./lib/libWebApi.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading shared library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    setStrategy =           dlsym(handle, "setStrategy");
    setUnzip =              dlsym(handle, "setUnzip");
    setMaxTime =            dlsym(handle, "setMaxTime");
    setMaxDepth =           dlsym(handle, "setMaxDepth");
    setEvalMultithreading = dlsym(handle, "setEvalMultithreading");
    loadTuplesDescriptor =  dlsym(handle, "loadTuplesDescriptor");
    initializeTables =      dlsym(handle, "initializeTables");
    initializeEvaluator =   dlsym(handle, "initializeEvaluator");
    bestAction =            dlsym(handle, "bestAction");

    char *error;
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "Error loading symbols: %s\n", error);
        exit(EXIT_FAILURE);
    }

    setStrategy("./data/eval-function.bin.special");
    setUnzip(1);
    setMaxTime(20);
    setMaxDepth(8);
    setEvalMultithreading(1);

    if (!loadTuplesDescriptor()) {
        fprintf(stderr, "Failed to load tuples descriptor\n");
        exit(EXIT_FAILURE);
    }

    initializeTables();
    initializeEvaluator();
    fprintf(stdout, "AI library initialized successfully.\n");
}

// Read the contents of a file into a string
char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(length + 1);
    if (!content) {
        fprintf(stderr, "Could not allocate memory for file content\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(content, 1, length, file);
    content[length] = '\0';

    fclose(file);
    return content;
}

// Handle HTTP requests
static enum MHD_Result request_handler(
        void *cls, struct MHD_Connection *connection, const char *url, const char *method,
        const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls
    ) {
    if (strcmp(method, "OPTIONS") == 0) {
        // Handle CORS preflight requests
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS, "GET, POST, OPTIONS");
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS, "Content-Type");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    const char *response_str;
    struct MHD_Response *response;
    int ret;

    if (strcmp(url, "/") == 0 || strcmp(url, "/page_not_found") == 0) {
        response_str = "404 Error: Page Not Found";
        response = MHD_create_response_from_buffer(strlen(response_str), (void *)response_str,  MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");
        ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    } else if (strncmp(url, "/2048/", 6) == 0) {
        const char *board_str = url + 6;
        uint64_t board;
        if (sscanf(board_str, "%lx", &board) != 1) {
            response_str = "400 Error: Invalid Input";
            response = MHD_create_response_from_buffer(strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
            MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");
            ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        } else {
            pthread_mutex_lock(&lock);
            uint64_t move = bestAction(board);
            pthread_mutex_unlock(&lock);
            
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "{\"move\": \"%lu\"}", move);
            response = MHD_create_response_from_buffer(strlen(buffer), (void *)buffer, MHD_RESPMEM_MUST_COPY);
            MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
            MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        }
    } else {
        response_str = "404 Error: Page Not Found";
        response = MHD_create_response_from_buffer(strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");
        ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    }

    MHD_destroy_response(response);
    return ret;
}

int main() {
    // Set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    init_ailib();
    pthread_mutex_init(&lock, NULL);

    // Specify the address to bind to (0.0.0.0 means bind to all interfaces)
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5555);
    inet_pton(AF_INET, "0.0.0.0", &serv_addr.sin_addr); // Use "0.0.0.0" for all interfaces

    // SSL certificate and key files
    const char *cert_file_path = "cert.pem";
    const char *key_file_path = "privkey.pem";

    // Read SSL certificate and key files
    char *cert_file = read_file(cert_file_path);
    char *key_file = read_file(key_file_path);

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL, 5555, 
        NULL, NULL, &request_handler, NULL,
        MHD_OPTION_HTTPS_MEM_KEY, key_file,
        MHD_OPTION_HTTPS_MEM_CERT, cert_file,
        MHD_OPTION_SOCK_ADDR, (struct sockaddr *)&serv_addr,
        MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "Failed to start HTTP server\n");
        free(cert_file);
        free(key_file);
        return 1;
    }

    fprintf(stdout, "Server is running on https://localhost:5555\n");

    // Keep the server running indefinitely
    pause();

    MHD_stop_daemon(daemon);
    pthread_mutex_destroy(&lock);

    free(cert_file);
    free(key_file);

    return 0;
}
