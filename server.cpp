//  Copyright 2019 Nikita Golikov

#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <iostream>
#include <netdb.h>
#include "server.h"
#include <arpa/inet.h>
#include <sys/timerfd.h>

server::server(uint16_t port_, epoll_ctr& ctr_) :
               port(port_),
               ctr(ctr_),
               server_fd(open_server_socket(port), ctr, [this] {
                 auto fd = accept(server_fd.fd, nullptr, nullptr);
                 auto ptr = std::make_unique<connection>(*this, fd);
                 connections.insert(std::make_pair(ptr.get(), std::move(ptr)));
               }) {}

int server::open_server_socket(uint16_t port) {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    throw std::runtime_error(std::strerror(errno));
  }
  sockaddr_in sockaddr_;
  sockaddr_.sin_family = AF_INET;
  sockaddr_.sin_addr.s_addr = 0;
  sockaddr_.sin_port = htons(port);

  int enable = 1;
  auto r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (r < 0) {
    throw std::runtime_error(std::strerror(errno));
  }

  auto result = bind(fd, reinterpret_cast<sockaddr const*>(&sockaddr_),
          sizeof sockaddr_);
  if (result) {
    throw std::runtime_error(std::strerror(errno));
  }
  result = listen(fd, SOMAXCONN);
  if (result) {
    throw std::runtime_error(std::strerror(errno));
  }
  return fd;
}

server::~server() = default;

connection::connection(server& srv_, int fd_) :
                       srv(srv_),
                       fd(fd_, srv.ctr, [this] {
                         auto n = recv(fd.fd, buf, sizeof buf, 0);
                         if (n < 0) {
                           throw std::runtime_error(std::strerror(errno));
                         }
                         if (!n) {
                           srv.disconnect(*this);
                           return;
                         }
                         if (n == 1) {
                           return;
                         }
                         std::string url(buf, n);
                         std::unique_lock lg(m);
                         try {
                           process_data(url);
                         } catch (bad_connection_error&) {
                           lg.unlock();
                           srv.disconnect(*this);
                           return;
                         } catch (...) {
                           std::cout << "unexpected error occurred" <<std::endl;
                         }
                         if (has_work) {
                           cv.notify_all();
                         }
                       }),
                       timer(timerfd_create(CLOCK_MONOTONIC, 0), srv.ctr,
                               [this] {
                         srv.disconnect(*this);
                       }),
                       current_thread([this] {
                         update_timer<true>();
                         while (true) {
                           std::unique_lock lg(m);
                           cv.wait(lg, [this] {
                             return cancel || has_work;
                           });
                           if (cancel) {
                             break;
                           }
                           auto url = queries.front();
                           queries.pop();
                           update_timer<false>();
                           lg.unlock();
                           auto processed = server::getaddrinfo(url);
                           lg.lock();
                           has_work = !queries.empty();
                           if (!has_work) {
                             update_timer<true>();
                           }
                           if (processed.empty()) {
                             processed = "An error occurred while processing"
                                          " address " + url + "\n";
                           }
                           if (srv.connections.count(this)) {
                             auto result = send(fd.fd, processed.c_str(),
                                                processed.size(), 0);
                             if (result < 0) {
                               std::cout << std::strerror(errno) << std::endl;
                             }
                           }
                         }
                       }) {}

connection::~connection() {
  {
    std::unique_lock lg(m);
    cancel = true;
    cv.notify_all();
  }
  current_thread.join();
  srv.disconnect(*this);
}

template <bool on>
void connection::update_timer() {
  constexpr itimerspec ts{{0, 0},
                          {on ? TIMEOUT : 0, 0}};
  auto r = timerfd_settime(timer.fd, 0, &ts,  nullptr);
  if (r) {
    std::cout << std::strerror(errno) << std::endl;
  }
}

void connection::process_data(std::string const& url) {
  auto is_ok = [](char ch) {
    char const* CHARS = "_.\\-~";
    auto i = static_cast<unsigned char>(ch);
    return std::isalpha(i) || std::isdigit(i) ||
      strchr(CHARS, ch);
  };
  for (auto ch : url) {
    if (prefix.size() >= BUF_SIZE) {
      std::cout << "too long query" << std::endl;
      throw bad_connection_error();
    }
    if (!is_ok(ch)) {
      if (prefix.empty()) {
        continue;
      }
      queries.push(prefix);
      has_work = true;
      prefix.clear();
    } else {
      prefix += ch;
    }
  }
}

std::string server::getaddrinfo(std::string const& query) {
  addrinfo* result{};
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo_guard g{&result};
  auto r = ::getaddrinfo(query.c_str(), "80", &hints, &result);
  if (r) {
    return "";
  }
  std::string result_string;
  for (auto cur = result; cur; cur = cur->ai_next) {
    result_string += inet_ntoa(reinterpret_cast<sockaddr_in const*>
                               (cur->ai_addr)->sin_addr);
    result_string += '\n';
  }
  return result_string;
}

void server::disconnect(connection& conn) {
  connections.erase(&conn);
}

addrinfo_guard::~addrinfo_guard() {
  freeaddrinfo(*info);
}
