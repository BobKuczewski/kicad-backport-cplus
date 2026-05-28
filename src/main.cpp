#include "kicad_backport/kicad_backport.h"


int main( int argc, char** argv )
{
    KICAD_BACKPORT::CONVERTER converter;
    return converter.Run( argc, argv );
}
