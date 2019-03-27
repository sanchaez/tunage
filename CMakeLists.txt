cmake_minimum_required(VERSION 3.10)

project(tunage LANGUAGES CXX C)

# helper functions and definitions
include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

# fix library build warnings with ninja
if(POLICY CMP0058)
    cmake_policy(SET CMP0058 NEW)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_STATIC_RUNTIME OFF)
if(MSVC AND ENABLE_STATIC_RUNTIME)
  foreach(flag_var
          CMAKE_CXX_FLAGS
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL
          CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endforeach(flag_var)
endif()

# main app
set(APP_SOURCES_DIR src)
set(APP_INSTALL_DIR ${CMAKE_BINARY_DIR}/tunage)
add_subdirectory(${APP_SOURCES_DIR})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${APP_INSTALL_DIR}
    LIBRARY DESTINATION ${APP_INSTALL_DIR}/lib
    ARCHIVE DESTINATION ${APP_INSTALL_DIR}/lib/static)

install(DIRECTORY sample shaders
    DESTINATION ${APP_INSTALL_DIR})

# external projects
set(EXTERNAL_PROJECTS_DIR external)

set(kissfft_DIR ${EXTERNAL_PROJECTS_DIR}/kissfft)
set(taglib_DIR ${EXTERNAL_PROJECTS_DIR}/taglib)
set(portaudio_DIR ${EXTERNAL_PROJECTS_DIR}/portaudio)
set(libnyquist_DIR ${EXTERNAL_PROJECTS_DIR}/libnyquist)
set(CTPL_DIR ${EXTERNAL_PROJECTS_DIR}/CTPL)

add_subdirectory(${kissfft_DIR})
add_subdirectory(${taglib_DIR})
add_subdirectory(${libnyquist_DIR})
add_subdirectory(${portaudio_DIR})
add_subdirectory(${CTPL_DIR})
