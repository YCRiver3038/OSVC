cmake_minimum_required(VERSION 3.12)
project(osvc)

add_executable(osvc src/main.cpp)
add_executable(remocon src/rcom.cpp)
add_library(NW STATIC src/network.cpp)

find_package(PkgConfig)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(RUBBERBAND REQUIRED rubberband)
#pkg_check_modules(JSON REQUIRED nlohmann_json)

#target_include_directories(osvc PUBLIC ${PORTAUDIO_INCLUDE_DIRS} ${RUBBERBAND_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS})
target_include_directories(osvc PUBLIC ${PORTAUDIO_INCLUDE_DIRS} ${RUBBERBAND_INCLUDE_DIRS})

target_compile_options(osvc PUBLIC --std=c++17 -O3)
target_compile_options(remocon PUBLIC --std=c++17 -O3)
target_compile_options(NW PUBLIC --std=c++17 -O3)

if(WIN32)
    target_link_libraries(osvc NW wsock32 ws2_32 ${PORTAUDIO_LIBRARIES} ${RUBBERBAND_LIBRARIES})
else()
    target_link_libraries(osvc NW ${PORTAUDIO_LIBRARIES} ${RUBBERBAND_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(remocon NW wsock32 ws2_32)
else()
    target_link_libraries(remocon NW)
endif()
