# twc
This is a simple implementation of an async two-way-channel using the select system call.

It passes traffic between two file descriptors (typically sockets) simultaniouly without creating additional threads or processes.
