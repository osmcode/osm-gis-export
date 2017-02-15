#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(osm_gis_export_overview osm_gis_export_overview.cpp)
target_link_libraries(osm_gis_export_overview ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES})
set_pthread_on_target(osm_gis_export_overview)
install(TARGETS osm_gis_export_overview DESTINATION bin)

add_executable(osmium_toogr osmium_toogr.cpp)
target_link_libraries(osmium_toogr ${OSMIUM_LIBRARIES})
set_pthread_on_target(osmium_toogr)
install(TARGETS osmium_toogr DESTINATION bin)

add_executable(osmium_toogr2 osmium_toogr2.cpp)
target_link_libraries(osmium_toogr2 ${OSMIUM_LIBRARIES})
set_pthread_on_target(osmium_toogr2)
install(TARGETS osmium_toogr2 DESTINATION bin)

add_executable(osmium_toogr2_exp osmium_toogr2_exp.cpp)
target_link_libraries(osmium_toogr2_exp ${OSMIUM_LIBRARIES})
set_pthread_on_target(osmium_toogr2_exp)
install(TARGETS osmium_toogr2_exp DESTINATION bin)

