echo Creating simple_build directory

mkdir -p simple_build

echo Copying source files to the simple_build directory

cp src/*.cpp simple_build
cp src/internal/*.h simple_build
cp include/kicad_backport/*.h simple_build

echo Entering simple_build directory

cd simple_build

echo Rewriting source files for new directory structure

sed -i s#kicad_backport/##g kicad_backport.cpp
sed -i s#kicad_backport/##g kicad_backport_document.cpp
sed -i s#kicad_backport/##g kicad_backport.h
sed -i s#kicad_backport/##g kicad_backport_legacy.cpp
sed -i s#kicad_backport/##g kicad_backport_legacy.h
sed -i s#kicad_backport/##g kicad_backport_pathmap.cpp
sed -i s#kicad_backport/##g kicad_backport_pathmap.h
sed -i s#kicad_backport/##g kicad_backport_report.cpp
sed -i s#kicad_backport/##g kicad_backport_report.h
sed -i s#kicad_backport/##g kicad_backport_rule_rewriters.cpp
sed -i s#internal/##g kicad_backport_rule_rewriters.cpp
sed -i s#kicad_backport/##g kicad_backport_rule_rewriters.h
sed -i s#internal/##g kicad_backport_rule_rewriters.h
sed -i s#kicad_backport/##g kicad_backport_rules.cpp
sed -i s#internal/##g kicad_backport_rules.cpp
sed -i s#kicad_backport/##g kicad_backport_rules.h
sed -i s#kicad_backport/##g kicad_backport_upgrade.cpp
sed -i s#kicad_backport/##g kicad_backport_upgrade.h
sed -i s#kicad_backport/##g kicad_backport_util.cpp
sed -i s#kicad_backport/##g kicad_backport_util.h
sed -i s#kicad_backport/##g kicad_backport_versions.cpp
sed -i s#kicad_backport/##g kicad_backport_versions.h
sed -i s#kicad_backport/##g main.cpp
sed -i s#kicad_backport/##g sexpr.cpp
sed -i s#kicad_backport/##g sexpr.h

echo Compiling source files

g++ -std=c++17 -c kicad_backport.cpp
g++ -std=c++17 -c kicad_backport_document.cpp
g++ -std=c++17 -c kicad_backport_legacy.cpp
g++ -std=c++17 -c kicad_backport_pathmap.cpp
g++ -std=c++17 -c kicad_backport_report.cpp
g++ -std=c++17 -c kicad_backport_rule_rewriters.cpp
g++ -std=c++17 -c kicad_backport_rules.cpp
g++ -std=c++17 -c kicad_backport_upgrade.cpp
g++ -std=c++17 -c kicad_backport_util.cpp
g++ -std=c++17 -c kicad_backport_versions.cpp
g++ -std=c++17 -c main.cpp
g++ -std=c++17 -c sexpr.cpp

echo Linking program as main

g++ -std=c++17 -pthread -o main main.o kicad_backport_document.o kicad_backport.o kicad_backport_report.o kicad_backport_rules.o kicad_backport_util.o kicad_backport_legacy.o kicad_backport_pathmap.o kicad_backport_rule_rewriters.o kicad_backport_upgrade.o kicad_backport_versions.o sexpr.o

echo Returning to parent directory

cd ..
