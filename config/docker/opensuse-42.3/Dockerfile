FROM opensuse:42.3
MAINTAINER Travis Gockel <travis@gockelhut.com>

COPY repos /tmp/repos
RUN zypper addrepo -f /tmp/repos/server:database.repo
RUN zypper --gpg-auto-import-keys refresh

RUN zypper install -y       \
    apache-ivy              \
    cmake                   \
    grep                    \
    gcc7-c++                \
    git                     \
    googletest-devel        \
    java-1_8_0-openjdk      \
    lcov                    \
    libzookeeper2-devel

# There's probably a better way to do this...
RUN rm -f /usr/bin/c++
RUN ln -s /usr/bin/g++-7 /usr/bin/c++

CMD ["/root/zookeeper-cpp/config/run-tests"]
