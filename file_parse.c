#include "file_request.h"

int parse_file_request(HTTP_CURL_REQ *http_curl_req, char *url, int ival, char *file_name)
{
	CURLM *multi_handle = http_curl_req->multi_handle;
        URL_FILE *handle = http_curl_req->handle;

        FILE *outf;

        int nread, space_found, index, count;
        char buffer[MAX_LEN];
        char workbuffer[2 * MAX_LEN - 1];
	char palindrome[MAX_LEN];

        /* Test rewind */
#if DEBUG
	char output_file[50];
	sprintf(output_file, "%s%d", "rewind.test", ival);

        outf=fopen(output_file,"w+");
        if(!outf) {
                perror("couldn't open fread output file\n");
                return 1;
        }
#endif

        handle = url_fopen(&multi_handle, url, "r");
        if(!handle) {
                printf("couldn't url_fopen() %s\n", url);
                fclose(outf);
                return 2;
        }
        nread = 1;

        memset(workbuffer, '\0', MAX_LEN);
        index = 0;
        while(nread) {
                nread = url_fread(multi_handle, buffer, 1,sizeof(buffer), handle);
                space_found = nread - 1;
                if(space_found <= 0) {
                        break;
                }
                if(nread < MAX_LEN) {
                        space_found = nread;
                        if(buffer[nread - 1] == ' ' || buffer[nread - 1] == '\n') {
                                buffer[nread - 1] = '\0';
                        }
                        goto set_buffer;
                }

                /* Check for the space from END to FIRST, if there is space found then
                 * return the index of it */
                while(space_found != 0) {
                        if(buffer[space_found] == ' ' || buffer[space_found] == '\n') {
                                break;
                        }
                        space_found--;
                }
set_buffer:
                memcpy(workbuffer + index, buffer, space_found);
		count = 0;
		index = 0;
		while(index <= space_found) {
			palindrome[count] = workbuffer[index];
			switch(palindrome[count]) {
				case ' ':
				case '\n':
				case '\t':
					palindrome[count] = '\0';
#if DEBUG
					printf("string %d %s\n", count, palindrome);
#endif
					store_longest_palindrome(palindrome, ival, file_name);
					count = -1;
			}
			count++;
			index++;
		}
#if DEBUG
                printf(" * * * * * workbuffer=%s index=%d space_found=%d nread=%d * * * * *\n", workbuffer, index, space_found, nread);
                fwrite(workbuffer,1,nread,outf);
#endif

                if(nread < MAX_LEN)
                        break;

                index = nread - space_found;
                memcpy(workbuffer, buffer + space_found, nread - space_found);
#if DEBUG
                workbuffer[nread - space_found] = '\0';
                printf(" PENDING * * * %s %d %d %d * * * * *\n", workbuffer, index, space_found, nread);
#endif
        }

        url_fclose(multi_handle, handle);
#if DEBUG
        fclose(outf);
#endif

        return 0;/* all done */
}

