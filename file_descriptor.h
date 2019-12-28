//
// Created by nigo on 27/12/2019.
//

#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

struct file_descriptor {
  int fd;
  explicit file_descriptor(int fd);
  ~file_descriptor();
};

#endif //FILE_DESCRIPTOR_H
