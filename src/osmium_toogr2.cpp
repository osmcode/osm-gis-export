/*

  This is an example tool that converts OSM data to some output format
  like Spatialite or Shapefiles using the OGR library.

  This version does multipolygon handling (in contrast to the osmium_toogr
  example which doesn't).

*/

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

#include <gdalcpp.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp> // IWYU pragma: keep
#include <osmium/io/any_input.hpp> // IWYU pragma: keep
#include <osmium/util/memory.hpp>
#include <osmium/visitor.hpp>

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

template <class TProjection>
class MyOGRHandler : public osmium::handler::Handler {

    gdalcpp::Layer m_layer_point;
    gdalcpp::Layer m_layer_linestring;
    gdalcpp::Layer m_layer_polygon;

    osmium::geom::OGRFactory<TProjection>& m_factory;

public:

    MyOGRHandler(gdalcpp::Dataset& dataset, osmium::geom::OGRFactory<TProjection>& factory) :
        m_layer_point(dataset, "postboxes", wkbPoint),
        m_layer_linestring(dataset, "roads", wkbLineString),
        m_layer_polygon(dataset, "buildings", wkbMultiPolygon),
        m_factory(factory) {

        m_layer_point.add_field("id", OFTReal, 10);
        m_layer_point.add_field("operator", OFTString, 30);

        m_layer_linestring.add_field("id", OFTReal, 10);
        m_layer_linestring.add_field("type", OFTString, 30);

        m_layer_polygon.add_field("id", OFTReal, 10);
        m_layer_polygon.add_field("type", OFTString, 30);
    }

    void node(const osmium::Node& node) {
        const char* amenity = node.tags()["amenity"];
        if (amenity && !std::strcmp(amenity, "post_box")) {
            gdalcpp::Feature feature{m_layer_point, m_factory.create_point(node)};
            feature.set_field("id", static_cast<double>(node.id()));
            feature.set_field("operator", node.tags().get_value_by_key("operator"));
            feature.add_to_layer();
        }
    }

    void way(const osmium::Way& way) {
        const char* highway = way.tags()["highway"];
        if (highway) {
            try {
                gdalcpp::Feature feature{m_layer_linestring, m_factory.create_linestring(way)};
                feature.set_field("id", static_cast<double>(way.id()));
                feature.set_field("type", highway);
                feature.add_to_layer();
            } catch (const osmium::geometry_error&) {
                std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
            }
        }
    }

    void area(const osmium::Area& area) {
        const char* building = area.tags()["building"];
        if (building) {
            try {
                gdalcpp::Feature feature{m_layer_polygon, m_factory.create_multipolygon(area)};
                feature.set_field("id", static_cast<double>(area.id()));
                feature.set_field("type", building);
                feature.add_to_layer();
            } catch (const osmium::geometry_error&) {
                std::cerr << "Ignoring illegal geometry for area "
                          << area.id()
                          << " created from "
                          << (area.from_way() ? "way" : "relation")
                          << " with id="
                          << area.orig_id() << ".\n";
            }
        }
    }

};

/* ================================================== */

void print_help() {
    std::cout << "osmium_toogr2 [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE is not given stdin is assumed.\n" \
              << "If OUTFILE is not given 'ogr_out' is used.\n" \
              << "\nOptions:\n" \
              << "  -h, --help           This help message\n" \
              << "  -d, --debug          Enable debug output\n" \
              << "  -f, --format=FORMAT  Output OGR format (Default: 'SQLite')\n";
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",   no_argument, 0, 'h'},
        {"debug",  no_argument, 0, 'd'},
        {"format", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    std::string output_format{"SQLite"};
    bool debug = false;

    while (true) {
        int c = getopt_long(argc, argv, "hdf:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(0);
            case 'd':
                debug = true;
                break;
            case 'f':
                output_format = optarg;
                break;
            default:
                std::exit(1);
        }
    }

    std::string input_filename;
    std::string output_filename{"ogr_out"};
    int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]\n";
        std::exit(1);
    } else if (remaining_args == 2) {
        input_filename =  argv[optind];
        output_filename = argv[optind+1];
    } else if (remaining_args == 1) {
        input_filename =  argv[optind];
    } else {
        input_filename = "-";
    }

    osmium::io::File input_file{input_filename};

    osmium::area::Assembler::config_type assembler_config;
    if (debug) {
        assembler_config.debug_level = 1;
    }
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config};

    std::cerr << "Pass 1...\n";
    osmium::relations::read_relations(input_file, mp_manager);
    std::cerr << "Pass 1 done\n";

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    // Choose one of the following:

    // 1. Use WGS84, do not project coordinates.
    //osmium::geom::OGRFactory<> factory {};

    // 2. Project coordinates into "Web Mercator".
    osmium::geom::OGRFactory<osmium::geom::MercatorProjection> factory;

    // 3. Use any projection that the proj library can handle.
    //    (Initialize projection with EPSG code or proj string).
    //    In addition you need to link with "-lproj" and add
    //    #include <osmium/geom/projection.hpp>.
    //osmium::geom::OGRFactory<osmium::geom::Projection> factory {osmium::geom::Projection(3857)};

    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");
    gdalcpp::Dataset dataset{output_format, output_filename, gdalcpp::SRS{factory.proj_string()}, { "SPATIALITE=TRUE", "INIT_WITH_EPSG=no" }};
    MyOGRHandler<decltype(factory)::projection_type> ogr_handler{dataset, factory};

    std::cerr << "Pass 2...\n";
    osmium::io::Reader reader{input_file};

    osmium::apply(reader, location_handler, ogr_handler, mp_manager.handler([&ogr_handler](const osmium::memory::Buffer& area_buffer) {
        osmium::apply(area_buffer, ogr_handler);
    }));

    reader.close();
    std::cerr << "Pass 2 done\n";

    std::vector<osmium::object_id_type> incomplete_relations_ids;
    mp_manager.for_each_incomplete_relation([&](const osmium::relations::RelationHandle& handle){
        incomplete_relations_ids.push_back(handle->id());
    });
    if (!incomplete_relations_ids.empty()) {
        std::cerr << "Warning! Some member ways missing for these multipolygon relations:";
        for (const auto id : incomplete_relations_ids) {
            std::cerr << " " << id;
        }
        std::cerr << "\n";
    }

    osmium::MemoryUsage memory;
    if (memory.peak()) {
        std::cerr << "Memory used: " << memory.peak() << " MBytes\n";
    }
}

