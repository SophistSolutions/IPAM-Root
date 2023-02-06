# About this project

IPAM - Interplanetary Album Manager - is a set of tools to organize and categorize content on the
IPFS - [Interplanetary Filesystem](https://ipfs.tech/).

The original motivation behind IPAM was to allow seemless sharing and organizing of photo albums,
but without giving up ownership/control to a third party (like Facebook or Google).

## Design Overview
[Design Overview](Documentation/ReadMe.md).

## Building

~~~
make all
~~~

or for a quicker build

~~~
make CONFIGURATION=Debug all -j10
~~~

## Testing

~~~
make run-tests
~~~
