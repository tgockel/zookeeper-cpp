ZooKeeper C++
=============

A ZooKeeper client for C++.

Features include (but are not necessarily limited to):

- Simple

  - Connect with just a connection string
  - Clients should not require factories
  - Does not require *any* knowledge of the Java or C APIs

- Configurable

  - Use the parts you need
  - Change parts to fit in your application

- Safe

  - In the best case, illegal code should fail to compile
  - An illegal action should throw an exception
  - Utility functions have a `strong exception guarantee <http://www.gotw.ca/gotw/082.htm>`_

- Stable

  - Worry less about upgrading -- the API and ABI will not change out from under you

- Documented

  - Consumable by human beings
  - Answers questions you might actually ask

**NOTE**: This library is a work-in-progress.
All documentation you see here is subject to change and non-existence.

Usage
-----

Ultimately, the usage looks like this (assuming you have a ZooKeeper server running on your local host)::

    #include <zk/client.hpp>
    #include <zk/multi.hpp>

    #include <iostream>

    int main()
    {
        zk::client client("zk://127.0.0.1:2181");

        // foobar's type is zk::future<std::pair<zk::buffer, zk::stat>>
        auto foobar = client.get("/foo/bar");

        // children's type is zk::future<std::pair<std::vector<std::string>, zk::stat>>
        auto children = client.get_children("/foo");

        // set_res's type is zk::future<zk::stat>
        auto set_res = client.set("/foo/bar", "some data");
        auto foo_bar_version = set_res.get().data_version;

        // create_res's type is zk::future<std::string>
        auto create_res = client.create("/foo/baz", "more data");
        // More explicit: client.create("/foo/baz", "more data", zk::acls::all(), zk::create_flags::none);

        zk::multi ops =
        {
            zk::op::check("/foo", zk::version::any()),
            zk::op::check("/foo/baz", foo_bar_version),
            zk::op::create("/foo/bap", "hi", nullopt, zk::create_flags::sequential),
            zk::op::erase("/foo/bzr"),
        };
        // multi_res's type is zk::future<zk::multi_result>
        auto multi_res = client.commit(ops);
        multi_res.get();
    }

Value-Added Features
--------------------

The core library of ``libzkpp`` provides the primitives for connecting to and manipulating a ZooKeeper database.
This library also bundles a number of other features that are commonly required when working with a ZooKeeper cluster.

``zk/curator``
^^^^^^^^^^^^^^

Things in ``zk/curator`` have features found in the `Apache Curator <http://curator.apache.org/>`_ project.

* Elections

  * `Leader Latch <https://github.com/tgockel/zookeeper-cpp/issues/1>`_
  * `Leader Election <https://github.com/tgockel/zookeeper-cpp/issues/2>`_

* Locks

  * `Shared Reentrant Lock <https://github.com/tgockel/zookeeper-cpp/issues/3>`_
  * `Shared Lock <https://github.com/tgockel/zookeeper-cpp/issues/4>`_
  * `Shared Reentrant Read Write Lock <https://github.com/tgockel/zookeeper-cpp/issues/5>`_
  * `Shared Semaphore <https://github.com/tgockel/zookeeper-cpp/issues/6>`_
  * `Multi Shared Lock <https://github.com/tgockel/zookeeper-cpp/issues/7>`_

* Barriers

  * `Barrier <https://github.com/tgockel/zookeeper-cpp/issues/8>`_
  * `Double Barrier <https://github.com/tgockel/zookeeper-cpp/issues/9>`_

* Counters

  * `Shared Counter <https://github.com/tgockel/zookeeper-cpp/issues/10>`_
  * `Distributed Atomic Long <https://github.com/tgockel/zookeeper-cpp/issues/11>`_

* Caches

  * `Path Cache <https://github.com/tgockel/zookeeper-cpp/issues/12>`_
  * `Node Cache <https://github.com/tgockel/zookeeper-cpp/issues/13>`_
  * `Tree Cache <https://github.com/tgockel/zookeeper-cpp/issues/14>`_

* Nodes

  * `Persistent Node <https://github.com/tgockel/zookeeper-cpp/issues/15>`_
  * `Persistent TTL Node <https://github.com/tgockel/zookeeper-cpp/issues/16>`_
  * `Group Member <https://github.com/tgockel/zookeeper-cpp/issues/17>`_

None of the queue types are planned to be implemented.
The `Curator Documentation (TN4) <https://cwiki.apache.org/confluence/display/CURATOR/TN4>`_ advises against their use,
claiming "it is a bad idea to use ZooKeeper as a Queue."
The authors of this library agree with this claim.

``zk/fake``
^^^^^^^^^^^

This library also provides a fake version of ZooKeeper which operates in-memory.
It is meant to be used in your unit testing, when fine-grained control of behavior of ZooKeeper is needed.
This allows for the injection of arbitrary behavior into ZK, allowing you to simulate some of the hard-to-reproduce
issues like ``zk::event_type::not_watching``, ``zk::marshalling_error``, or timing bugs.
It also allows for fast creation and teardown of entire databases, which is commonly done in unit testing.

It is connected to through using a connection string of the form::

    fake://{name}

To use this in unit tests link to ``libzkpp_fake`` and ``zk::fake::server``::

    TEST(my_test)
    {
        // The default constructor uses a randomly-generated unique name
        zk::fake::server server;

        // Fetch that name through the connection_string
        zk::client client(server.connection_string());

        // use client normally
    }

``zk/server``
^^^^^^^^^^^^^

This library controls a ZooKeeper Java process on this machine.
It is meant to be used in applications that manage a ZooKeeper cluster from native code.

Unsupported Functionality
-------------------------

If you are used to using ZooKeeper via the Java or C APIs, there are a few things that are explicitly not supported in
this library.

Global Watches
^^^^^^^^^^^^^^

There are two main ways to receive watch notifications: the global watch or through use a watcher objects.
In the Java API, the ``ZooKeeper`` client allows for a global
`Watcher <https://zookeeper.apache.org/doc/r3.4.10/api/org/apache/zookeeper/Watcher.html>`_.
In the C API, ``zookeeper_init`` can be provided with a global function with the signature
``void (*)(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx)`` to achieve this same result.
The ZooKeeper community considers global watches as "legacy" and prefers the use of watcher objects set on a per-path
basis.
As such, global watches are *not* supported by this library.

Synchronous API
^^^^^^^^^^^^^^^

The C library offers both a synchronous and an asynchronous API.
This library offers only an asynchronous version.
If you prefer a synchronous API, call ``get()`` on the returned ``future`` to block until you receive the response.

Non-Linux
^^^^^^^^^

Can you get this library working on platforms that are not Linux?
Maybe.
But Linux is the primary development, testing, and deployment platform of people writing distributed applications, so
this library is targetted at Linux.
Blame Windows.

License
-------

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
the License. You may obtain a copy of the License at
`http://www.apache.org/licenses/LICENSE-2.0 <http://www.apache.org/licenses/LICENSE-2.0>`_.

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.

F.A.Q.
------

Why ``erase`` instead of ``delete``?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the Java and C APIs, the act of removing a ZNode is called ``delete`` and ``zoo_delete``, respectively.
However, ``delete`` is a C++ keyword and cannot be used as a member function.
So, this library uses ``erase``, which falls in line with standard C++ containers.
Alternatives such as calling the operation ``delete_`` look a bit worse (in the author's opinion).
