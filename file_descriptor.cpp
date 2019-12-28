//  Copyright 2019 Nikita Golikov

#include "file_descriptor.h"

#include <unistd.h>
#include <stdexcept>
#include <cstring>

file_descriptor::file_descriptor(int fd) : fd(fd) {
  if (fd < 0) {
    throw std::runtime_error(std::strerror(errno));
  }
}

file_descriptor::~file_descriptor() {
  if (fd != -1) {
    close(fd);
  }
}
