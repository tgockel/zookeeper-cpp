FROM ubuntu:16.04
MAINTAINER Travis Gockel <travis@gockelhut.com>

RUN apt-get update
RUN apt-get install --yes software-properties-common
RUN add-apt-repository --yes ppa:ubuntu-toolchain-r/test
RUN apt-get update

# You might ask why g++-6 is installed, even when we intend to build with GCC 7. It turns out that Coveralls won't work
# if it isn't installed due to...something. Instead of root causing the issue, we're going to ignore it and hope it gets
# fixed in a future release.
RUN apt-get install --yes   \
    cmake                   \
    grep                    \
    g++-6                   \
    g++-7                   \
    ivy                     \
    libgtest-dev            \
    libzookeeper-mt-dev

# Code Coverage
RUN apt-get install --yes   \
    git                     \
    lcov                    \
    python-pip
RUN pip install --upgrade pip
RUN pip install     \
    cpp-coveralls   \
    pyyaml

# Short story: update-alternatives is the right solution for things, but this involves less Ubuntu.
RUN rm -f /usr/bin/c++
RUN ln -s /usr/bin/g++-7 /usr/bin/c++

CMD ["/root/zookeeper-cpp/config/run-tests"]
