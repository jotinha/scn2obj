scn2obj
=======

scn2obj is a utility to convert .scn files, from everyone's favorite 3d tactical cooperative realistic less than lethal shooter SWAT3, to Wavefront's OBJ file format, a standard 3d file format.

It's the result of unhealthy cracking, hacking and binary editing the game mission files. It turns out to be similar in scructure to quake's bsp, with additional data for handling portals and cells, entities and lightmaps.

I'd say close to everything important of the file structure is understood, with the major exception of lightmaps. No documentation but you can check CScn.h and CScn.cpp if you're into that sort of thing.

This is a stripped down, gitified, version of the [scnedit project at googlecode](http://code.google.com/p/scnedit/), a full 3d viewer for scn files based on the Irrlicht engine.

Usage
-----
    scn2obj [-s <scale>] <filename>
    
        filename - scn file name with extension
         
        OPTIONS:
        -s <scale> - scale geometry by factor <scale>


Compiling
---------

Compilation should be pretty straightforward. Only using standard C/C++ libraries.

Run `make build` or simply `g++ src/*.cpp -o scn2edit`

License
-------
scn2obj is covered by the new BSD license. See LICENSE.