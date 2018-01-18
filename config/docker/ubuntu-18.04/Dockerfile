FROM ubuntu:18.04
MAINTAINER Travis Gockel <travis@gockelhut.com>

RUN apt-get update
RUN apt-get install --yes   \
    cmake                   \
    grep                    \
    g++-7                   \
    ninja-build             \
    ivy                     \
    libgtest-dev            \
    libzookeeper-mt-dev

RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-7 99

CMD ["/root/zookeeper-cpp/config/run-tests"]
