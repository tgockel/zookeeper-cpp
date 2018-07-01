Contributing Guide
==================

So you want to contribute to the ZooKeeper C++ library?
I'd love your contribution!
Please help me.

Building
--------

Building the system only requires `CMake <https://cmake.org/>`_ and the standard-issue C++ compilation tools.

Docker
^^^^^^

Docker is the official mechanism for supporting multiple Linux distributions (see the
`TravisCI <https://travis-ci.org/tgockel/zookeeper-cpp>`_ build).
If you would like to do this at home, simply use the ``dev-env`` script::

    $> cd /path/to/zookeeper-cpp
    $> ./config/dev-env ubuntu-18.04

This will create a Docker image named something like ``dev/zookeeper-cpp/ubuntu-18.04`` and run that image with the
project's working directory mapped to ``~/zookeeper-cpp`` with you in control of a shell.
Inside Docker, you can now build::

    root@0ae2f54b152b:~/zookeeper-cpp# mkdir build-debug
    root@0ae2f54b152b:~/zookeeper-cpp# cd build-debug
    root@0ae2f54b152b:~/zookeeper-cpp/build-debug# cmake -GNinja ..
    ... output ...
    root@0ae2f54b152b:~/zookeeper-cpp/build-debug# ninja test
    ... output ...

This experience is pretty decent.
The biggest annoyance is editing within the Docker image makes files you touch owned by *root* (I suspect there is a way
to prevent this, but I am far from competent at Docker).
If you use `KDevelop <https://www.kdevelop.org/>`_, you can use the IDE to build and debug inside of these images with
`KDevelop Runtimes <http://www.proli.net/2017/05/23/kdevelop-runtimes-docker-and-flatpak-integration/>`_.

Process
-------

This library follows the `GitHub Fork + Pull Model <https://help.github.com/articles/about-pull-requests/>`_.
Below are the more project-specific steps.

Issue Tracker
^^^^^^^^^^^^^

All work *must* be tracked in the `Issue Tracker <https://github.com/tgockel/zookeeper-cpp/issues>`_, otherwise the
maintainer will have no idea what is going on.
Try to find an existing bug in the list of issues -- if you can't find it, open a new issue with a descriptive title and
descriptive description.
If you are unclear on if it should be a bug or not, mark it with a *Question* tag or just send me an
`email <mailto:travis@gockelhut.com>`_.
Assign the issue to yourself so I don't forget who is working on it.

For more granular tracking, the issue should move across the
`GitHub Project board <https://github.com/tgockel/zookeeper-cpp/projects/1>`_.
The columns of the project should be somewhat intuitive:

:Backlog:
    Things we are planning on doing soon.

:Design:
    System is being designed.
    What this usually means is the API is being written.
    *Please* write your API first -- it can save a lot of time in the long run.

:Implementation:
    The component is currently being implemented.

:Pull Request:
    There is an open pull request.

:Done:
    Work is complete!

Developing
^^^^^^^^^^

1. Fork the repository.
2. Branch in your fork (not actually required, but generally considered a Good Idea).
3. Write your code.
4. If this is your first contribution, add yourself to ``AUTHORS`` (alphabetically).
5. Commit your code (somewhere in the commit message, be sure to mention "Issue #NN", where "NN" is the issue number you
   were working on).
6. Watch your tests pass for all environments in TravisCI.
7. Issue a pull request from your branch to the master branch of the main repository.
8. Close the branch in your repository (not actually required, but clean repos are nice).

Sign Your Commits
"""""""""""""""""

When committing code, please `sign commits with GPG <https://help.github.com/articles/signing-commits-using-gpg/>`_.
This lets me know that work submitted by you was really created by you (security or something like that).
If you always want to sign commits instead of specifying ``-S`` on the command line every time, add it to your global
configuration::

    $> git config --global user.signingkey ${YOUR_KEY_ID}
    $> git config --global commit.gpgsign true
