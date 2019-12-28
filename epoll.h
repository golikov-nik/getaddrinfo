//
// Created by nigo on 27/12/2019.
//

#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <functional>
#include "file_descriptor.h"

struct epolled_fd;

struct epoll_ctr {
  epoll_ctr();
  ~epoll_ctr();

  void add(epolled_fd& efd);
  void remove(epolled_fd& efd);
  void poll();

  static const size_t MAX_EVENTS = 10;
  static const size_t TIMEOUT = 10 * 60;
 private:
  file_descriptor epoll_fd;
};

struct epolled_fd : file_descriptor {
  using callback_t = std::function<void(uint32_t)>;

  epolled_fd(int fd, epoll_ctr& ctr, callback_t cb);
  ~epolled_fd();

  epoll_ctr& ctr;
  callback_t cb;
};

#endif //EPOLL_H
