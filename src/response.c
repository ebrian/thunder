#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "request.h"
#include "response.h"

bs_response *bs_init_response(void) {
    bs_response *response = malloc(sizeof(bs_response));

    if (response == NULL) {
        bserve_fatal("memory allocation error");
    }

    response->body_len = 0;
    response->body = NULL;

    return response;
}

void bs_dealloc_response(bs_response *response) {
    if (response->body != NULL) free(response->body);
    free(response);
}

char *init_header_string(int code) {
    char *http = "HTTP/1.1";

    char buffer[32];
    if (code == STATUS_SUCCESS_OK)
        strcpy(buffer, "OK");

    int res_strlen = strlen(http) + 1 + 3 + 1 + strlen(buffer) + 3;

    char *response_string = malloc(res_strlen * sizeof(char));
    snprintf(response_string, res_strlen, "%s %d %s\r\n", http, code, buffer);

    return response_string;
}

void append_txt_header(char **res_string, const char *header, const char *value) {
    int new_len = strlen(*res_string) + strlen(header) + 2 + strlen(value) + 2 + 1;

    *res_string = realloc(*res_string, new_len);

    strcat(*res_string, header);
    strcat(*res_string, ": ");
    strcat(*res_string, value);
    strcat(*res_string, "\r\n");
}

void append_int_header(char **res_string, const char *header, int value) {
    char buf[16];
    sprintf(buf, "%d", value);

    append_txt_header(res_string, header, buf);
}

void terminate_headers(char **res_string) {
    int new_len = strlen(*res_string) + 2 + 1;

    *res_string = realloc(*res_string, new_len);

    strcat(*res_string, "\r\n");
}

void transmit_data(int fd, const char *data, int size) {
    int total_bytes_written = 0;

    while (size > 0) {
        int bytes_written = write(fd, data + total_bytes_written, size);

        if (bytes_written < 0)
            bserve_fatal("failed to write to socket");

        total_bytes_written += bytes_written;
        size -= bytes_written;
    }
}

void bs_send_response_200(bs_request *request, bs_response *response) {
    char *headers = init_header_string(STATUS_SUCCESS_OK);

    append_int_header(&headers, "Content-Length", response->body_len);

    if (strstr(request->path, ".html")) {
        append_txt_header(&headers, "Content-Type", "text/html");
    } else if (strstr(request->path, ".js")) {
        append_txt_header(&headers, "Content-Type", "application/javascript");
    } else if (strstr(request->path, ".css")) {
        append_txt_header(&headers, "Content-Type", "text/css");
    } else if (strstr(request->path, ".jpg") || strstr(request->path, ".jpeg")) {
        append_txt_header(&headers, "Content-Type", "image/jpeg");
    } else if (strstr(request->path, ".png")) {
        append_txt_header(&headers, "Content-Type", "text/png");
    } else {
        append_txt_header(&headers, "Content-Type", "application/octet-stream");
    }

    terminate_headers(&headers);

    transmit_data(response->socket, headers, strlen(headers));
    transmit_data(response->socket, response->body, response->body_len);
}
