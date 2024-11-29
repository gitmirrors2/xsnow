# Xsnow: let it snow on your desktop

## General

Xsnow is derived from Rick Jansen's [xsnow-1.42](https://janswaal.home.xs4all.nl/Xsnow/).
It now runs in many desktop environments: Gnome, KDE, FVWM, etc.

Find [here](https://ratrabbit.nl/ratrabbit/xsnow/index.html) more information 
about this release.

## Compilation and installation

The file `dependencies` lists the dependencies. These should be
installed before compiling xsnow.

     tar xf xsnow-<version>.tar.gz
     cd xsnow-<version>
     # in some cases you need to run
     #    ./bootstrap
     # this requires that you have autoconf and automake installed
     ./configure
     make
     sudo make install

Xsnow will be installed in /usr/local/games as `xsnow`.

For users of debian distro's: you can download the appropriate 
.deb file and install with:
  
     sudo apt install ./xsnow_<version>_<arch>.deb

Raspberry pi - Raspian users: choose the .deb with arch=armhf (32 bit) or arch=arm64 (64 bit).

If the above recipes do not work, you can try and run the
script 'simplemake.sh':

     ./simplemake.sh

If problems persist, you can adapt simplemake.sh.

## Maintainers:

The bootstrap script

  ./bootstrap

puts everything in place and performs a autoreconf -fvi.
See also: Languages.

### Running make check

When the environment variable XSNOW_FAST_CHECK is set, 
make check will only perform a superficial check, otherwize
a more thorough test invoked. You need Xvfb, scrot, 
xdotool and xdpyinfo (from x11-utils) available.

When the environment variable XSNOW_USESCREEN is set,
the thorough test will use your screen to run xsnow in.
Be sure not to move the mouse when the test is running.

## Self replication

When not disabled in `./configure`, xsnow will be build as a self-replicating
program, self-replicating activated by the flag `-selfrep`.

To create a tar ball:

     xsnow -selfrep > xsnow.tar.gz

To create the source ball directly:

     xsnow -selfrep | tar zxf -

If you changed something, run `./configure` or `make dist` to refresh the tar ball, 
otherwise `xsnow -selfrep` will create the old version of the source.

## Languages

Since the autoconf suite changes the .po files in po/, I placed the .po files in
pofiles/. In bootstrap these files are copied over .po files in po/.


Have fun!



