Contributing Guide
==================

So you want to contribute to the ZooKeeper C++ library?
I'd love your contribution!
Please help me.

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
    Not being actively worked on, but might be a good idea.

:Design:
    System is being designed.
    What this usually means is the API is being written.
    *Please* write your API first -- it can save a lot of time in the long run.

:Implementation:
    The component is currently being implemented.

:Testing:
    Component is implemented, but has not passed unit tests on all platforms.

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
