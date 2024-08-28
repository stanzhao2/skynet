

#include <stdnet.h>
using namespace stdnet;

void print_error(const error_code& ec) {
  printf("%s\n", ec.message().c_str());
}

void on_receive(const error_code& ec, const char* data, size_t size, typeof<io::socket> socket) {
  if (ec) {
    print_error(ec);
    socket->close();
    return;
  }
  socket->async_send(data, size);
}

int main() {
  error_code ec;
  auto ios = ref_new<io::service>();
  ios->signal().add(SIGINT, ec);
  ios->signal().add(SIGTERM, ec);
  ios->signal().async_wait(
    [ios](const error_code& ec, int code) {
      ec ? print_error(ec) : ios->stop();
    }
  );
  auto server = ref_new<io_socket_server>(ios, io::socket::websocket);
  ec = server->listen(8800, 0, [](const error_code& ec, typeof<io::socket> peer) {
    ec ? peer->close() : async_receive(peer, _bind(on_receive, _Arg1, _Arg2, _Arg3, peer));
  });
  if (ec) {
    print_error(ec);
    return 0;
  }
  while (!ios->stopped()) {
    ios->run();
  }
  return 0;
}
