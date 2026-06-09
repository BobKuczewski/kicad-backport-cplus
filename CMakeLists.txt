cmake_minimum_required( VERSION 3.10 )

project( kicad-backport-cplus VERSION 0.4.1 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )
set( KICAD_BACKPORT_STATIC_RUNTIME "ON" CACHE STRING "Link libstdc++/libgcc statically on Linux when supported." )
set_property( CACHE KICAD_BACKPORT_STATIC_RUNTIME PROPERTY STRINGS ON OFF )

set( KICAD_BACKPORT_SOURCE_LIST cmake/kicad_backport_sources.txt )
set_property( DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${KICAD_BACKPORT_SOURCE_LIST} )
file( STRINGS ${KICAD_BACKPORT_SOURCE_LIST} KICAD_BACKPORT_SOURCES )

add_executable( kicad-backport ${KICAD_BACKPORT_SOURCES} )

find_package( Threads REQUIRED )

target_include_directories( kicad-backport
    PRIVATE
        include
        src
)

install( TARGETS kicad-backport
    RUNTIME DESTINATION bin
)

if( MSVC )
    target_compile_options( kicad-backport PRIVATE /W4 /permissive- )
else()
    target_compile_options( kicad-backport PRIVATE -Wall -Wextra -Wpedantic )
    target_link_libraries( kicad-backport PRIVATE Threads::Threads )
    if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9 )
        target_link_libraries( kicad-backport PRIVATE stdc++fs )
    endif()
    if( CMAKE_SYSTEM_NAME STREQUAL "Linux" AND KICAD_BACKPORT_STATIC_RUNTIME )
        set_property( TARGET kicad-backport APPEND_STRING PROPERTY
            LINK_FLAGS " -static-libstdc++ -static-libgcc" )
    endif()
endif()
