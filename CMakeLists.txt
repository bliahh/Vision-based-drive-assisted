cmake_minimum_required(VERSION 3.16)
project(adas_hud CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(glm REQUIRED)

add_executable(adas_hud
    src/main.cpp
    src/hud_renderer.cpp
    src/road_renderer.cpp
    src/lane_renderer.cpp
    src/car_renderer.cpp
    src/sign_renderer.cpp
    src/building_renderer.cpp
    src/tcp_client.cpp
    src/tcp_thread.cpp
    src/objloader.cpp
    src/person_renderer.cpp
    src/street_light_renderer.cpp
    src/star_renderer.cpp
    src/sky_renderer.cpp
    src/rain_renderer.cpp
)

target_include_directories(adas_hud PRIVATE
    include
    ${GLUT_INCLUDE_DIRS}
)

target_link_libraries(adas_hud PRIVATE
    OpenGL::GL
    GLEW::GLEW
    nlohmann_json::nlohmann_json
    glm::glm
    ${GLUT_LIBRARIES}
)

target_compile_options(adas_hud PRIVATE -Wall -Wextra -O2)
target_compile_definitions(adas_hud PRIVATE ASSET_DIR="${CMAKE_SOURCE_DIR}")