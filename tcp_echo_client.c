#include <stdlib.h>
#include <string.h>
#include <uv.h>

uv_loop_t *loop;
uv_tcp_t client;
uv_connect_t connect_req;
uv_buf_t buffer;
uv_stream_t *server;
uv_timer_t send_timer;
uv_timer_t retry_timer;

struct sockaddr_in dest;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void on_retry_connect(uv_connect_t *req, int status);
void on_connect(uv_connect_t *req, int status);
void on_write(uv_write_t *req, int status);
void on_send_timer(uv_timer_t *timer);
void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
void cleanup_and_retry(uv_timer_t *timer);

void start_retry() {
  uv_timer_stop(&send_timer);
  uv_close((uv_handle_t *)&client, NULL);
  uv_timer_init(loop, &retry_timer);
  // wait for 1 second, and try to reconnect the server every 2 seconds
  uv_timer_start(&retry_timer, cleanup_and_retry, 1000, 2000);
}

void cleanup_and_retry(uv_timer_t *timer) {
  // Reinitialize the TCP client handle
  uv_tcp_init(loop, &client);

  // Retry the connection
  uv_tcp_connect(&connect_req, &client, (const struct sockaddr *)&dest,
                 on_retry_connect);
}

void on_retry_connect(uv_connect_t *req, int status) {
  if (status < 0) {
    fprintf(stderr, "Connection retried error: %s\n", uv_strerror(status));
    return; // Keep retrying
  }

  // Stop the retry timer
  uv_timer_stop(&retry_timer);

  on_connect(req, status);
}

void on_connect(uv_connect_t *req, int status) {
  if (status < 0) {
    fprintf(stderr, "Connection error: %s\n", uv_strerror(status));

    // Retry connection after 1 second
    start_retry();

    return;
  }

  fprintf(stdout, "Connected to the server!\n");

  // Save the server handle for reading replies
  server = req->handle;

  // Start reading from the server
  uv_read_start((uv_stream_t *)server, alloc_buffer, on_read);

  // Create and start the timer to send the payload every 3 seconds
  uv_timer_init(loop, &send_timer);
  uv_timer_start(&send_timer, on_send_timer, 0,
                 3000); // 3000 milliseconds interval
}

void on_write(uv_write_t *req, int status) {
  if (status < 0) {
    fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    start_retry();
  }

  // Free the write request memory
  free(req);
}

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  if (nread > 0) {
    fprintf(stdout, "Received reply from server: %.*s\n", (int)nread,
            buf->base);
  } else if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
    }

    start_retry();
  }

  // Don't forget to free the buffer
  if (buf->base) {
    free(buf->base);
  }
}

void on_send_timer(uv_timer_t *timer) {
  // Predefined payload
  const char *payload = "Hello, server!";

  // Allocating memory for the buffer
  buffer = uv_buf_init((char *)payload, strlen(payload));

  // Allocating memory for the write request
  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));

  // Sending the payload
  int ret = uv_write(write_req, (uv_stream_t *)&client, &buffer, 1, on_write);
  if (ret < 0) {
    start_retry();
  }
}

int main() {
  loop = uv_default_loop();

  // Initialize TCP client
  uv_tcp_init(loop, &client);

  // Server address and port
  uv_ip4_addr("127.0.0.1", 8080, &dest);

  // Connect to the server
  uv_tcp_connect(&connect_req, &client, (const struct sockaddr *)&dest,
                 on_connect);

  // Run the loop
  return uv_run(loop, UV_RUN_DEFAULT);
}
