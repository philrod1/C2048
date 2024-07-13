# 2048 AI Server

This project is a 2048 AI server implemented in C. It uses `libmicrohttpd` for the HTTP server functionality and supports HTTPS connections. The AI logic is implemented in a shared library, which is dynamically loaded at runtime.

The home of the original AI project is here: -

https://github.com/aszczepanski/2048

## Features

- HTTP and HTTPS support
- CORS handling for cross-origin requests
- Multi-threaded request handling
- Signal handling for graceful shutdown
- Systemd service integration for starting on boot

## Prerequisites

- GCC
- `libmicrohttpd` development package
- `libpthread` development package
- `dlfcn` for dynamic library loading
- Systemd (for service integration)

## Setup

### Installation

1. **Clone the repository:**

    ```sh
    git clone https://github.com/philrod1/2048-ai-server.git
    cd 2048-ai-server
    ```

2. **Install dependencies:**

    On Ubuntu or Debian-based systems:

    ```sh
    sudo apt-get update
    sudo apt-get install -y build-essential libmicrohttpd-dev
    ```

3. **Build the project:**

    ```sh
    make
    ```

4. **Install the service:**

    ```sh
    sudo make install
    ```

5. **Start the service:**

    ```sh
    sudo systemctl start 2048server
    ```

6. **Enable the service to start on boot:**

    ```sh
    sudo systemctl enable 2048server
    ```

### Configuration

The following files are required and should be placed in the appropriate directories:

- `cert.pem`: SSL certificate file
- `privkey.pem`: SSL private key file
- `lib/libWebApi.so`: Shared library for the AI logic
- `data/eval-function.bin.special`: Binary file required by the AI logic

### Directory Structure

.
├── cert.pem
├── privkey.pem
├── lib
│ └── libWebApi.so
├── data
│ └── eval-function.bin.special
├── c2048.c
├── Makefile
└── README.md

### Logs

To check the status of the service and view logs:

```sh
sudo systemctl status 2048server
sudo journalctl -u 2048server
```

### Usage
Once the server is running, you can send HTTP requests to it. Here is an example request:

```sh
Copy code
curl -k https://HOSTNAME:5555/2048/YOUR_BOARD_STATE
```

Replace YOUR_BOARD_STATE with the hexadecimal representation of the board state.

## Contributing
1. Fork the repository if you like
2. Push any changes
3. Create a pull request

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments
* The libmicrohttpd library for HTTP server functionality
* The creators of the original 2048 game
* Wojciech Jaśkowski for the AI.  Read the paper here: https://ieeexplore.ieee.org/document/7814202 or here: https://arxiv.org/pdf/1604.05085.pdf
