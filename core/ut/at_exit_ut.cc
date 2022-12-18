#include "core/at_exit.h"

#include "gtest/gtest.h"

#include <iostream>
#include <memory>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

struct AtExitParams {
  AtExitParams(int d, const char* s)
      : fd(d)
      , str(s) {}

  int fd;
  const char* str;
};

void MyAtExitFunc(void* ptr) {
  std::unique_ptr<AtExitParams> params(static_cast<AtExitParams*>(ptr));
  if (write(params->fd, params->str, strlen(params->str)) < 0) {
    abort();
  }
}

TEST(AtExitTest, TestAtExit) {
  int ret;
  int pipefd[2];

  ret = pipe(pipefd);

  ASSERT_TRUE(ret == 0);

  pid_t pid = fork();

  if (pid > 0) {
    char data[1024];
    memset(data, 0, 1024);
    int last = 0;

    close(pipefd[1]);

    while (read(pipefd[0], data + last++, 1) > 0 && last < 1024) {
    }
    data[--last] = 0;

    ASSERT_TRUE(strcmp(data, "High prio\nMiddle prio\nLow-middle prio\nLow prio\nVery low prio\n") == 0);
  } else {
    close(pipefd[0]);

    core::AtExit(MyAtExitFunc, new AtExitParams(pipefd[1], "Low prio\n"), 3);
    core::AtExit(MyAtExitFunc, new AtExitParams(pipefd[1], "Middle prio\n"), 5);
    core::AtExit(MyAtExitFunc, new AtExitParams(pipefd[1], "High prio\n"), 7);
    core::AtExit(MyAtExitFunc, new AtExitParams(pipefd[1], "Very low prio\n"), 1);
    core::AtExit(MyAtExitFunc, new AtExitParams(pipefd[1], "Low-middle prio\n"), 4);

    exit(0);
  }
}