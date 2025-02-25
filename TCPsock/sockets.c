#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define HOST "www.yahoo.com"
#define PORT "80"

struct my_addrinfo {
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;
};

int main() {
    int sockfd;
    struct my_addrinfo addr_info;
    char buf[2048]; // Buffer to store server response
    char *request = "GET / HTTP/1.1\r\n"
                    "Host: www.yahoo.com\r\n"
                    "Connection: close\r\n\r\n";

    // Setting up hints for `getaddrinfo`
    memset(&addr_info.hints, 0, sizeof(addr_info.hints));
    addr_info.hints.ai_family = AF_UNSPEC;      // Support IPv4 and IPv6
    addr_info.hints.ai_socktype = SOCK_STREAM; // TCP uses SOCK_STREAM 

    // resolving hostname
    if (getaddrinfo(HOST, PORT, &addr_info.hints, &addr_info.res) != 0) {
        perror("getaddrinfo() error"); // instead of using the perror function , we can use an if condtition or smtg but this is simpler , looks more cool
        return -1;
    }

    // creating and connecting the socket
    for (addr_info.p = addr_info.res; addr_info.p != NULL; addr_info.p = addr_info.p->ai_next) {
        sockfd = socket(addr_info.p->ai_family, addr_info.p->ai_socktype, addr_info.p->ai_protocol);
        if (sockfd < 0) continue; // Try next address on failure
        if (connect(sockfd, addr_info.p->ai_addr, addr_info.p->ai_addrlen) == 0) break; // Success
        close(sockfd); // Close failed socket
    }

    if (addr_info.p == NULL) { // No valid address was found
        fprintf(stderr, "Unable to connect to the server\n");
        freeaddrinfo(addr_info.res); // Free memory
        return -1;
    }

    freeaddrinfo(addr_info.res); // Free addrinfo results once connected

    // sending a GET request (HTTP)
    if (write(sockfd, request, strlen(request)) < 0) {
        perror("write() error");
        close(sockfd);
        return -1;
    }

    // reading the server's response 
    memset(buf, 0, sizeof(buf)); // Clear buffer
    ssize_t bytes_read = read(sockfd, buf, sizeof(buf) - 1); // Read server response
    if (bytes_read < 0) {
        perror("read() error");
        close(sockfd);
        return -1;
    }
    buf[bytes_read] = '\0'; // Null-terminate the response

    // parsing a response 
    char *headers_end = strstr(buf, "\r\n\r\n"); // Locate the end of the headers
    if (headers_end != NULL) {
        *headers_end = '\0'; // Split headers from body
        char *body = headers_end + 4; // Body starts after "\r\n\r\n"

        printf("\n--- HTTP Headers ---\n%s\n", buf);
        printf("\n--- HTTP Body ---\n%s\n", body);
    } else {
        printf("\n--- Full Response (No Headers Detected) ---\n%s\n", buf);
    }

    // closing the socket 
    close(sockfd);
    return 0;
}
