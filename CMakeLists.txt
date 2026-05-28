cmake_minimum_required( VERSION 3.16 )

project( kicad-backport-cplus VERSION 0.1.1 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

add_executable( kicad-backport
    src/kicad_backport.cpp
    src/kicad_backport_document.cpp
    src/kicad_backport_report.cpp
    src/kicad_backport_rule_rewriters.cpp
    src/kicad_backport_rules.cpp
    src/kicad_backport_util.cpp
    src/kicad_backport_versions.cpp
    src/main.cpp
    src/sexpr.cpp
)

target_include_directories( kicad-backport PRIVATE include )

install( TARGETS kicad-backport
    RUNTIME DESTINATION bin
)

if( MSVC )
    target_compile_options( kicad-backport PRIVATE /W4 /permissive- )
else()
    target_compile_options( kicad-backport PRIVATE -Wall -Wextra -Wpedantic )
endif()
