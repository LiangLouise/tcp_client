# TCP Echo Client with Libuv

* send to the target every 3 and expects an echo reply;
* if the target gets down, wait for 1 second and try to reconnect every 2 seconds until the target is up again.