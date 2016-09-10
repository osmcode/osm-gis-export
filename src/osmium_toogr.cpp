/*

  This is an example tool that converts OSM data to some output format
  like Spatialite or Shapefiles using the OGR library.

*/

#include <iostream>
#include <getopt.h>

#include <gdalcpp.hpp>

#include <osmium/index/map/all.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>

#include <osmium/geom/ogr.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

class MyOGRHandler : public osmium::handler::Handler {

    gdalcpp::Layer m_layer_point;
    gdalcpp::Layer m_layer_linestring;

    osmium::geom::OGRFactory<> m_factory;

public:

    explicit MyOGRHandler(gdalcpp::Dataset& dataset) :
        m_layer_point(dataset, "postboxes", wkbPoint),
        m_layer_linestring(dataset, "roads", wkbLineString) {

        m_layer_point.add_field("id", OFTReal, 10);
        m_layer_point.add_field("operator", OFTString, 30);

        m_layer_linestring.add_field("id", OFTReal, 10);
        m_layer_linestring.add_field("type", OFTString, 30);
    }

    void node(const osmium::Node& node) {
        const char* amenity = node.tags().get_value_by_key("amenity");
        if (amenity && !strcmp(amenity, "post_box")) {
            gdalcpp::Feature feature(m_layer_point, m_factory.create_point(node));
            feature.set_field("id", static_cast<double>(node.id()));
            feature.set_field("operator", node.tags().get_value_by_key("operator"));
            feature.add_to_layer();
        }
    }

    void way(const osmium::Way& way) {
        const char* highway = way.tags().get_value_by_key("highway");
        if (highway) {
            try {
                gdalcpp::Feature feature(m_layer_linestring, m_factory.create_linestring(way));
                feature.set_field("id", static_cast<double>(way.id()));
                feature.set_field("type", highway);
                feature.add_to_layer();
            } catch (osmium::geometry_error&) {
                std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
            }
        }
    }

};

/* ================================================== */

void print_help() {
    std::cout << "osmium_toogr [OPTIONS] [INFILE [OUTFILE]]\n\n" \
              << "If INFILE is not given stdin is assumed.\n" \
              << "If OUTFILE is not given 'ogr_out' is used.\n" \
              << "\nOptions:\n" \
              << "  -h, --help                 This help message\n" \
              << "  -l, --location_store=TYPE  Set location store\n" \
              << "  -f, --format=FORMAT        Output OGR format (Default: 'SQLite')\n" \
              << "  -L                         See available location stores\n";
}

int main(int argc, char* argv[]) {
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    static struct option long_options[] = {
        {"help",                 no_argument,       0, 'h'},
        {"format",               required_argument, 0, 'f'},
        {"location_store",       required_argument, 0, 'l'},
        {"list_location_stores", no_argument,       0, 'L'},
        {0, 0, 0, 0}
    };

    std::string output_format{"SQLite"};
    std::string location_store{"sparse_mem_array"};

    while (true) {
        int c = getopt_long(argc, argv, "hf:l:L", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(0);
            case 'f':
                output_format = optarg;
                break;
            case 'l':
                location_store = optarg;
                break;
            case 'L':
                std::cout << "Available map types:\n";
                for (const auto& map_type : map_factory.map_types()) {
                    std::cout << "  " << map_type << "\n";
                }
                std::exit(0);
            default:
                std::exit(1);
        }
    }

    std::string input_filename;
    std::string output_filename("ogr_out");
    int remaining_args = argc - optind;
    if (remaining_args > 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE [OUTFILE]]" << std::endl;
        std::exit(1);
    } else if (remaining_args == 2) {
        input_filename =  argv[optind];
        output_filename = argv[optind+1];
    } else if (remaining_args == 1) {
        input_filename =  argv[optind];
    } else {
        input_filename = "-";
    }

    osmium::io::Reader reader(input_filename);

    std::unique_ptr<index_type> index = map_factory.create_map(location_store);
    location_handler_type location_handler{*index};
    location_handler.ignore_errors();

    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");
    gdalcpp::Dataset dataset{output_format, output_filename, gdalcpp::SRS{}, { "SPATIALITE=TRUE", "INIT_WITH_EPSG=no" }};
    MyOGRHandler ogr_handler{dataset};

    osmium::apply(reader, location_handler, ogr_handler);
    reader.close();

    int locations_fd = open("locations.dump", O_WRONLY | O_CREAT, 0644);
    if (locations_fd < 0) {
        throw std::system_error(errno, std::system_category(), "Open failed");
    }
    index->dump_as_list(locations_fd);
    close(locations_fd);
}

