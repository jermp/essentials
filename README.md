Essentials
----------

A self-contained, header-only, collection of some essential C++ utilities that I use extensively for data structure design and benchmarking.
Meant to be used in larger projects.

#### NOTE: C++17 is needed to use the library.

What's inside?
--------------

* benchmarking facilities, including: messages displaying local time,
configurable timer class, function to prevent code elision by compiler,
simple creation and printing of json documents
* functions to serialize-to and load-from disk data structures
* functions to compute the numbr of bytes consumed by data structures
* support for creating, removing, and iterate inside directories

* **Experimental**: transparent support for contiguous memory allocation


Examples
--------

See the `test` folder for examples.
To compile such examples, just type the following commands from the parent directory.

    cd test
    mkdir build
    cd build
	cmake ..
	make