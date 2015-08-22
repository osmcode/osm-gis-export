#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(osmium_toogr osmium_toogr.cpp)
target_link_libraries(osmium_toogr ${OSMIUM_LIBRARIES})
install(TARGETS osmium_toogr DESTINATION bin)

add_executable(osmium_toogr2 osmium_toogr2.cpp)
target_link_libraries(osmium_toogr2 ${OSMIUM_LIBRARIES})
install(TARGETS osmium_toogr2 DESTINATION bin)

add_executable(osmium_toogr2_exp osmium_toogr2_exp.cpp)
target_link_libraries(osmium_toogr2_exp ${OSMIUM_LIBRARIES})
install(TARGETS osmium_toogr2_exp DESTINATION bin)

