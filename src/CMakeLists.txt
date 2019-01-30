find_package(Qt5 COMPONENTS Core Quick Multimedia REQUIRED)

set(APP_SOURCES
    audio_engine/decoder.h
    audio_engine/playback.h
    audio_engine/ringbuffer.h
    audio_engine/spectrumanalyzer.h
    audio_engine/types.h
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
)
