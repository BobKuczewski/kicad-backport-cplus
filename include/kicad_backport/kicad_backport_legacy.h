#pragma once

#include "kicad_backport/kicad_backport.h"

#include <filesystem>
#include <string>


namespace KICAD_BACKPORT
{

bool IsLegacyKind( KIND aKind );
DOCUMENT LoadLegacyDocument( const std::filesystem::path& aPath, std::string aText );

} // namespace KICAD_BACKPORT
