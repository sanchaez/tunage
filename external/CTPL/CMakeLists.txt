add_library(CTPL INTERFACE)

target_sources(CTPL INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ctpl_stl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ctpl.h)

target_include_directories(CTPL INTERFACE include/)
