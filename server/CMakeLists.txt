
#add the include folder, so that the compiler will find all files there
include_directories(include)

#take all .c files from src folder and add them to the SOURCE_FILE variable
file(GLOB SOURCE_FILES src/*.cpp)
#take all .h files from include folder and add them to the HEADER_FILES variable
file(GLOB HEADER_FILES include/*.h)

#define the output executable and the files from which it is created
add_executable(server ${SOURCE_FILES} ${HEADER_FILES})

#add dynamic linked libraries
target_link_libraries (server ${CMAKE_THREAD_LIBS_INIT})
