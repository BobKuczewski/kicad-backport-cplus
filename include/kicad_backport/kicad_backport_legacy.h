#pragma once

#include "kicad_backport/kicad_backport.h"

#include <filesystem>
#include <string>
#include <vector>


namespace KICAD_BACKPORT
{

bool IsLegacyKind( KIND aKind );
DOCUMENT LoadLegacyDocument( const std::filesystem::path& aPath, std::string aText );
KIND SexprKindForLegacyKind( KIND aKind );
KIND LegacyKindForSexprKind( KIND aKind );
int LegacyDocumentMajorVersion( const DOCUMENT& aDocument );

std::string LegacyTargetVersionForKind( KIND aKind, int aTargetMajor );
std::string ConvertLegacyToSexprText( const DOCUMENT& aDocument, const std::string& aTargetVersion,
                                      KIND* aTargetKind, std::vector<std::string>* aWarnings );
std::string ConvertSexprToLegacyText( const DOCUMENT& aDocument, int aTargetMajor,
                                      KIND* aTargetKind, std::vector<std::string>* aWarnings );
std::string RewriteLegacyTextForTarget( const DOCUMENT& aDocument, int aTargetMajor,
                                        std::vector<std::string>* aWarnings );
std::string LegacyDocumentationSidecarText( const DOCUMENT& aDocument, int aTargetMajor,
                                            std::vector<std::string>* aWarnings );

} // namespace KICAD_BACKPORT
