FROM ubuntu:16.04
MAINTAINER Travis Gockel <travis@gockelhut.com>

RUN apt-get update
RUN apt-get install --yes software-properties-common
RUN add-apt-repository --yes ppa:ubuntu-toolchain-r/test
RUN apt-get update

RUN apt-get install --yes   \
    cmake                   \
    g++-7                   \
    lcov                    \
    libgtest-dev            \
    libzookeeper-mt-dev
RUN rm -f /usr/bin/c++
RUN ln -s /usr/bin/g++-7 /usr/bin/c++

COPY ["run-tests", "/root/run-tests"]
CMD ["/root/run-tests"]
