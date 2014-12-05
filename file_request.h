#include <curl/curl.h>
#include <string.h>

#define FILE_NAME_SIZE 256
#define WORKER_THREADS 50
#define MAX_LEN 2046

enum fcurl_type_e {
        CFTYPE_NONE=0,
        CFTYPE_FILE=1,
        CFTYPE_CURL=2
};

struct fcurl_data
{
        enum fcurl_type_e type;     /* type of handle */
        union {
                CURL *curl;
                FILE *file;
        } handle;                   /* handle */

        char *buffer;               /* buffer to store cached data*/
        size_t buffer_len;          /* currently allocated buffers length */
        size_t buffer_pos;          /* end of data in buffer*/
        int still_running;          /* Is background url fetch still in progress */
};

typedef struct fcurl_data URL_FILE;

struct _http_curl_req {
	CURLM *multi_handle;
	URL_FILE *handle;
};

typedef struct _http_curl_req HTTP_CURL_REQ;

/*  exported functions */
URL_FILE *url_fopen(CURLM **multi_handle, const char *url,const char *operation);
int url_fclose(CURLM *multi_handle, URL_FILE *file);
int url_feof(URL_FILE *file);
size_t url_fread(CURLM *multi_handle, void *ptr, size_t size, size_t nmemb, URL_FILE *file);
char * url_fgets(CURLM *multi_handle, char *ptr, size_t size, URL_FILE *file);
void url_rewind(CURLM *multi_handle, URL_FILE *file);

/*  we use a global one for convenience */
extern HTTP_CURL_REQ http_curl_req[WORKER_THREADS];

int parse_file_request(HTTP_CURL_REQ *http_curl_req, char *url, int ival, char *file_name);

