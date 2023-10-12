# Project design

## Perfect Links

### UDP receive

The process is listening on the UDP socket for messages. When a message is received, it asynchronously calls `PerfectLink.Receive(m)` to process the message.

### PerfectLink

The perfect link object needs to contain a concurrent set of delivered messages and a handler function which handles the deliver of the object.

### PerfectLink.Send

The `PerfectLink.send(m)` function stubbornly sends the message:

```c++
while(true) {
    UDP_send(m);
}
```

This should be done in a separate thread, in order not to block the receiving of messages.
Also, there's no need for timeout if we configure the UDP socket to block, this way we can have the `UDP_send()` call to block, which will result in fastest possible message sending (possibly even filling the entire socket sending buffer, so it will block until there's enough space in the send buffer).

### PerfectLink.Receive

The `PerfectLink.Receive(m)` operation is described as follows:

```c++
if (set.contains(m)) {
    set.insert(m);
    this.handle(m);
}
```

It is absolutely critical for `set.contains(m)` operation to be quick, as it will be invoked a lot, compared to the `set.insert(m)`.

We should be careful to make these operations thread-safe, otherwise, we risk calling the `this.handle(m)` twice, in the case when another thread manages to complete `set.contains()` while the current thread hasn't finished `set.insert(m)`.

### PerfectLink receive handler

When we receive the message, we are supposed to write it to a file. This could be done in a handler everytime, or, we could invoke the handler to cash the writes in memory and then periodically write to file. This means that when we clean up the process, we still have to flush everything to disk, otherwise we risk losing the data.

### Signal handlers

By using the [`signal()`](https://www.tutorialspoint.com/cplusplus/cpp_signal_handling.htm) function we can register a handler to clean up the process.

### ACK Messages

When receiving a message, each process returns an ACK message to the sender to signal that it got the message. The sender then send an ACK back to signal that it received ACK, so both processes can stop stubbornly sending messages.

Without this second ACK, the receiver process will have to stubbornly send messages as it would be unaware if the first process crashed or received the message.

This is not exactly in the specification of perfect links, however, this would fullfill the property of perfect links, while managing to interrupt the stubborn sending of messages.

### Multiple threads receiving and sending

We can just have multiple threads receiving, since they will block on the UDP.recv. For multiple sending processes, we could have a counter representing which thread the send should go to, i.e if there are 3 threads and the counter == 0, then it goes to thread 1, etc.
