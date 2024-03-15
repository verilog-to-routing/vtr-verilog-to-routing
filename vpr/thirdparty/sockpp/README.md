# sockpp

Simple, modern, C++ socket library.

This is a fairly low-level C++ wrapper around the Berkeley sockets library using `socket`, `acceptor,` and `connector` classes that are familiar concepts from other languages.

The base `socket` class wraps a system socket handle, and maintains its lifetime. When the C++ object goes out of scope, it closes the underlying socket handle. Socket objects are generally _moveable_ but not _copyable_. A socket can be transferred from one scope (or thread) to another using `std::move()`.

Currently supports: IPv4, IPv6, and Unix-Domain Sockets on Linux, Mac, and Windows. Other *nix and POSIX systems should work with little or no modification.

There is also some experimental support for CAN bus programming on Linux using the SocketCAN package. This gives CAN bus adapters a network interface, with limitations dictated by the CAN message protocol.

All code in the library lives within the `sockpp` C++ namespace.

## Latest News

To keep up with the latest announcements for this project, follow me at:

**Mastodon/Fosstodon:** [@fpagliughi@fosstodon.org](https://fosstodon.org/@fpagliughi)

**Twitter:** [@fmpagliughi](https://twitter.com/fmpagliughi)

If you're using this library, send me a message, and let me know how you're using it.  I'm always curious to see where it winds up!


## Contributing

Contributions are accepted and appreciated. New and unstable work is done in the `develop` branch. Please submit all pull requests against that branch, not _master_.

For more information, refer to: [CONTRIBUTING.md](https://github.com/fpagliughi/sockpp/blob/master/CONTRIBUTING.md)


## Building the Library

CMake is the supported build system.

### Requirements:

- A conforming C++-14 compiler.
    - _gcc_ v5.0 or later (or) _clang_ v3.8 or later.
    - _Visual Studio 2015_, or later on WIndows.
- _CMake_ v3.12 or newer.
- _Doxygen_ (optional) to generate API docs.
- _Catch2_ (optional) v2.x or v3.x to build and run unit tests.

To build with default options:

```
$ cd sockpp
$ cmake -Bbuild .
$ cmake --build build/
```

To install:

```
$ cmake --build build/ --target install
```

### Build Options

The library has several build options via CMake to choose between creating a static or shared (dynamic) library - or both. It also allows you to build the example options, and if Doxygen is

Variable | Default Value | Description
------------ | ------------- | -------------
SOCKPP_BUILD_SHARED | ON | Whether to build the shared library
SOCKPP_BUILD_STATIC | OFF | Whether to build the static library
SOCKPP_BUILD_DOCUMENTATION | OFF | Create and install the HTML based API documentation (requires _Doxygen)_
SOCKPP_BUILD_EXAMPLES | OFF | Build example programs
SOCKPP_BUILD_TESTS | OFF | Build the unit tests (requires _Catch2_, v2.x or 3.x)
SOCKPP_BUILD_CAN | OFF | Build SocketCAN support. (Linux only)

Set these using the '-D' switch in the CMake configuration command. For example, to build documentation and example apps:

```
$ cd sockpp
$ cmake -Bbuild -DSOCKPP_BUILD_DOCUMENTATION=ON -DSOCKPP_BUILD_EXAMPLES=ON .
$ cmake --build build/
```

## TCP Sockets

TCP and other "streaming" network applications are usually set up as either servers or clients. An acceptor is used to create a TCP/streaming server. It binds an address and listens on a known port to accept incoming connections. When a connection is accepted, a new, streaming socket is created. That new socket can be handled directly or moved to a thread (or thread pool) for processing.

Conversely, to create a TCP client, a connector object is created and connected to a server at a known address (typically host and socket). When connected, the socket is a streaming one which can be used to read and write, directly.

For IPv4 the `tcp_acceptor` and `tcp_connector` classes are used to create servers and clients, respectively. These use the `inet_address` class to specify endpoint addresses composed of a 32-bit host address and a 16-bit port number.

### TCP Server: `tcp_acceptor`

The `tcp_acceptor` is used to set up a server and listen for incoming connections.

    int16_t port = 12345;
    sockpp::tcp_acceptor acc(port);

    if (!acc)
        report_error(acc.last_error_str());

    // Accept a new client connection
    sockpp::tcp_socket sock = acc.accept();

The acceptor normally sits in a loop accepting new connections, and passes them off to another process, thread, or thread pool to interact with the client. In standard C++, this could look like:

    while (true) {
        // Accept a new client connection
        sockpp::tcp_socket sock = acc.accept();

        if (!sock) {
            cerr << "Error accepting incoming connection: "
                << acc.last_error_str() << endl;
        }
        else {
            // Create a thread and transfer the new stream to it.
            thread thr(run_echo, std::move(sock));
            thr.detach();
        }
    }

The hazards of a thread-per-connection design is well documented, but the same technique can be used to pass the socket into a thread pool, if one is available.

See the [tcpechosvr.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpechosvr.cpp) example.

### TCP Client: `tcp_connector`

The TCP client is somewhat simpler in that a `tcp_connector` object is created and connected, then can be used to read and write data directly.

    sockpp::tcp_connector conn;
    int16_t port = 12345;

    if (!conn.connect(sockpp::inet_address("localhost", port)))
        report_error(conn.last_error_str());

    conn.write_n("Hello", 5);

    char buf[16];
    ssize_t n = conn.read(buf, sizeof(buf));

See the [tcpecho.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpecho.cpp) example.

### UDP Socket: `udp_socket`

UDP sockets can be used for connectionless communications:

    sockpp::udp_socket sock;
    sockpp::inet_address addr("localhost", 12345);

    std::string msg("Hello there!");
    sock.send_to(msg, addr);

    sockpp::inet_address srcAddr;

    char buf[16];
    ssize_t n = sock.recv(buf, sizeof(buf), &srcAddr);

See the [udpecho.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/udp/udpecho.cpp) and [udpechosvr.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/udp/udpechosvr.cpp) examples.
### IPv6

The same style of  connectors and acceptors can be used for TCP connections over IPv6 using the classes:

    inet6_address
    tcp6_connector
    tcp6_acceptor
    tcp6_socket
    udp6_socket

Examples are in the [examples/tcp](https://github.com/fpagliughi/sockpp/tree/master/examples/tcp) directory.

### Unix Domain Sockets

The same is true for local connection on *nix systems that implement Unix Domain Sockets. For that use the classes:

    unix_address
    unix_connector
    unix_acceptor
    unix_socket  (unix_stream_socket)
    unix_dgram_socket

Examples are in the [examples/unix](https://github.com/fpagliughi/sockpp/tree/master/examples/unix) directory.

### SocketCAN (CAN bus on Linux)

The Controller Area Network (CAN bus) is a relatively simple protocol typically used by microcontrollers to communicate inside an automobile or industrial machine. Linux has the _SocketCAN_ package which allows processes to share acces to a physical CAN bus interface using sockets in user space. See: [Linux SocketCAN](https://www.kernel.org/doc/html/latest/networking/can.html)

At the lowest level, CAN devices write individual packets, called "frames" to a specific numeric addresses on the bus. 

For examle a device with a temperature sensor might read the temperature persoidically and write it to the bus as a raw 32-bit integer, like:

```
can_address addr("CAN0");
can_socket sock(addr);

// The agreed ID to broadcast temperature on the bus
canid_t canID = 0x40;

while (true) {
    this_thread::sleep_for(1s);

    // Write the time to the CAN bus as a 32-bit int
    int32_t t = read_temperature();

    can_frame frame { canID, &t, sizeof(t) };
    sock.send(frame);
}
```

A receiver to get a frame might look like this:

```
can_address addr("CAN0");
can_socket sock(addr);

can_frame frame;
sock.recv(&frame);
```

## Implementation Details

The socket class hierarchy is built upon a base `socket` class. Most simple applications will probably not use `socket` directly, but rather use derived classes defined for a specific address family like `tcp_connector` and `tcp_acceptor`.

The socket objects keep a handle to an underlying OS socket handle and a cached value for the last error that occurred for that socket. The socket handle is typically an integer file descriptor, with values >=0 for open sockets, and -1 for an unopened or invalid socket. The value used for unopened sockets is defined as a constant, `INVALID_SOCKET`, although it usually doesn't need to be tested directly, as the object itself will evaluate to _false_ if it's uninitialized or in an error state. A typical error check would be like this:

    tcp_connector conn({"localhost", 12345});

    if (!conn)
        cerr << conn.last_error_str() << std::endl;

The default constructors for each of the socket classes do nothing, and simply set the underlying handle to `INVALID_SOCKET`. They do not create a socket object. The call to actively connect a `connector` object or open an `acceptor` object will create an underlying OS socket and then perform the requested operation.

An application can generally perform most low-level operations with the library. Unconnected and unbound sockets can be created with the static `create()` function in most of the classes, and then manually bind and listen on those sockets.

The `socket::handle()` method exposes the underlying OS handle which can then be sent to any platform API call that is not exposed by the library.

### Thread Safety

A socket object is not thread-safe. Applications that want to have multiple threads reading from a socket or writing to a socket should use some form of serialization, such as a `std::mutex` to protect access.

A `socket` can be _moved_ from one thread to another safely. This is a common pattern for a server which uses one thread to accept incoming connections and then passes off the new socket to another thread or thread pool for handling. This can be done like:

    sockpp::tcp6_socket sock = acc.accept(&peer);

    // Create a thread and transfer the new socket to it.
    std::thread thr(handle_connection, std::move(sock));

In this case, _handle_connection_ would be a function that takes a socket by value, like:

    void handle_connection(sockpp::tcp6_socket sock) { ... }

Since a `socket` can not be copied, the only choice would be to move the socket to a function like this.

It is a common patern, especially in client applications, to have one thread to read from a socket and another thread to write to the socket. In this case the underlying socket handle can be considered thread safe (one read thread and one write thread). But even in this scenario, a `sockpp::socket` object is still not thread-safe due especially to the cached error value. The write thread might see an error that happened on the read thread and visa versa.

The solution for this case is to use the `socket::clone()` method to make a copy of the socket. This will use the system's `dup()` function or similar create another socket with a duplicated copy of the socket handle. This has the added benefit that each copy of the socket can maintain an independent lifetime. The underlying socket will not be closed until both objects go out of scope.

    sockpp::tcp_connector conn({host, port});

    auto rdSock = conn.clone();
    std::thread rdThr(read_thread_func, std::move(rdSock));

The `socket::shutdown()` method can be used to communicate the intent to close the socket from one of these objects to the other without needing another thread signaling mechanism.

See the [tcpechomt.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpechomt.cpp) example.