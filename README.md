# wsh

wsh is a shell-like program made for devops and sysadmins that need to 
administer several machines at once. With `wsh`, you can ssh into multiple 
machines and run one-off commands very simply

## What wsh isn't

* configuration management 
* a shell 
* an `ssh` replacement

## Dependencies

* `glib2` >= 2.32
* `libssh` (**NOT** `libssh2`) 
* `protobuf-c`
* `sudo`

### Build deps

* `cmake`
* c compiler (`gcc`/`clang`) 
* `make`/`ninja`
* `doxygen` if you want API docs

## Architecture

`wshd` is a program that resides on remote hosts. `wshc` ssh's into
them, honoring your ssh config and exec's `wshd`. Over the secure ssh
pipe, `wshc` issues instructions to `wshd`, and `wshd` executes them on
the remote host. If privileges need to be raised, `wshc` will prompt
you for creds prior to ssh'ing, and submit them once the ssh
connection has been established.

## Advantages

* configuration-less

wsh honors your existing `ssh` and `sudo` configuration files, leaving
all of the work of authorization and authentication to `ssh` and `sudo`

* sudo-ready

Unlike `dsh`, wsh has `sudo` support built in, so that you don't need
to use some kind of hack to get your `sudo` creds over the network.

* logging

wsh logs everything to syslog

# BUILDING

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

# How does this compare to ansible and fabric?

I happen to have 1600 hosts lying around so...

```
wshc -f hosts -t 100 -- uname -a  18.88s user 2.02s system 92% cpu 22.630 total
ansible all -i ./hosts -f 30 -a "uname -a"  702.94s user 8.69s system 128% cpu 9:11.77 total
fab -H $(paste -s hosts -d,) -D -P -- uname -a  176.96s user 90.55s system 332% cpu 1:20.55 total

[ worr on worr-ld1 ] ( ~ ) % wc -l hosts
1611 hosts
```
