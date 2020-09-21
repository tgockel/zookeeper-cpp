FROM opensuse/leap:15
LABEL maintainer="Travis Gockel <travis@gockelhut.com>"

RUN zypper addrepo -f "https://download.opensuse.org/repositories/server:/database/openSUSE_Leap_15.2/" "serverdatabase" \
 && zypper --no-gpg-checks  \
    install -y              \
    apache-ivy              \
    cmake                   \
    grep                    \
    gcc-c++                 \
    git                     \
    googletest-devel        \
    java-1_8_0-openjdk      \
    lcov                    \
    libzookeeper2-devel     \
    ninja

CMD ["/root/zookeeper-cpp/config/run-tests"]
