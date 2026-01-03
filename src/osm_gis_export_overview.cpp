
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include <gdalcpp.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/geom/factory.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp> // IWYU pragma: keep
#include <osmium/io/any_input.hpp> // IWYU pragma: keep
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

struct config {
    bool add_untagged_nodes = false;
    bool add_metadata = false;
    bool verbose = false;
};

template <class TProjection>
class MyOGRHandler : public osmium::handler::Handler {

    static const std::size_t max_length_tags = 200;

    config m_cfg;

    gdalcpp::Dataset& m_dataset;
    gdalcpp::Layer m_layer_point;
    gdalcpp::Layer m_layer_linestring;
    gdalcpp::Layer m_layer_multipolygon;

    osmium::geom::OGRFactory<TProjection>& m_factory;

    void add_metadata_fields(gdalcpp::Layer& layer) {
        layer.add_field("version", OFTInteger, 7);
        layer.add_field("changeset", OFTInteger, 7);
        layer.add_field("timestamp", OFTString, 20);
        layer.add_field("uid", OFTInteger, 7);
        layer.add_field("user", OFTString, 256);
    }

    static void add_metadata(gdalcpp::Feature& feature, const osmium::OSMObject& object) {
        feature.set_field("version", int32_t(object.version()));
        feature.set_field("changeset", int32_t(object.changeset()));
        feature.set_field("timestamp", object.timestamp().to_iso().c_str());
        feature.set_field("uid", int32_t(object.uid()));
        feature.set_field("user", object.user());
    }

    static void add_tags(gdalcpp::Feature& feature, const osmium::OSMObject& object) {
        std::string tags;
        for (const auto& tag : object.tags()) {
            tags += tag.key();
            tags += "=";
            tags += tag.value();
            tags += ",";
        }
        if (!tags.empty()) {
            tags.pop_back();
        }
        feature.set_field("tags", tags.c_str());
    }

    void add_feature(gdalcpp::Feature& feature, const osmium::OSMObject& object) {
        if (m_cfg.add_metadata) {
            add_metadata(feature, object);
        }
        add_tags(feature, object);
        feature.add_to_layer();
    }

public:

    MyOGRHandler(gdalcpp::Dataset& dataset, osmium::geom::OGRFactory<TProjection>& factory, const config& cfg) :
        m_cfg(cfg),
        m_dataset(dataset),
        m_layer_point(dataset, "points", wkbPoint, {"SPATIAL_INDEX=NO"}),
        m_layer_linestring(dataset, "lines", wkbLineString, {"SPATIAL_INDEX=NO"}),
        m_layer_multipolygon(dataset, "areas", wkbMultiPolygon, {"SPATIAL_INDEX=NO"}),
        m_factory(factory) {

        m_layer_point.add_field("id", OFTReal, 10);
        m_layer_linestring.add_field("id", OFTInteger, 7);
        m_layer_multipolygon.add_field("id", OFTInteger, 7);

        m_layer_point.add_field("tags", OFTString, max_length_tags);
        m_layer_linestring.add_field("tags", OFTString, max_length_tags);
        m_layer_multipolygon.add_field("tags", OFTString, max_length_tags);

        if (m_cfg.add_metadata) {
            add_metadata_fields(m_layer_point);
            add_metadata_fields(m_layer_linestring);
            add_metadata_fields(m_layer_multipolygon);
        }
    }

    void node(const osmium::Node& node) {
        if (m_cfg.add_untagged_nodes || !node.tags().empty()) {
            gdalcpp::Feature feature{m_layer_point, m_factory.create_point(node)};
            feature.set_field("id", double(node.id()));
            add_feature(feature, node);
        }
    }

    void way(const osmium::Way& way) {
        try {
            gdalcpp::Feature feature{m_layer_linestring, m_factory.create_linestring(way)};
            feature.set_field("id", int32_t(way.id()));
            add_feature(feature, way);
        } catch (const osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

    void area(const osmium::Area& area) {
        try {
            gdalcpp::Feature feature{m_layer_multipolygon, m_factory.create_multipolygon(area)};
            feature.set_field("id", int32_t(area.id()));
            add_feature(feature, area);
        } catch (const osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for area "
                        << area.id()
                        << " created from "
                        << (area.from_way() ? "way" : "relation")
                        << " with id="
                        << area.orig_id() << ".\n";
        }
    }

};

/* ================================================== */

namespace po = boost::program_options;

void print_help(const po::options_description& desc) {
    std::cout << "osm_gis_export_overview [OPTIONS] OSM-FILE\n\n"
              << "If OSM-FILE is not used, stdin is assumed.\n\n"
              << desc << "\n";
}

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("OPTIONS");
        desc.add_options()
            ("help,h", "Print usage information")
            ("verbose,v", "Enable verbose output")
            ("output,o", po::value<std::string>(), "Output file name")
            ("output-format,f", po::value<std::string>()->default_value("SQLite"), "Output OGR format (Default: 'SQLite')")
            ("add-untagged-nodes", "Add untagged nodes to point layer")
            ("add-metadata", "Add columns for version, changeset, timestamp, uid, and user")
            ("features-per-transaction", po::value<int>()->default_value(100000), "Number of features to add per transaction")
        ;

        po::options_description hidden;
        hidden.add_options()
        ("input-filename", po::value<std::string>(), "OSM input file")
        ;

        po::options_description parsed_options;
        parsed_options.add(desc).add(hidden);

        po::positional_options_description positional;
        positional.add("input-filename", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(parsed_options).positional(positional).run(), vm);
        po::notify(vm);

        std::string input_filename;
        std::string output_filename;
        std::string output_format{"SQLite"};
        const bool debug = false;

        config cfg;

        if (vm.count("help")) {
            print_help(desc);
            return 0;
        }

        if (vm.count("verbose")) {
            cfg.verbose = true;
        }

        if (vm.count("output-format")) {
            output_format = vm["output-format"].as<std::string>();
        }

        if (vm.count("input-filename")) {
            input_filename = vm["input-filename"].as<std::string>();
        }

        if (vm.count("output")) {
            output_filename = vm["output"].as<std::string>();
        } else {
            auto slash = input_filename.rfind('/');
            if (slash == std::string::npos) {
                slash = 0;
            } else {
                ++slash;
            }
            output_filename = input_filename.substr(slash);
            auto dot = output_filename.find('.');
            if (dot != std::string::npos) {
                output_filename.erase(dot);
            }
            output_filename.append(".db");
        }

        int features_per_transaction = 0;
        if (vm.count("features-per-transaction")) {
            features_per_transaction = vm["features-per-transaction"].as<int>();
        }

        if (vm.count("add-untagged-nodes")) {
            cfg.add_untagged_nodes = true;
        }

        if (vm.count("add-metadata")) {
            cfg.add_metadata = true;
        }

        osmium::util::VerboseOutput vout{cfg.verbose};
        vout << "Writing to '" << output_filename << "'\n";

        const osmium::io::File input_file{input_filename};

        osmium::area::Assembler::config_type assembler_config;
        if (debug) {
            assembler_config.debug_level = 1;
        }
        osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config};

        vout << "Pass 1...\n";
        osmium::relations::read_relations(input_file, mp_manager);
        vout << "Pass 1 done\n";

        index_type index_pos;
        location_handler_type location_handler{index_pos};
        location_handler.ignore_errors();

        osmium::geom::OGRFactory<> factory {};

        CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");
        gdalcpp::Dataset dataset{output_format, output_filename, gdalcpp::SRS{factory.proj_string()}, { "SPATIALITE=TRUE", "INIT_WITH_EPSG=no" }};
        dataset.exec("PRAGMA journal_mode = OFF;");
        if (features_per_transaction) {
            dataset.enable_auto_transactions(features_per_transaction);
        }

        MyOGRHandler<decltype(factory)::projection_type> ogr_handler(dataset, factory, cfg);

        vout << "Pass 2...\n";
        osmium::io::Reader reader{input_file};

        osmium::apply(reader, location_handler, ogr_handler, mp_manager.handler([&ogr_handler](const osmium::memory::Buffer& area_buffer) {
            osmium::apply(area_buffer, ogr_handler);
        }));

        reader.close();
        vout << "Pass 2 done\n";

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

        const osmium::MemoryUsage memory;
        if (memory.peak()) {
            vout << "Memory used: " << memory.peak() << " MBytes\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

