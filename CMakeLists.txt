cmake_minimum_required(VERSION 3.28)
project(audiopp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NCURSES_LOCAL_DIR "${CMAKE_BINARY_DIR}/ncurses_pkg")

execute_process(
        COMMAND ./configure --prefix=${NCURSES_LOCAL_DIR} --with-static --enable-widec
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/ncurses
)

execute_process(
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/ncurses
)

execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/ncurses)

set(USR_HOME_DIR $ENV{HOME})

set(SRC_FILES src/main.cpp
        src/audiomanager.cpp
        src/states.cpp
        src/tuimanager.cpp
        src/commandline.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_PATH=\"${USR_HOME_DIR}/.local/audiopp\")

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)

target_include_directories(${PROJECT_NAME} PRIVATE ${NCURSES_LOCAL_DIR}/include)
target_link_directories(${PROJECT_NAME} PRIVATE ${NCURSES_LOCAL_DIR}/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE ncursesw)