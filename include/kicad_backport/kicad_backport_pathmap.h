#pragma once

#include "kicad_backport/kicad_backport.h"

#include <filesystem>
#include <string>


namespace KICAD_BACKPORT
{

std::string TargetExtensionForKind( KIND aKind );
std::filesystem::path WithTargetExtension( const std::filesystem::path& aPath, KIND aTargetKind );

} // namespace KICAD_BACKPORT
