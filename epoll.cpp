//
// Created by nigo on 27/12/2019.
//

#include "epoll.h"

#include <utility>
#include <cstring>
#include <csignal>
#include <iostream>

namespace {
  volatile sig_atomic_t stop_polling_fl = 0;
  void stop_polling([[maybe_unused]] int signal) {
    stop_polling_fl = 1;
  }
}

epoll_ctr::epoll_ctr() : epoll_fd(epoll_create1(0)) {}

void epoll_ctr::add(epolled_fd& efd) {
  epoll_event evt;
  evt.events = EPOLLIN;
  evt.data.fd = efd.fd;
  evt.data.ptr = reinterpret_cast<void*>(&efd);
  auto result = epoll_ctl(epoll_fd.fd, EPOLL_CTL_ADD, efd.fd, &evt);
  if (result < 0) {
    throw std::runtime_error(std::strerror(errno));
  }
}

void epoll_ctr::remove(epolled_fd& efd) {
  auto result = epoll_ctl(epoll_fd.fd, EPOLL_CTL_DEL, efd.fd, nullptr);
  if (result < 0) {
    throw std::runtime_error(std::strerror(errno));
  }
}

void epoll_ctr::poll() {
  signal(SIGINT, ::stop_polling);
  signal(SIGTERM, ::stop_polling);
  signal(SIGPIPE, SIG_IGN);

  std::array<epoll_event, MAX_EVENTS> events;
  while (true) {
    auto n = epoll_wait(epoll_fd.fd, events.data(), MAX_EVENTS, TIMEOUT);
    if (stop_polling_fl) {
      break;
    }
    if (n < 0) {
      throw std::runtime_error(std::strerror(errno));
    }
    for (int i = 0; i != n; ++i) {
      reinterpret_cast<epolled_fd*>(events[i].data.ptr)->cb();
    }
  }
}

epoll_ctr::~epoll_ctr() = default;

epolled_fd::epolled_fd(int fd_, epoll_ctr& ctr_,
                     epolled_fd::callback_t cb_) : file_descriptor(fd_),
                                                  ctr(ctr_),
                                                  cb(std::move(cb_)) {
  ctr.add(*this);
}

epolled_fd::~epolled_fd() {
  ctr.remove(*this);
}
