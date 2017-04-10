# Contributing to wsh

Read the following if you want to contribute code to wsh

## File structure

There are three main directories:

1. library
2. server
3. client

Most of the logic should go into the lib. I'd like to present as much as wsh
as possible through libwsh, so that third party devs can use it to make
something cool.

Anything that is ultra client or server specific can go into the appropriate
folder.

Under each directory, there's src and test directories. Source code goes in
src, unit tests go into test.

## Branches

* devel - this is where the magic happens. Submit your pull requests against this branch
* master - the code here is guaranteed to build, and to work (for some definitions of work)

Any other branches are probably feature branches

## Tags

All releases are tagged commits on the master branch.

## How do I send a patch?

1. Fork it
2. Create a feature or a bugfix branch
3. dev dev dev
4. Write a unit test
5. make test
6. commit, rebase into a single commit and file a pull request

## Coding standards

* tabs for indentation, spaces for alignment
* unit tests
* markdown for files like this, mandoc for manpages

## Why didn't my patch get merged?

I like patches - they're less work for me, but there's a difference between an
acceptable patch and shit.

* Write some unit tests. Test success, failure, everything. I measure coverage with coveralls, so if total coverage decreases, then I won't merge
* If tests fail, I will not merge your patch until you fix it
* If you dump something that should be in the lib in the server or client, I'll tell you to rework your patch
* Undocumented features - edit the manpages/docs
* No doxygen docs

## Contributors License Agreement

By contributing you agree that these contributions are your own (or approved
by your employer) and you grant a full, complete, irrevocable copyright
license to all users and developers of the project, present and future,
pursuant to the license of the project.
