#pragma once

#include "kicad_backport/sexpr.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>


namespace KICAD_BACKPORT
{

// Supported KiCad S-expression document families.
enum class KIND
{
    UNKNOWN,
    SYMBOL_LIBRARY,
    SCHEMATIC,
    BOARD,
    FOOTPRINT,
    DESIGN_RULES,
    WORKSHEET
};

// Parsed KiCad document with its detected type and file version.
struct DOCUMENT
{
    std::filesystem::path     Path;
    std::unique_ptr<SEXPR::NODE> Root;
    KIND                      Kind = KIND::UNKNOWN;
    std::string               Version;
};

// Per-file result used by convert and inspect commands.
struct FILE_REPORT
{
    std::string              Path;
    std::string              Kind;
    std::string              SourceVersion;
    std::string              TargetVersion;
    bool                     Changed = false;
    std::vector<std::string> Warnings;
};

// Command-line front end for project/file inspection and downgrade conversion.
class CONVERTER
{
public:
    static constexpr const char* VERSION = "0.1.1";

    int Run( int aArgc, char** aArgv );

private:
    // CLI subcommands.
    int runConvert( const std::vector<std::string>& aArgs );
    int runInspect( const std::vector<std::string>& aArgs );
    void printUsage() const;

    // Document IO and conversion.
    DOCUMENT loadDocument( const std::filesystem::path& aPath ) const;
    FILE_REPORT normalizeFile( const std::filesystem::path& aInput,
                               const std::filesystem::path& aOutput,
                               const std::string& aTarget );
    std::vector<FILE_REPORT> inspectPath( const std::filesystem::path& aPath ) const;

    void ensureVersion( DOCUMENT& aDocument, const std::string& aVersion ) const;

    // Output naming helpers.
    std::filesystem::path versionedOutputPath( const std::filesystem::path& aPath,
                                               const std::string& aTarget ) const;

    // Project tree filtering and copying.
    bool isKiCadDocumentPath( const std::filesystem::path& aPath ) const;
    bool isKiCadProjectFilePath( const std::filesystem::path& aPath ) const;
    bool isExcludedProjectDirName( const std::string& aName ) const;
    std::vector<std::filesystem::path> copyProjectTree( const std::filesystem::path& aInput,
                                                        const std::filesystem::path& aOutput ) const;

    void writeReport( const std::filesystem::path& aPath,
                      const std::vector<FILE_REPORT>& aReports ) const;

    // Creates legacy local board display settings for KiCad 7/8.
    void ensureLegacyProjectLocalSettings( const std::filesystem::path& aPath,
                                           const std::string& aTargetSuffix ) const;
};

// Detects KiCad document type from the root S-expression or file extension.
std::string KindName( KIND aKind );
KIND DetectKind( const std::filesystem::path& aPath, const std::string& aTopLevel );

} // namespace KICAD_BACKPORT
