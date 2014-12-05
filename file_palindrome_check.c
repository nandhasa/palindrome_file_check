#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "file_request.h"

#define IP_LEN 16

/* global variables */
struct file_holder {
	char file_name[FILE_NAME_SIZE];
#if DEBUG
	char str_palindrome[MAX_LEN];
#endif
	int max_len;
} f_holder[WORKER_THREADS];

HTTP_CURL_REQ http_curl_req[WORKER_THREADS];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mf_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *fp;
char ip_address[IP_LEN];

void store_longest_palindrome(char *str, int index, char *file_name) {

	int len;
	if(!check_palindrome(str))
		return;

	len = strlen(str);
	pthread_mutex_lock( &mutex1 );
	if(f_holder[index].max_len < len) {
#if DEBUG
		strcpy(f_holder[index].str_palindrome, str);
#endif
		strcpy(f_holder[index].file_name, file_name);
		f_holder[index].max_len = len;
	}
	pthread_mutex_unlock( &mutex1 );
}

int check_palindrome(char *str) {
	int len = strlen(str);
	int check_len = len / 2;
	int i;
	for(i=0; i < check_len; i++) {
		if(str[i] != str[len - i - 1])
			break;
	}
	return (i == check_len);
}

/* Initialise the file holder variables */
void file_holder_intialize(char *file_list_name) {
	int i;

	pthread_mutex_lock( &mf_mutex );
	fp = fopen(file_list_name, "r");
	if(fp == NULL) {
		fprintf(stderr, "Unable to open %s file\n", file_list_name);
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock( &mf_mutex );

	for(i = 0; i < WORKER_THREADS; i++) {
		f_holder[i].max_len = 0;
		http_curl_req[i].multi_handle = NULL;
		http_curl_req[i].handle = NULL;

	}

}

/* parse the files names from the <file> which has list of files */
int assign_files_to_workers(char *file_name) {
	int ret = -1;
	int len;
	pthread_mutex_lock( &mf_mutex );
	if((fgets(file_name, FILE_NAME_SIZE, fp) != NULL)) {
		file_name[strlen(file_name) - 1] = '\0';
		ret = 0;
	}
	pthread_mutex_unlock( &mf_mutex );
	return ret;
}

#if defined(LOCAL_FILE)
void *pick_file_to_process(void *value) {
	FILE *fp;
	char *file_name = malloc(FILE_NAME_SIZE);
	int c;
	int count;
	char palindrome[MAX_LEN];
	int *ival = (int *)value;

	while(1) {
		if(assign_files_to_workers(file_name) != 0) {
			printf("Thread id Exited %u\n", (unsigned int)pthread_self());
			break;
		}

		fp = fopen(file_name, "r");
		if(fp == NULL) {
			fprintf(stderr, "Unable to open %s file\n", file_name);
			exit(EXIT_FAILURE);
		}

		count = 0;
		while((c = fgetc(fp))) {
			palindrome[count++] = c;
			switch(c) {
				case ' ':
				case '\n':
				case '\t':
				case EOF:
					palindrome[count - 1] = '\0';
					store_longest_palindrome(palindrome, *ival, file_name);
					count = 0;
			}
			if(c == EOF)
				goto exit_reading;
		}
exit_reading:
		fclose(fp);

	}
	free(file_name);
}
#else 
void *pick_file_to_process(void *value) {
	char *http_url = malloc(1024);
	char *file_name = malloc(FILE_NAME_SIZE);
	int *ival = (int *)value;
	unsigned int tid = pthread_self();

#if DEBUG
	printf("Thread id invoked %u -- %d\n", tid, *ival);
#endif
	while(1) {
		if(assign_files_to_workers(file_name) != 0) {
#if DEBUG
			printf("Thread id Exited %u\n", tid);
#endif
			break;
		}
		sprintf(http_url, "%s%s/%s", "http://", ip_address, file_name);
#if DEBUG
		printf("Thread %d File processing %s starts\n", *ival, http_url);
#endif
		parse_file_request(&http_curl_req[*ival], http_url, *ival, file_name);
	}
	free(file_name);
}

#endif

void print_result() {
	int index;
	int selected = 0;
	int len = 0;

	for(index = 0; index < WORKER_THREADS; index++) {
		if(len < f_holder[index].max_len) { 
			selected = index;
			len = f_holder[index].max_len;	
		}
	}
#if DEBUG
	printf("String : %s\n", f_holder[selected].str_palindrome);
#endif
	printf("File Name: %s\tLength: %d\n", f_holder[selected].file_name, len);
}

int main(int argc, char *argv[]) {
	int iret1;
	int i, c;
	int tid[WORKER_THREADS];
	pthread_t thread[WORKER_THREADS];
	char file_list_name[FILE_NAME_SIZE];
	int setip = 0;
	int setfile = 0;

	while ((c = getopt (argc, argv, "d:f:")) != -1) {
		switch (c)
		{
			case 'd':
				strcpy(ip_address, optarg);
				setip = 1;
				break;
			case 'f':
				strcpy(file_list_name, optarg);
				setfile = 1;
				break;
			case '?':
				printf("%s -d <ip> -f <file list>\n", argv[0]);
				return 1;
			default:
				break;
		}
	}
	if(!setip || !setfile) {
		printf("%s -d <ip> -f <file list>\n", argv[0]);
		return -1;
	}

	file_holder_intialize(file_list_name);

	for(i = 0; i < WORKER_THREADS; i++) {
		tid[i] = i;
		iret1 = pthread_create( &thread[i], NULL, &pick_file_to_process, (void*) &tid[i]);
		if(iret1)
		{
			fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
			exit(EXIT_FAILURE);
		}
	}

	for(i = 0; i < WORKER_THREADS; i++) {
		pthread_join(thread[i], NULL);
	}
	
	print_result();
}
