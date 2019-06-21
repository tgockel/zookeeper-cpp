FROM ubuntu:18.04
LABEL maintainer="Travis Gockel <travis@gockelhut.com>"

RUN apt-get update          \
 && apt-get install --yes   \
    cmake                   \
    grep                    \
    googletest              \
    g++-7                   \
    ivy                     \
    lcov                    \
    libgtest-dev            \
    libzookeeper-mt-dev     \
    ninja-build

RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-7 99

CMD ["/root/zookeeper-cpp/config/run-tests"]
