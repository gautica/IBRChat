cmake_minimum_required(VERSION 2.8.0)

project(IBRChat)

set (CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

#the cmake way to activate a well known library (here pthreads)
find_package (Threads)

add_subdirectory(server)

#add_subdirectory(client)
