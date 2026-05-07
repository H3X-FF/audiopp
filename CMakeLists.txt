cmake_minimum_required(VERSION 3.28)
project(audiopp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
  add_compile_definitions(PDC_FORCE_UTF8 PDC_WIDE)

  if(MSVC)
    add_compile_options(/utf-8)
    endif()

endif()

set(SRC_FILES src/main.cpp
        src/audiomanager.cpp
        src/states.cpp
        src/ui.cpp
        src/command_mode_and_pipe.cpp
        src/file_commands.cpp
        src/sort_commands.cpp
        src/animations.cpp
        src/help.cpp
        src/vfs.cpp
        src/settings.cpp
        src/environment.cpp)

if(WIN32)
    set(PDC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/PDCursesMod")

    file(GLOB PDC_SOURCES 
        "${PDC_DIR}/pdcurses/*.c"
        "${PDC_DIR}/wincon/*.c"
    )
    list(APPEND SRC_FILES ${PDC_SOURCES})
    include_directories(${PDC_DIR})
else()
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

endif ()

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${NCURSES_LOCAL_DIR}/include)
    target_link_directories(${PROJECT_NAME} PRIVATE ${NCURSES_LOCAL_DIR}/lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE ncursesw)
endif ()