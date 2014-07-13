ContactCards
============

ContactCards is a simple address book written in C.

vCards are safed  in a local database and fetched from a remote server using the
cardDav protocoll.

[Bugtracker](https://github.com/florianl/ContactCards/issues)

Requirements
------------

The following packages are required:
- libgtk-3-dev
- libsqlite3-dev
- libneon27-dev

Build
-----

	$ ./autogen.sh
	$ ./configure
	$ make

License: GPLv2
Contact: dev (AT) der-flo.net
More Information: https://der-flo.net/contactcards/
