FROM ubuntu:20.04
LABEL maintainer="Travis Gockel <travis@gockelhut.com>"

RUN apt-get update                  \
 && DEBIAN_FRONTEND=noninteractive  \
    apt-get install --yes           \
    cmake                           \
    g++                             \
    grep                            \
    googletest                      \
    ivy                             \
    lcov                            \
    libgtest-dev                    \
    libzookeeper-mt-dev             \
    ninja-build

CMD ["/root/zookeeper-cpp/config/run-tests"]
