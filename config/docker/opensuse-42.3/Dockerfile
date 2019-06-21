FROM opensuse:42.3
LABEL maintainer="Travis Gockel <travis@gockelhut.com>"

COPY repos /tmp/repos
RUN zypper addrepo -f /tmp/repos/server:database.repo \
 && zypper --gpg-auto-import-keys refresh

RUN zypper install -y       \
    apache-ivy              \
    cmake                   \
    grep                    \
    gcc7-c++                \
    git                     \
    googletest-devel        \
    java-1_8_0-openjdk      \
    lcov                    \
    libzookeeper2-devel     \
    ninja

RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-7 99

CMD ["/root/zookeeper-cpp/config/run-tests"]
