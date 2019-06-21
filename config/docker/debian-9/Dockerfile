FROM debian:9
LABEL maintainer="Travis Gockel <travis@gockelhut.com>"


RUN apt-get update          \
 && apt-get install --yes   \
    cmake                   \
    grep                    \
    googletest              \
    ivy                     \
    lcov                    \
    libgtest-dev            \
    libzookeeper-mt-dev     \
    ninja-build

RUN echo 'APT::Default-Release "stable";'                                      >> /etc/apt/apt.conf \
 && echo 'deb http://deb.debian.org/debian testing main'                       >> /etc/apt/sources.list \
 && echo 'deb http://security.debian.org/debian-security testing/updates main' >> /etc/apt/sources.list \
 && echo 'deb http://deb.debian.org/debian testing-updates main'               >> /etc/apt/sources.list \
 && apt-get update \
 && apt-get install --yes -t testing g++-7 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-7 99

CMD ["/root/zookeeper-cpp/config/run-tests"]
