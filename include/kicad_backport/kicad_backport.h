#ifndef KICAD_BACKPORT_KICAD_BACKPORT_H
#define KICAD_BACKPORT_KICAD_BACKPORT_H

#include "kicad_backport/compat.h"
#include "kicad_backport/filesystem.h"
#include "kicad_backport/sexpr.h"

#include <cstddef>
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
    WORKSHEET,
    PROJECT,
    LEGACY_SCHEMATIC,
    LEGACY_SYMBOL_LIBRARY,
    LEGACY_SYMBOL_DOCUMENTATION,
    LEGACY_PROJECT
};

// Parsed KiCad document with its detected type and file version.
struct DOCUMENT
{
    FS::path     Path;
    std::unique_ptr<SEXPR::NODE> Root;
    KIND                      Kind = KIND::UNKNOWN;
    std::string               Version;
    size_t                    SourceBytes = 0;
    std::string               RawText;
};

// Per-file result used by convert and inspect commands.
struct FILE_REPORT
{
    std::string              Path;
    std::string              InputPath;
    std::string              OutputPath;
    std::string              Kind;
    std::string              SourceKind;
    std::string              TargetKind;
    std::string              SourceVersion;
    std::string              TargetVersion;
    bool                     Changed = false;
    std::vector<std::string> Warnings;
};

struct PROJECT_COPY_ENTRY
{
    FS::path Source;
    FS::path Output;
    bool                  IsDocument = false;
};

// Command-line front end for project/file inspection and downgrade conversion.
class CONVERTER
{
public:
    static constexpr const char* VERSION = "0.4.2";

    int Run( int aArgc, char** aArgv );

private:
    // CLI subcommands.
    int runConvert( const std::vector<std::string>& aArgs );
    int runInspect( const std::vector<std::string>& aArgs );
    int runDetectVersions( const std::vector<std::string>& aArgs );
    void printUsage() const;

    // Document IO and conversion.
    DOCUMENT loadDocument( const FS::path& aPath ) const;
    FILE_REPORT normalizeFile( const FS::path& aInput,
                               const FS::path& aOutput,
                               const std::string& aTarget,
                               bool aPrintWarnings = true );
    std::vector<FILE_REPORT> inspectPath( const FS::path& aPath ) const;
    std::vector<FILE_REPORT> detectVersionsPath( const FS::path& aPath ) const;

    void ensureVersion( DOCUMENT& aDocument, const std::string& aVersion ) const;

    // Output naming helpers.
    FS::path versionedOutputPath( const FS::path& aPath,
                                               const std::string& aTarget ) const;

    // Project tree filtering and copying.
    bool isKiCadDocumentPath( const FS::path& aPath ) const;
    bool isKiCadProjectFilePath( const FS::path& aPath ) const;
    bool isExcludedProjectDirName( const std::string& aName ) const;
    std::vector<PROJECT_COPY_ENTRY> copyProjectTree( const FS::path& aInput,
                                                     const FS::path& aOutput,
                                                     const std::string& aTarget ) const;

    void writeReport( const FS::path& aPath,
                      const std::vector<FILE_REPORT>& aReports ) const;

    // Creates legacy local board display settings for KiCad 6/7/8.
    void ensureLegacyProjectLocalSettings( const FS::path& aPath,
                                           const std::string& aTargetSuffix,
                                           const FS::path& aSourcePath = {} ) const;
};

// Detects KiCad document type from the root S-expression or file extension.
std::string KindName( KIND aKind );
KIND DetectKind( const FS::path& aPath, const std::string& aTopLevel );

} // namespace KICAD_BACKPORT

#endif // KICAD_BACKPORT_KICAD_BACKPORT_H
