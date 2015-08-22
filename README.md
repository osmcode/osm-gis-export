
# OSM GIS Export

THIS IS A VERY PRELIMINARY VERSION JUST MOVED HERE FROM THE LIBOSMIUM EXAMPLES!

A bunch of programs to export OSM data into GIS formats such as Shapefiles,
PostgreSQL or Spatialite.


## Requires

- A C++11 compiler. GCC 4.8 and clang (LLVM) 3.4 and later work.
- [libosmium](https://github.com/osmcode/libosmium)
- [GDAL/OGR](http://gdal.org)


## Building

[CMake](http://www.cmake.org) is used for building.

To build run:

    mkdir build
    cd build
    cmake ..
    make


## License

OSM GIS Export is available under the Boost Software License. See LICENSE.txt.


## Authors

OSM GIS Export was mainly written and is maintained by Jochen Topf
(jochen@topf.org). See the git commit log for other authors.


## Contact

Bug reports, questions etc. should be directed to the
[issue tracker](https://github.com/osmcode/osm-gis-export).


