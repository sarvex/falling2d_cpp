Falling2d
=========
Falling2d is a basic 2D physics engine aimed to be used in game.
Since it is still under development, it should not be used by anyone for now.
If you are hopelessly searching for a new physics engine to try out, follow the
following the Compilation guidelines.

#Compilation

## Dependencies ##

The demo application requires:
  * `sfml2.0 RC` - http://www.sfml-dev.org/download.php
  * `cmake`
The code should be portable but has been tested under ArchLinux only for now.

## Compilation steps ##

    $ git://github.com/sebcrozet/falling2d.git
    $ cd falling2d
    $ cmake CMakeList.txt -Wno-dev
    $ make
    $ ./bin/Release/falling\_demo

#Algorithms involved

## Broad phase ##

* Sweep & Prune detector (so, AABBs) using space/time coherency
* OBB tree on non-convex polygon (one OBB by concave polygon of its convex
    decomposition)

## Narrow phase ##

  * GJK-hybrid for collision detection & light penetrations
  * EPA for bigger penetrations

## Contacts solving ##

  * Impulse based
  * Projected Gauss-Seidel LCP solver

## Misc ##

  * Islands
  * Object/Island sleeping
  * Handles any polygons (convex, concave, crossed)
