find_package(Qt5 COMPONENTS Core Quick REQUIRED)

add_library(AudioEngine INTERFACE)

target_sources(AudioEngine INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine/decoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine/playback.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine/ringbuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine/spectrumanalyzer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine/types.h)

target_include_directories(AudioEngine INTERFACE include/audio_engine)

set(APP_SOURCES
    applicationcontroller.cpp
    applicationcontroller.h
    visualisationrenderer.cpp
    visualisationrenderer.h
    playbackengine.cpp
    playbackengine.h
    playlistitemmodel.cpp
    playlistitemmodel.h
    audiotaginfo.cpp
    audiotaginfo.h
    main.cpp
    qml.qrc
    tunage.rc)

if (WIN32)
    set(WIN32_ON_OFF_SWITCH "WIN32")
else ()
    set(WIN32_ON_OFF_SWITCH "")
endif ()

add_executable(${PROJECT_NAME}
    ${WIN32_ON_OFF_SWITCH}
    ${APP_SOURCES})

add_definitions(-DTAGLIB_STATIC)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Quick
    tag_static
    libnyquist
    portaudio_static
    kissfft_static
    CTPL
    AudioEngine
)
