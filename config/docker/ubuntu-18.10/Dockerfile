FROM ubuntu:18.10
LABEL maintainer="Travis Gockel <travis@gockelhut.com>"

RUN apt-get update          \
 && apt-get install --yes   \
    cmake                   \
    grep                    \
    googletest              \
    g++                     \
    ivy                     \
    lcov                    \
    libgtest-dev            \
    libzookeeper-mt-dev     \
    ninja-build

CMD ["/root/zookeeper-cpp/config/run-tests"]
