cmake_minimum_required(VERSION 3.2)

project(Welle.Io VERSION 0.0.0 LANGUAGES C CXX)

if(NOT WELLE-IO_VERSION)
  set(WELLE-IO_VERSION ${PROJECT_VERSION})
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_WELLE_IO    "Build Welle.io"                        OFF )
option(BUILD_WELLE_CLI   "Build welle-cli"                       ON  )
option(WITH_APP_BUNDLE   "Enable Application Bundle for macOS"   OFF )
option(KISS_FFT          "KISS FFT instead of FFTW"              OFF )
option(PROFILING         "Enable profiling (see README.md)"      OFF )
option(AIRSPY            "Compile with Airspy support"           OFF )
option(RTLSDR            "Compile with RTL-SDR support"          ON  )
option(SOAPYSDR          "Compile with SoapySDR support"         OFF )

add_definitions(-Wall)
add_definitions(-g)
add_definitions(-DDABLIN_AAC_FAAD2)

if(MINGW)
    add_definitions(-municode)
endif()

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

#enable_testing()

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

if(APPLE AND WITH_APP_BUNDLE)
    if(NOT DEFINED BUNDLE_INSTALL_DIR)
        set(BUNDLE_INSTALL_DIR "/Applications")
    endif()

    set(GUI_INSTALL_DIR "${BUNDLE_INSTALL_DIR}")
else()
    include(GNUInstallDirs)

    set(GUI_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
endif()

if(BUILD_WELLE_IO)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt5 COMPONENTS Widgets Quick QuickControls2 Multimedia Charts Qml DBus REQUIRED)
endif()

if(PROFILING)
    add_definitions(-DWITH_PROFILING)
endif()

if(KISS_FFT)
    add_definitions(-DKISSFFT)
    set(fft_sources src/libs/kiss_fft/kiss_fft.c)
    set(KISS_INCLUDE_DIRS src/libs/kiss_fft)
else()
    find_package(FFTW3f REQUIRED)
    set(fft_sources "")
    set(KISS_INCLUDE_DIRS "")
endif()

find_package(Threads REQUIRED)
find_package(Faad REQUIRED)
find_package(MPG123 REQUIRED)

if (RTLSDR)
    find_package(LibRTLSDR REQUIRED)
endif()

if (AIRSPY)
    find_package(LibAIRSPY REQUIRED)
endif()

include_directories(
    src
    src/backend
    src/output
    src/various
    src/input
    src/libs/fec
    ${FFTW3F_INCLUDE_DIRS}
    ${KISS_INCLUDE_DIRS}
    ${FAAD_INCLUDE_DIRS}
    ${LIBRTLSDR_INCLUDE_DIRS}
)

set(backend_sources
    src/backend/dab-audio.cpp
    src/backend/decoder_adapter.cpp
    src/backend/dabplus_decoder.cpp
    src/backend/charsets.cpp
    src/backend/dab-constants.cpp
    src/backend/mot_manager.cpp
    src/backend/pad_decoder.cpp
    src/backend/eep-protection.cpp
    src/backend/fib-processor.cpp
    src/backend/fic-handler.cpp
    src/backend/msc-handler.cpp
    src/backend/freq-interleaver.cpp
    src/backend/ofdm-decoder.cpp
    src/backend/ofdm-processor.cpp
    src/backend/phasereference.cpp
    src/backend/phasetable.cpp
    src/backend/tii-decoder.cpp
    src/backend/protTables.cpp
    src/backend/radio-receiver.cpp
    src/backend/tools.cpp
    src/backend/uep-protection.cpp
    src/backend/viterbi.cpp
    src/various/Xtan2.cpp
    src/various/channels.cpp
    src/various/fft.cpp
    src/various/profiling.cpp
    src/various/wavfile.c
    src/libs/fec/decode_rs_char.c
    src/libs/fec/encode_rs_char.c
    src/libs/fec/init_rs_char.c
)

set(welle_cli_sources
    src/welle-cli/welle-cli.cpp
)

set(input_sources
    src/input/input_factory.cpp
    src/input/null_device.cpp
)

if(LIBRTLSDR_FOUND)
    add_definitions (-DHAVE_RTLSDR)
    set(input_sources  ${input_sources} src/input/rtl_sdr.cpp)
endif()

if(LIBAIRSPY_FOUND)
    add_definitions (-DHAVE_AIRSPY)
    set(input_sources  ${input_sources} src/input/airspy_sdr.cpp)
endif()

STRING(TIMESTAMP BUILD_DATE "%s" UTC)
add_definitions("-DBUILD_DATE=\"${BUILD_DATE}\"")

if(BUILD_WELLE_CLI)
    set(cliExecutableName welle-cli)
    add_executable (${cliExecutableName} ${welle_cli_sources} ${backend_sources} ${input_sources} ${fft_sources})

    if(CMAKE_BUILD_TYPE MATCHES Debug)
      SET_TARGET_PROPERTIES(${cliExecutableName} PROPERTIES COMPILE_FLAGS "-O2 -fno-omit-frame-pointer -fsanitize=address")
      SET_TARGET_PROPERTIES(${cliExecutableName} PROPERTIES LINK_FLAGS "-fno-omit-frame-pointer -fsanitize=address")
    endif(CMAKE_BUILD_TYPE MATCHES Debug)

    target_link_libraries (${cliExecutableName}
      ${LIBRTLSDR_LIBRARIES}
      ${LIBAIRSPY_LIBRARIES}
      ${FFTW3F_LIBRARIES}
      ${FAAD_LIBRARIES}
      ${MPG123_LIBRARIES}
      Threads::Threads
    )

    if(UNIX)
        INSTALL (TARGETS ${cliExecutableName} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
