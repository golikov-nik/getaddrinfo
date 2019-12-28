//
// Created by nigo on 28/12/2019.
//

#include <iostream>
#include "epoll.h"
#include "server.h"

int main() {
  try {
    epoll_ctr epoll;
    server gai(5672, epoll);
    epoll.poll();
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
  }
}
