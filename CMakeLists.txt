cmake_minimum_required(VERSION 3.16)
project(FpsServer)

set(CMAKE_CXX_STANDARD 17)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(fps_server 
    main.cpp
    player.cpp
    GameManager.cpp
    PhysicsEngine.cpp
    Monitor.cpp
)

target_link_libraries(fps_server Boost::system pthread)
