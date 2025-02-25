I'll format the README for the HTTP Client project with clean Markdown:

# Simple HTTP Client in C

This project demonstrates how to create a simple HTTP client in C using `getaddrinfo()` for domain name resolution and the `socket` API for network communication. It connects to a server (in this case, Yahoo), sends an HTTP GET request, and parses the response to display the headers and the body separately.

## Features

- Resolves domain names to both IPv4 and IPv6 addresses using `getaddrinfo()`.
- Establishes a TCP connection to the resolved IP address.
- Sends an HTTP GET request to the server.
- Receives and parses the HTTP response to separate headers from the body.
- Compatible with both IPv4 and IPv6.

## Requirements

- A Linux or Unix-like operating system.
- GCC or any compatible C compiler.
- Networking libraries (`<sys/socket.h>`, `<netdb.h>`, `<arpa/inet.h>`).

## How to Compile

Use the following command to compile the program:

```bash
gcc -o http_client http_client.c
```

## How to Run

Run the compiled program as follows:

```bash
./http_client
```

## Output Example

If the program runs successfully, you will see output like this:

### HTTP Headers:
```
HTTP/1.1 200 OK
Date: Fri, 01 Dec 2024 10:00:00 GMT
Server: ATS
Content-Length: 12345
Content-Type: text/html
Connection: close
```

### HTTP Body:
```html
<!DOCTYPE html>
<html>
<head>
<title>Yahoo</title>
</head>
<body>
<h1>Welcome to Yahoo!</h1>
</body>
</html>
```

## Key Functions Used

1. **`getaddrinfo()`**:
   - Resolves domain names (like `www.yahoo.com`) into usable `struct sockaddr` addresses.
   - Supports both IPv4 and IPv6.

2. **`socket()`**:
   - Creates a socket for communication.

3. **`connect()`**:
   - Connects the socket to the resolved server address.

4. **`write()`**:
   - Sends the HTTP GET request.

5. **`read()`**:
   - Reads the server's response.

6. **`close()`**:
   - Closes the socket after the communication is complete.

## Code Highlights

### Setting up `getaddrinfo`:
```c
memset(&addr_info.hints, 0, sizeof(addr_info.hints));
addr_info.hints.ai_family = AF_UNSPEC; // Support IPv4 and IPv6
addr_info.hints.ai_socktype = SOCK_STREAM; // TCP
if (getaddrinfo(HOST, PORT, &addr_info.hints, &addr_info.res) != 0) {
    perror("getaddrinfo() error");
    return -1;
}
```

### Sending HTTP Request:
```c
char *request = "GET / HTTP/1.1\r\n"
                "Host: www.yahoo.com\r\n"
                "Connection: close\r\n\r\n";
if (write(sockfd, request, strlen(request)) < 0) {
    perror("write() error");
    close(sockfd);
    return -1;
}
```

### Parsing Response:
```c
char *headers_end = strstr(buf, "\r\n\r\n"); // Locate the end of headers
if (headers_end != NULL) {
    *headers_end = '\0'; // Split headers from body
    char *body = headers_end + 4; // Body starts after "\r\n\r\n"
    printf("\n--- HTTP Headers ---\n%s\n", buf);
    printf("\n--- HTTP Body ---\n%s\n", body);
}
```

## Enhancements to Consider

1. **Dynamic Buffering**:
   - Instead of a fixed-size buffer, implement dynamic resizing for larger responses.

2. **Error Handling**:
   - Improve error handling for edge cases like timeouts or malformed responses.

3. **Support for HTTPS**:
   - Use OpenSSL or similar libraries to handle secure connections.

4. **Argument Parsing**:
   - Allow the user to specify the host, port, and path as command-line arguments.

## License

This project is open-source and distributed under the MIT License. Feel free to use, modify, and distribute.

## Acknowledgments

This project is a minimal HTTP client designed for learning purposes. It was inspired by foundational networking concepts in C.