//
// Created by nigo on 28/12/2019.
//

#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include "epoll.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

struct connection;

struct server {
  friend struct connection;
  server(uint16_t port, epoll_ctr& ctr);
  ~server();

 private:
  uint16_t port;
  epoll_ctr& ctr;
  epolled_fd server_fd;
  std::map<connection*, std::unique_ptr<connection>> connections;

  void disconnect(connection& conn);

  static int open_server_socket(uint16_t port);
  static std::string getaddrinfo(std::string query);
};

struct connection {
  connection(server& srv_, int fd_);
  ~connection();

  static size_t const BUF_SIZE = 1024;
  static size_t const TIMEOUT = 5 * 60;
 private:
  template <bool on>
  void update_timer();

  server& srv;
  epolled_fd fd;
  epolled_fd timer;
  std::mutex m;
  char buf[BUF_SIZE];
  std::queue<std::string> queries;
  bool has_work{};
  bool cancel{};
  std::condition_variable cv;
  std::thread current_thread;
};

struct addrinfo_guard {
  addrinfo** info{};

  ~addrinfo_guard();
};

#endif //SERVER_H
