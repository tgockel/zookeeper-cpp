FROM ubuntu:17.10
MAINTAINER Travis Gockel <travis@gockelhut.com>

RUN apt-get update
RUN apt-get install --yes software-properties-common
RUN add-apt-repository --yes ppa:ubuntu-toolchain-r/test
RUN apt-get update

RUN apt-get install --yes   \
    cmake                   \
    grep                    \
    g++-7                   \
    ivy                     \
    libgtest-dev            \
    libzookeeper-mt-dev     \
    ninja-build

RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-7 99

CMD ["/root/zookeeper-cpp/config/run-tests"]
