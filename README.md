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

Features
--------

- CardDAV: vCard Extensions to Web Distributed Authoring and Versioning (WebDAV) [RFC 6352](http://tools.ietf.org/html/rfc6352)
- The OAuth 2.0 Authorization Framework [RFC 6749](http://tools.ietf.org/html/rfc6749)
- HTTP Authentication: Basic and Digest Access Authentication [RFC 2617](http://tools.ietf.org/html/rfc2617)
- Using POST to Add Members to Web Distributed Authoring and Versioning (WebDAV) Collections [RFC 5995](http://tools.ietf.org/html/rfc5995)
- vCard Format Specification (Version 4) [RFC 6350](http://tools.ietf.org/html/rfc6350)

Try it on Fedora
----------------

Install the dependencies:

    $ yum install autoconf gettext-devel automake intltool gcc git gtk3-devel neon-devel sqlite-devel

Now get it, build it and run it:

    $ git clone https://github.com/florianl/ContactCards.git
    $ cd ContactCards
    $ ./autogen.sh
    $ ./configure
    $ make
    $ ./src/contactcards

Alternatively ContactCards may be also installed from copr using [flo/contactcards/](https://copr.fedoraproject.org/coprs/flo/contactcards/).


License: GPLv2

Contact: dev (AT) der-flo.net

More Information: https://der-flo.net/contactcards/
