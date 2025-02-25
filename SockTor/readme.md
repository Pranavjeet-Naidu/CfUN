
# SockTor: A SOCKS Proxy Wrapper for Sockets

SockTor is a dynamic library that intercepts the `connect()` system call to route TCP connections through a SOCKS4 proxy, such as the Tor network. It uses `dlsym()` to hook into the `connect()` function and facilitates anonymity and security when accessing external servers.

## Features

- **SOCKS4 Protocol Support**: Implements the SOCKS4 protocol for routing connections through a proxy.
- **Dynamic Function Interception**: Intercepts `connect()` using `dlsym()` and overrides its behavior.
- **Custom Proxy Requests**: Constructs and sends SOCKS4 proxy requests based on the destination address and port.
- **Proxy Response Validation**: Validates the response from the proxy server to ensure a successful connection.
- **Lightweight**: Minimal and efficient code design, relying on standard C libraries.

## Project Structure

```
├── Makefile         # Build system
├── SockTor.c        # Implementation of the proxy connection logic
├── SockTor.h        # Header file with struct definitions and function prototypes
└── readme.md        # Project documentation
```

## Requirements

- **Linux or Unix-based system**
- GCC compiler (or any C compiler supporting `dlsym()` and shared libraries)
- Tor or any SOCKS4-compatible proxy running on `127.0.0.1:9050` (default settings for Tor)

## How to Build

Compile the project into a shared library using the provided `Makefile`:

```bash
make
```

This will generate a shared library named `SockTor.so`.

## How to Use

1. **Set the `LD_PRELOAD` environment variable**:
   - Load the `SockTor.so` library before running any program that uses `connect()`:
   ```bash
   LD_PRELOAD=./SockTor.so <your_program>
   ```

2. **Ensure the Proxy is Running**:
   - For Tor, make sure the Tor service is active:
   ```bash
   systemctl start tor
   ```

3. **Connect Through Proxy**:
   - Any program run with `LD_PRELOAD` will automatically route its connections through the SOCKS4 proxy at `127.0.0.1:9050`.

## Code Highlights

### Proxy Request Structure

The SOCKS4 proxy request is defined in `SockTor.h`:

```c
struct proxy_request {
    int8 vn;                  // SOCKS version number
    int8 cd;                  // Command code (1 = CONNECT)
    int16 dstport;            // Destination port
    int32 dstip;              // Destination IP address
    unsigned char username[8];// Proxy username (optional)
};
```

### Dynamic Interception of `connect()`

The `connect()` function is overridden using `dlsym()`:

```c
int (*p)(int, const struct sockaddr*, socklen_t);
p = dlsym(RTLD_NEXT, "connect");
```

This ensures the system call is intercepted, allowing SockTor to route connections through the proxy.

## SOCKS4 Protocol Overview

1. **Client Request**:
   - The client sends a `proxy_request` structure containing:
     - Version (4 for SOCKS4)
     - Command (1 for CONNECT)
     - Destination port and IP
     - Username (if required)

2. **Server Response**:
   - The proxy server responds with a `proxy_response` structure:
     - A success code (`cd = 90`) indicates that the connection was established successfully.

## Makefile

The provided `Makefile` compiles the project into a shared library:

```makefile
all:
	gcc SockTor.c -o SockTor.so -fPIC -shared -ldl -D_GNU_SOURCE
```

To clean up generated files:

```bash
make clean
```

## Example

### Compiling a Program

Suppose you have a program named `client.c` that uses sockets. Compile it as follows:

```bash
gcc client.c -o client
```

### Running the Program Through SockTor

To route the program's connections through the proxy, use:

```bash
LD_PRELOAD=./SockTor.so ./client
```

### Output Example

When the proxy connection is established successfully, you will see:

```
Connected through proxy
Connected through proxy
Connected through proxy
```

If the connection fails, an error message will be displayed:

```
Unable to traverse the proxy, error code: <error_code>
```

## Troubleshooting

1. **"connect: Connection refused"**:
   - Ensure the SOCKS proxy (e.g., Tor) is running and listening on `127.0.0.1:9050`.

2. **"Read error"**:
   - Verify the proxy server is responding to requests and the target server is reachable.

3. **Segmentation Fault**:
   - Ensure the `LD_PRELOAD` library is compiled correctly and matches the architecture of the target program.

## License

This project is licensed under the MIT License. Feel free to use, modify, and distribute.

## Acknowledgments

This project is inspired by the need to route TCP connections through proxies for added privacy and security. The implementation is based on the SOCKS4 protocol and dynamic linking techniques.