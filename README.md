
# OSM GIS Export

THIS IS A VERY PRELIMINARY VERSION JUST MOVED HERE FROM THE LIBOSMIUM EXAMPLES!

A bunch of programs to export OSM data into GIS formats such as Shapefiles,
PostgreSQL or Spatialite.

[![Build Status](https://secure.travis-ci.org/osmcode/osm-gis-export.png)](http://travis-ci.org/osmcode/osm-gis-export)

Sorry, do docs yet. You have to look at the source code and change it according
to your needs. This software can be used as basis for your own experiments, but
you need to understand C++ for that. There is no one-size-fits-all solution
here. Use osmium_toogr as a basis if you only need nodes or ways, no
(multi)polygons. Use osmium_toogr2 as basis if you also need multipolygon
support.


## Requires

You need a C++11 compliant compiler. GCC 4.8 and later as well as clang 3.4 and
later are known to work. You also need the following libraries:

    Osmium Library
        Need at least version 2.6.1
        http://osmcode.org/libosmium

    Protozero
        https://github.com/mapbox/protozero
        Debian/Ubuntu: protozero
        Also included in the libosmium repository.

    Utfcpp
        http://utfcpp.sourceforge.net/
        Debian/Ubuntu: libutfcpp-dev
        Also included in the libosmium repository.

    gdalcpp
        https://github.com/joto/gdalcpp
        Also included in the libosmium repository.

    bz2lib (for reading and writing bzipped files)
        http://www.bzip.org/
        Debian/Ubuntu: libbz2-dev

    CMake (for building)
        http://www.cmake.org/
        Debian/Ubuntu: cmake

    Expat (for parsing XML files)
        http://expat.sourceforge.net/
        Debian/Ubuntu: libexpat1-dev
        openSUSE: libexpat-devel

    GDAL/OGR
        http://gdal.org/
        Debian/Ubuntu: libgdal-dev

    zlib (for PBF support)
        http://www.zlib.net/
        Debian/Ubuntu: zlib1g-dev
        openSUSE: zlib-devel

    Proj.4
        http://trac.osgeo.org/proj/
        Debian/Ubuntu: libproj-dev

## Installing dependencies

### On Debian Ubuntu

    apt-get install cmake libosmium libgdal-dev libproj-dev


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


