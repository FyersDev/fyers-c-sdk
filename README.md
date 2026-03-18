<a href="https://fyers.in/"><img src="https://assets.fyers.in/images/logo.svg" align="right" /></a>

# Fyers API C SDK : fyers-api-c - v1.1.0

The official Fyers C SDK for API-V3 Users [FYERS API](https://fyers.in/products/api/).

Fyers API is a set of REST-like APIs that provide integration with our in-house trading platform with which you can build your own customized trading applications.

## Documentation

- [Fyers API documentation](https://myapi.fyers.in/docsv3)

## Requirements

- CMake 3.15+
- C compiler (C11)
- **Dependencies:** libcurl, cJSON, OpenSSL, libwebsockets

| Package | Linux (apt) | macOS (Homebrew) | Windows (vcpkg) |
|---------|-------------|------------------|-----------------|
| cmake | `cmake` | `cmake` | CMake installer |
| curl | `libcurl4-openssl-dev` | `curl` | `curl` |
| cJSON | `libcjson-dev` | `cjson` | `cjson` |
| OpenSSL | `libssl-dev` | `openssl` | `openssl` |
| libwebsockets | `libwebsockets-dev` | `libwebsockets` | `libwebsockets` |

## Getting the source

Clone the repository (or download and extract the source), then build from that directory:

```bash
git clone <repository-url> fyers-c-sdk
cd fyers-c-sdk
```

Replace `<repository-url>` with the actual repo URL. All build and install steps below are run from this SDK root directory.

## Usage

### Linux (Ubuntu/Debian)

Install dependencies and build:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake \
    libcurl4-openssl-dev libcjson-dev libssl-dev libwebsockets-dev

mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install
sudo ldconfig
```

### macOS

```bash
brew install cmake curl cjson openssl libwebsockets

mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install
```

### Windows

1. Install [Visual Studio](https://visualstudio.microsoft.com/) (2019+) with "Desktop development with C++"
2. Install [CMake](https://cmake.org/download/)
3. Install [vcpkg](https://vcpkg.io/) and install dependencies for **x64**:

```powershell
vcpkg install cjson:x64-windows curl:x64-windows openssl:x64-windows libwebsockets:x64-windows
```

4. Build (from the SDK root directory):

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64
cmake --build build --config Release
cmake --install build --config Release --prefix C:\fyers-sdk
```

Use your actual vcpkg path if it is not `C:\vcpkg`. Add the install `bin/` directory to your PATH, or copy `fyers-api-c.dll` next to your executable.

### Link in Your Project

**Direct compile (gcc/clang):**
```bash
gcc main.c -lfyers-api-c -o myapp
```

**CMake:**
```cmake
find_package(fyers-api-c REQUIRED)
add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE fyers-api-c::fyers-api-c)
```

**Include headers:**
```c
#include <fyers_api.h>
#include <fyers_model.h>
#include <fyers_session.h>
#include <fyers_data_ws.h>
#include <fyers_order_ws.h>
```

### Quick Start

```c
#include <fyers_api.h>
#include <fyers_session.h>
#include <fyers_model.h>

int main(void) {
    fyers_session_t* session = fyers_session_create(
        "YOUR_CLIENT_ID", "YOUR_REDIRECT_URI", "YOUR_SECRET_KEY"
    );
    generate_authcode(session);
    fyers_session_set_authcode(session, "AUTH_CODE_FROM_REDIRECT");
    generate_token(session);

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    fyers_model_t* model = fyers_model_create(
        client_id, access_token, false, NULL, FYERS_LOG_INFO
    );

    fyers_response_t* response = fyers_model_get_profile(model);
    if (response && response->error == FYERS_OK)
        printf("Profile: %.*s\n", (int)response->size, response->data);
    fyers_response_destroy(response);

    fyers_model_destroy(model);
    fyers_session_destroy(session);
    fyers_cleanup();
    return 0;
}
```

Try running the sample code from the [Fyers API documentation](https://myapi.fyers.in/docsv3).

## APIs Supported by Fyers C SDK

#### User

* Profile
* Funds
* Holdings

#### Transaction Info

* Orders
* Positions
* Trades

#### Order Placement

* Place Order
* Place Multi Order
* Place Basket Orders
* Place MultiLeg Order

#### GTT Orders

* GTT Single
* GTT Modify Order
* GTT Cancel Order
* GTT Order Book

#### Smart Orders

* Smart Limit
* Smart Trail
* Smart Step
* Smart SIP
* Modify Smart Order
* Cancel Smart Order
* Pause Smart Order
* Resume Smart Order
* Smart Order Book

#### Smart Exit

* Create Smart Exit
* Fetch Smart Exit
* Modify / Activate / Deactivate Smart Exit

#### Other Transactions

* Modify Orders
* Cancel Order
* Exit Position
* Convert Position

#### Broker Config

* Market Status

#### Data Api

* History
* Quotes
* Market Depth
* Option Chain

#### Price Alert

* Create Price Alert
* Get Price Alerts
* Modify Price Alert
* Delete Price Alert
* Enable/Disable Price Alert

## Web Socket

#### General Socket

* General Socket (orders)
* General Socket (trades)
* General Socket (positions)
* General Socket (general)

#### Market Data

* Market Data Symbol Update
* Market Data Indices Update
* Market Data Depth Update
* Market Data Lite-Mode

## Supported Platforms

| Platform | Library Output |
|----------|----------------|
| **macOS** | `libfyers-api-c.dylib` |
| **Linux** | `libfyers-api-c.so` |
| **Windows** | `fyers-api-c.dll` + `fyers-api-c.lib` |

## Examples

Build with examples enabled:

```bash
cmake .. -DBUILD_EXAMPLES=ON
make
```

Examples are in `build/examples/` (REST API, WebSocket data, WebSocket orders).

## Troubleshooting

| Issue | Fix |
|-------|-----|
| **macOS:** `dyld: Library not loaded` | Set `DYLD_LIBRARY_PATH=/usr/local/lib` or install to `/usr/local` |
| **Linux:** `cannot open shared object file` | Run `sudo ldconfig` or set `LD_LIBRARY_PATH=/usr/local/lib` |
| **Windows:** `fyers-api-c.dll was not found` | Add `bin/` to PATH or copy DLL next to your executable |
| **Missing dependencies** | Linux: install `-dev` packages; macOS: Homebrew; Windows: vcpkg |

## Release Notes

* Cross-platform support (macOS, Linux, Windows)
* Shared library with proper symbol export for Windows DLL
* CMake package config for `find_package(fyers-api-c)`
* Smart Orders and Smart Exits API support
