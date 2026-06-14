#include "kicad_backport/kicad_backport.h"
#include "kicad_backport/kicad_backport_legacy.h"
#include "kicad_backport/kicad_backport_pathmap.h"
#include "kicad_backport/kicad_backport_report.h"
#include "kicad_backport/kicad_backport_rules.h"
#include "kicad_backport/kicad_backport_upgrade.h"
#include "kicad_backport/kicad_backport_util.h"
#include "kicad_backport/kicad_backport_versions.h"

#ifndef KICAD_BACKPORT_WITH_ZSTD
#define KICAD_BACKPORT_WITH_ZSTD 0
#endif

#if KICAD_BACKPORT_WITH_ZSTD
#include "third_party/zstd/lib/zstd.h"
#endif

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>


namespace KICAD_BACKPORT
{

namespace
{

using CLOCK = std::chrono::steady_clock;

struct EMBEDDED_FILE
{
    std::string Name;
    std::string Type;
    std::string Data;
};

struct EMBEDDED_MODEL_EXPORT_RESULT
{
    int Extracted = 0;
    int Rewritten = 0;
    int Removed = 0;
    std::vector<std::string> Warnings;
};

long long elapsedMicros( CLOCK::time_point aStart, CLOCK::time_point aEnd )
{
    return std::chrono::duration_cast<std::chrono::microseconds>( aEnd - aStart ).count();
}


bool timingEnabled()
{
#if defined( _MSC_VER )
    char* value = nullptr;
    size_t len = 0;
    _dupenv_s( &value, &len, "KICAD_BACKPORT_TIMING" );
    bool enabled = value && value[0] && std::string( value ) != "0";
    std::free( value );
    return enabled;
#else
    const char* value = std::getenv( "KICAD_BACKPORT_TIMING" );
    return value && value[0] && std::string( value ) != "0";
#endif
}


bool isLibraryTablePath( const FS::path& aPath )
{
    std::string name = Lower( aPath.filename().string() );
    return name == "sym-lib-table" || name == "fp-lib-table";
}


bool isPathSeparator( char aChar )
{
    return aChar == '/' || aChar == '\\';
}


std::string safeEmbeddedFilename( const std::string& aName )
{
    std::string out;

    for( char ch : aName )
    {
        unsigned char c = static_cast<unsigned char>( ch );

        if( ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>'
            || ch == '|' || isPathSeparator( ch ) || c < 32 )
        {
            out.push_back( '_' );
        }
        else
        {
            out.push_back( ch );
        }
    }

    out = Trim( out );

    while( !out.empty() && ( out.back() == '.' || out.back() == ' ' ) )
        out.pop_back();

    if( out.empty() || out == "." || out == ".." )
        return "embedded_model.step";

    return out;
}


std::string embeddedUriFilename( const std::string& aValue )
{
    static const std::string prefix = "kicad-embed://";

    if( !StartsWith( aValue, prefix ) )
        return "";

    std::string name = aValue.substr( prefix.size() );

    while( !name.empty() && isPathSeparator( name.front() ) )
        name.erase( name.begin() );

    return safeEmbeddedFilename( name );
}


bool isModelFileName( const std::string& aName )
{
    std::string ext = Lower( FS::path( aName ).extension().string() );
    static const std::set<std::string> modelExts = {
        ".step", ".stp", ".wrl", ".iges", ".igs", ".stl", ".obj"
    };

    return modelExts.count( ext ) != 0;
}


std::string embeddedDataAtomText( const SEXPR::NODE* aData )
{
    if( !aData || aData->IsAtom() || aData->HeadView() != "data" )
        return "";

    std::string out;

    for( size_t i = 1; i < aData->Children.size(); ++i )
    {
        const std::unique_ptr<SEXPR::NODE>& child = aData->Children[i];

        if( !child || !child->IsAtom() )
            continue;

        std::string atom = child->Atom;

        if( !atom.empty() && atom.front() == '|' )
            atom.erase( atom.begin() );

        if( !atom.empty() && atom.back() == '|' )
            atom.pop_back();

        out += atom;
    }

    return out;
}


int base64Value( char aChar )
{
    if( aChar >= 'A' && aChar <= 'Z' )
        return aChar - 'A';

    if( aChar >= 'a' && aChar <= 'z' )
        return aChar - 'a' + 26;

    if( aChar >= '0' && aChar <= '9' )
        return aChar - '0' + 52;

    if( aChar == '+' )
        return 62;

    if( aChar == '/' )
        return 63;

    return -1;
}


bool decodeBase64( const std::string& aInput, std::string& aOutput )
{
    aOutput.clear();
    int value = 0;
    int bits = -8;

    for( char ch : aInput )
    {
        if( std::isspace( static_cast<unsigned char>( ch ) ) )
            continue;

        if( ch == '=' )
            break;

        int decoded = base64Value( ch );

        if( decoded < 0 )
            return false;

        value = ( value << 6 ) | decoded;
        bits += 6;

        if( bits >= 0 )
        {
            aOutput.push_back( static_cast<char>( ( value >> bits ) & 0xFF ) );
            bits -= 8;
        }
    }

    return true;
}


bool zstdDecompress( const std::string& aCompressed, std::string& aOutput,
                     std::string& aError )
{
#if !KICAD_BACKPORT_WITH_ZSTD
    (void) aCompressed;

    aOutput.clear();
    aError = "zstd support is not compiled in";
    return false;
#else
    static const size_t maxOutputSize = 256u * 1024u * 1024u;

    aOutput.clear();
    aError.clear();

    if( aCompressed.empty() )
    {
        aError = "empty compressed data";
        return false;
    }

    unsigned long long contentSize = ZSTD_getFrameContentSize( aCompressed.data(),
                                                               aCompressed.size() );

    if( contentSize == ZSTD_CONTENTSIZE_ERROR )
    {
        aError = "embedded data is not a valid zstd frame";
        return false;
    }

    if( contentSize != ZSTD_CONTENTSIZE_UNKNOWN
        && contentSize > static_cast<unsigned long long>( maxOutputSize ) )
    {
        aError = "embedded zstd frame is too large";
        return false;
    }

    if( contentSize != ZSTD_CONTENTSIZE_UNKNOWN )
    {
        aOutput.assign( static_cast<size_t>( contentSize ), '\0' );
        size_t decompressed = ZSTD_decompress( aOutput.empty() ? nullptr : &aOutput[0],
                                               aOutput.size(), aCompressed.data(),
                                               aCompressed.size() );

        if( ZSTD_isError( decompressed ) )
        {
            aError = ZSTD_getErrorName( decompressed );
            return false;
        }

        aOutput.resize( decompressed );
        return true;
    }

    std::unique_ptr<ZSTD_DCtx, size_t (*)( ZSTD_DCtx* )> dctx( ZSTD_createDCtx(),
                                                               ZSTD_freeDCtx );

    if( !dctx )
    {
        aError = "could not create zstd decompression context";
        return false;
    }

    size_t bufferSize = ZSTD_DStreamOutSize();

    if( bufferSize == 0 )
        bufferSize = 128u * 1024u;

    std::vector<char> buffer( bufferSize );
    ZSTD_inBuffer input = { aCompressed.data(), aCompressed.size(), 0 };
    size_t remaining = 1;

    while( input.pos < input.size || remaining != 0 )
    {
        size_t oldInputPos = input.pos;
        ZSTD_outBuffer output = { buffer.data(), buffer.size(), 0 };
        remaining = ZSTD_decompressStream( dctx.get(), &output, &input );

        if( ZSTD_isError( remaining ) )
        {
            aError = ZSTD_getErrorName( remaining );
            return false;
        }

        if( output.pos > 0 )
        {
            if( aOutput.size() > maxOutputSize - output.pos )
            {
                aError = "embedded zstd frame is too large";
                return false;
            }

            aOutput.append( buffer.data(), output.pos );
        }

        if( remaining == 0 && input.pos == input.size )
            break;

        if( input.pos == oldInputPos && output.pos == 0 )
        {
            aError = input.pos == input.size ? "incomplete zstd frame"
                                             : "zstd stream made no progress";
            return false;
        }
    }

    return true;
#endif
}


void collectEmbeddedFiles( SEXPR::NODE* aRoot, std::map<std::string, EMBEDDED_FILE>& aFiles )
{
    if( !aRoot || aRoot->IsAtom() )
        return;

    if( aRoot->HeadView() == "embedded_files" || aRoot->HeadView() == "embedded_file" )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->HeadView() != "file" )
                continue;

            EMBEDDED_FILE file;
            SEXPR::NODE* nameNode = child->ChildList( "name" );
            SEXPR::NODE* typeNode = child->ChildList( "type" );
            file.Name = safeEmbeddedFilename( nameNode ? nameNode->AtomAt( 1 ) : "" );
            file.Type = typeNode ? typeNode->AtomAt( 1 ) : "";
            file.Data = embeddedDataAtomText( child->ChildList( "data" ) );

            if( !file.Name.empty() )
                aFiles[file.Name] = std::move( file );
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        collectEmbeddedFiles( child.get(), aFiles );
}


int rewriteEmbeddedModelUris( SEXPR::NODE* aRoot, const std::set<std::string>& aExtractedNames,
                              bool aRemoveUnresolved )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;
    kept.reserve( aRoot->Children.size() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        bool remove = false;

        if( child && !child->IsAtom() && child->HeadView() == "model" )
        {
            std::string name = embeddedUriFilename( child->AtomAt( 1 ) );

            if( !name.empty() )
            {
                if( aExtractedNames.count( name ) )
                {
                    if( child->SetAtomAt( 1, "${KIPRJMOD}/3D/" + name, true ) )
                        ++changed;
                }
                else if( aRemoveUnresolved )
                {
                    remove = true;
                    ++changed;
                }
            }
        }

        if( !remove )
        {
            changed += rewriteEmbeddedModelUris( child.get(), aExtractedNames,
                                                 aRemoveUnresolved );
            kept.push_back( std::move( child ) );
        }
    }

    aRoot->Children = std::move( kept );
    return changed;
}


int removeEmbeddedFileNodes( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;
    kept.reserve( aRoot->Children.size() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom()
            && ( child->HeadView() == "embedded_files" || child->HeadView() == "embedded_file" ) )
        {
            ++removed;
            continue;
        }

        removed += removeEmbeddedFileNodes( child.get() );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return removed;
}


EMBEDDED_MODEL_EXPORT_RESULT externalizeEmbeddedModelsForLegacyTargets( DOCUMENT& aDocument,
                                                                        const FS::path& aOutput )
{
    EMBEDDED_MODEL_EXPORT_RESULT result;

    if( !aDocument.Root || ( aDocument.Kind != KIND::BOARD && aDocument.Kind != KIND::FOOTPRINT ) )
        return result;

    std::map<std::string, EMBEDDED_FILE> files;
    collectEmbeddedFiles( aDocument.Root.get(), files );

    if( files.empty() )
        return result;

    FS::path modelDir = aOutput.parent_path() / "3D";
    std::set<std::string> extractedNames;

    for( const auto& item : files )
    {
        const EMBEDDED_FILE& file = item.second;

        if( file.Type != "model" || !isModelFileName( file.Name ) )
            continue;

        std::string compressed;

        if( !decodeBase64( file.Data, compressed ) )
        {
            result.Warnings.push_back( "could not decode embedded 3D model " + file.Name );
            continue;
        }

        std::string decompressed;
        std::string error;

        if( !zstdDecompress( compressed, decompressed, error ) )
        {
            result.Warnings.push_back( "could not extract embedded 3D model " + file.Name
                                      + ": " + error );
            continue;
        }

        WriteTextFile( modelDir / file.Name, decompressed );
        extractedNames.insert( file.Name );
        ++result.Extracted;
    }

    result.Rewritten = rewriteEmbeddedModelUris( aDocument.Root.get(), extractedNames, true );
    int removedEmbeddedFileNodes = removeEmbeddedFileNodes( aDocument.Root.get() );

    if( result.Rewritten > result.Extracted )
        result.Removed = result.Rewritten - result.Extracted;

    if( result.Extracted > 0 )
    {
        std::ostringstream msg;
        msg << "extracted " << result.Extracted
            << " embedded 3D model file(s) to 3D/ and rewrote kicad-embed model references";
        result.Warnings.push_back( msg.str() );
    }

    if( removedEmbeddedFileNodes > 0 )
    {
        std::ostringstream msg;
        msg << "removed " << removedEmbeddedFileNodes
            << " embedded file container(s) after externalizing 3D models";
        result.Warnings.push_back( msg.str() );
    }

    if( result.Removed > 0 )
    {
        std::ostringstream msg;
        msg << "removed " << result.Removed
            << " unresolved embedded 3D model reference(s) unsupported by the target format";
        result.Warnings.push_back( msg.str() );
    }

    return result;
}


bool isTokenChar( char aCh )
{
    return !std::isspace( static_cast<unsigned char>( aCh ) ) && aCh != '(' && aCh != ')';
}


std::string firstSexprHead( const std::string& aText )
{
    size_t pos = 0;

    if( aText.size() >= 3 && static_cast<unsigned char>( aText[0] ) == 0xEF
        && static_cast<unsigned char>( aText[1] ) == 0xBB
        && static_cast<unsigned char>( aText[2] ) == 0xBF )
    {
        pos = 3;
    }

    while( pos < aText.size() && std::isspace( static_cast<unsigned char>( aText[pos] ) ) )
        ++pos;

    if( pos >= aText.size() || aText[pos] != '(' )
        return "";

    ++pos;
    size_t start = pos;

    while( pos < aText.size() && isTokenChar( aText[pos] ) )
        ++pos;

    return aText.substr( start, pos - start );
}


std::string findSexprVersion( const std::string& aText )
{
    size_t pos = 0;

    while( ( pos = aText.find( "(version", pos ) ) != std::string::npos )
    {
        size_t afterHead = pos + 8;

        if( afterHead >= aText.size()
            || !std::isspace( static_cast<unsigned char>( aText[afterHead] ) ) )
        {
            pos = afterHead;
            continue;
        }

        pos = afterHead;

        while( pos < aText.size() && std::isspace( static_cast<unsigned char>( aText[pos] ) ) )
            ++pos;

        if( pos >= aText.size() )
            return "";

        bool quoted = aText[pos] == '"';

        if( quoted )
            ++pos;

        size_t start = pos;

        while( pos < aText.size() )
        {
            char ch = aText[pos];

            if( quoted ? ch == '"' : !isTokenChar( ch ) )
                break;

            ++pos;
        }

        return aText.substr( start, pos - start );
    }

    return "";
}


std::string firstMatchingLineSuffix( const std::string& aText, const std::string& aPrefix )
{
    std::istringstream in( aText );
    std::string line;

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        if( StartsWith( line, aPrefix ) )
            return Trim( line.substr( aPrefix.size() ) );
    }

    return "";
}


std::string fastVersionForKind( KIND aKind, const std::string& aText )
{
    switch( aKind )
    {
    case KIND::PROJECT:
        return "kicad-project-json";

    case KIND::LEGACY_SCHEMATIC:
    {
        std::string value = firstMatchingLineSuffix( aText,
                                                     "EESchema Schematic File Version" );
        return value.empty() ? "legacy-sch" : "legacy-sch-v" + value;
    }

    case KIND::LEGACY_SYMBOL_LIBRARY:
    {
        std::string value = firstMatchingLineSuffix( aText, "EESchema-LIBRARY Version" );
        return value.empty() ? "legacy-lib" : "legacy-lib-" + value;
    }

    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
        return "legacy-dcm-2.0";

    case KIND::LEGACY_PROJECT:
        return "legacy-pro";

    default:
        break;
    }

    std::string version = findSexprVersion( aText );
    return version.empty() ? "unknown" : version;
}


FILE_REPORT detectVersionFast( const FS::path& aPath )
{
    std::string text = ReadTextFile( aPath );
    KIND extensionKind = DetectKind( aPath, std::string() );
    std::string head = IsLegacyKind( extensionKind ) || extensionKind == KIND::PROJECT
            ? std::string() : firstSexprHead( text );
    KIND kind = DetectKind( aPath, head );

    FILE_REPORT report;
    report.Path = aPath.string();
    report.InputPath = aPath.string();
    report.OutputPath = aPath.string();
    report.Kind = KindName( kind );
    report.SourceKind = report.Kind;
    report.TargetKind = report.Kind;
    report.SourceVersion = fastVersionForKind( kind, text );
    return report;
}


bool hasDetectedVersion( const FILE_REPORT& aReport )
{
    return aReport.Kind != "unknown" && !aReport.SourceVersion.empty()
           && aReport.SourceVersion != "unknown";
}


KIND kindFromReportName( const std::string& aKind )
{
    if( aKind == "symbol-library" ) return KIND::SYMBOL_LIBRARY;
    if( aKind == "schematic" ) return KIND::SCHEMATIC;
    if( aKind == "board" ) return KIND::BOARD;
    if( aKind == "footprint" ) return KIND::FOOTPRINT;
    if( aKind == "design-rules" ) return KIND::DESIGN_RULES;
    if( aKind == "worksheet" ) return KIND::WORKSHEET;
    if( aKind == "project" ) return KIND::PROJECT;
    if( aKind == "legacy-schematic" ) return KIND::LEGACY_SCHEMATIC;
    if( aKind == "legacy-symbol-library" ) return KIND::LEGACY_SYMBOL_LIBRARY;
    if( aKind == "legacy-symbol-documentation" ) return KIND::LEGACY_SYMBOL_DOCUMENTATION;
    if( aKind == "legacy-project" ) return KIND::LEGACY_PROJECT;
    return KIND::UNKNOWN;
}


std::string displayReportVersion( const std::string& aKind, const std::string& aVersion,
                                  const std::string& aFallback )
{
    if( aVersion.empty() )
        return aFallback;

    KIND kind = kindFromReportName( aKind );

    if( kind == KIND::UNKNOWN )
        return aVersion;

    return DisplayVersionAlias( kind, aVersion );
}


std::string resolveDocumentTargetVersion( const DOCUMENT& aDocument, const std::string& aTarget )
{
    std::string resolved = ResolveTargetVersion( aDocument.Kind, aTarget );

    if( IsNumber( aTarget ) && IsNumber( aDocument.Version ) && IsNumber( resolved ) )
    {
        int source = std::stoi( aDocument.Version );
        int target = std::stoi( resolved );

        if( source > 0 && source < target )
            return aDocument.Version;
    }

    return resolved;
}


FS::path withTargetFamilyExtension( const FS::path& aPath,
                                                 const std::string& aTarget )
{
    int targetMajor = TargetMajorVersion( aTarget );
    KIND sourceKind = DetectKind( aPath, std::string() );

    if( targetMajor <= 5 )
    {
        KIND legacyKind = LegacyKindForSexprKind( sourceKind );

        if( legacyKind != KIND::UNKNOWN )
            return WithTargetExtension( aPath, legacyKind );

        return aPath;
    }

    KIND sexprKind = SexprKindForLegacyKind( sourceKind );

    if( sexprKind != KIND::UNKNOWN )
        return WithTargetExtension( aPath, sexprKind );

    return aPath;
}


int removeDirectChildByHead( SEXPR::NODE* aRoot, const std::string& aHead )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;
    kept.reserve( aRoot->Children.size() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == aHead )
        {
            ++removed;
            continue;
        }

        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return removed;
}


std::string replaceTrailingText( const std::string& aValue, const std::string& aFrom,
                                 const std::string& aTo )
{
    if( !EndsWith( Lower( aValue ), Lower( aFrom ) ) )
        return aValue;

    return aValue.substr( 0, aValue.size() - aFrom.size() ) + aTo;
}


int ensureLegacySchematicLibraryLine( const FS::path& aSchematic, const std::string& aLibrary )
{
    std::string text = ReadTextFile( aSchematic );
    std::string line = "LIBS:" + aLibrary;

    if( text.find( line + "\n" ) != std::string::npos
        || text.find( line + "\r\n" ) != std::string::npos )
    {
        return 0;
    }

    size_t insertAt = std::string::npos;
    size_t pos = 0;

    while( pos < text.size() )
    {
        size_t lineEnd = text.find( '\n', pos );
        size_t next = lineEnd == std::string::npos ? text.size() : lineEnd + 1;
        std::string current = text.substr( pos, lineEnd == std::string::npos
                                                ? std::string::npos : lineEnd - pos );

        if( !current.empty() && current.back() == '\r' )
            current.pop_back();

        if( StartsWith( current, "LIBS:" ) )
            insertAt = next;
        else if( insertAt != std::string::npos )
            break;

        if( lineEnd == std::string::npos )
            break;

        pos = next;
    }

    if( insertAt == std::string::npos )
    {
        size_t headerEnd = text.find( '\n' );
        insertAt = headerEnd == std::string::npos ? text.size() : headerEnd + 1;
    }

    text.insert( insertAt, line + "\n" );
    WriteTextFile( aSchematic, text );
    return 1;
}


FILE_REPORT ensureLegacySchematicCacheLibrary( const FS::path& aProjectDir,
                                               const std::vector<PROJECT_COPY_ENTRY>& aCopied )
{
    FILE_REPORT report;
    report.Path = aProjectDir.string();
    report.OutputPath = aProjectDir.string();
    report.Kind = "legacy-symbol-library";
    report.SourceKind = "legacy-symbol-library";
    report.TargetKind = "legacy-symbol-library";
    report.SourceVersion = "legacy-lib";
    report.TargetVersion = "legacy-lib";

    FS::path projectPath;

    for( const PROJECT_COPY_ENTRY& entry : aCopied )
    {
        if( Lower( entry.Output.extension().string() ) == ".pro"
            && entry.Output.parent_path() == aProjectDir )
        {
            projectPath = entry.Output;
            break;
        }
    }

    if( projectPath.empty() )
        return report;

    FS::path libraryPath = aProjectDir / "Library.lib";

    if( !FS::exists( libraryPath ) )
        return report;

    std::string cacheName = projectPath.stem().string() + "-cache";
    FS::path cachePath = aProjectDir / ( cacheName + ".lib" );
    FS::copy_file( libraryPath, cachePath, FS::copy_options::overwrite_existing );
    report.Changed = true;
    report.Path = cachePath.string();
    report.OutputPath = cachePath.string();
    report.Warnings.push_back( "created legacy schematic cache library " + cachePath.filename().string() );

    int schematicChanges = 0;

    std::error_code error;

    for( FS::directory_iterator it( aProjectDir, error ), end; it != end; ++it )
    {
        if( error )
            break;

        const FS::directory_entry& entry = *it;

        if( entry.is_regular_file() && Lower( entry.path().extension().string() ) == ".sch" )
            schematicChanges += ensureLegacySchematicLibraryLine( entry.path(), cacheName );
    }

    if( schematicChanges > 0 )
        report.Warnings.push_back( "added legacy schematic cache library references" );

    return report;
}


int normalizeLegacySymbolLibraryTableEntries( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "sym_lib_table" )
        return 0;

    int changed = 0;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "lib" )
            continue;

        if( SEXPR::NODE* type = child->ChildList( "type" ) )
        {
            if( type->SetAtomAt( 1, "Legacy", true ) )
                ++changed;
        }

        if( SEXPR::NODE* uri = child->ChildList( "uri" ) )
        {
            std::string mapped = replaceTrailingText( uri->AtomAt( 1 ), ".kicad_sym", ".lib" );

            if( mapped != uri->AtomAt( 1 ) && uri->SetAtomAt( 1, mapped, true ) )
                ++changed;
        }
    }

    return changed;
}


FILE_REPORT normalizeLegacyLibraryTable( const FS::path& aPath, int aTargetMajor )
{
    FILE_REPORT report;
    report.Path = aPath.string();
    report.InputPath = aPath.string();
    report.OutputPath = aPath.string();
    report.Kind = "library-table";
    report.SourceKind = "library-table";
    report.TargetKind = "library-table";
    report.SourceVersion = "project-local";
    report.TargetVersion = aTargetMajor <= 5 ? "legacy-compatible" : "modern-compatible";

    try
    {
        std::string text = ReadTextFile( aPath );
        std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( text );
        int changed = removeDirectChildByHead( root.get(), "version" );

        if( aTargetMajor <= 5 )
            changed += normalizeLegacySymbolLibraryTableEntries( root.get() );

        if( changed > 0 )
        {
            WriteTextFile( aPath, SEXPR::Format( root.get(), text.size() ) );
            report.Changed = true;
        }
    }
    catch( const std::exception& e )
    {
        report.Warnings.push_back( std::string( "could not normalize project library table: " )
                                   + e.what() );
    }

    return report;
}


std::unique_ptr<SEXPR::NODE> listNode( const std::string& aHead )
{
    std::unique_ptr<SEXPR::NODE> node = SEXPR::NODE::MakeList();
    node->Children.push_back( SEXPR::NODE::MakeAtom( aHead ) );
    return node;
}


std::unique_ptr<SEXPR::NODE> cloneNode( const SEXPR::NODE* aNode )
{
    if( !aNode )
        return nullptr;

    if( aNode->IsAtom() )
        return SEXPR::NODE::MakeAtom( std::string( aNode->Atom ), aNode->Quoted );

    std::unique_ptr<SEXPR::NODE> clone = SEXPR::NODE::MakeList();
    clone->Children.reserve( aNode->Children.size() );

    for( const std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
        clone->Children.push_back( cloneNode( child.get() ) );

    return clone;
}


std::string childAtomOrEmpty( SEXPR::NODE* aNode, const std::string& aHead,
                              size_t aIndex = 1 )
{
    SEXPR::NODE* child = aNode ? aNode->ChildList( aHead ) : nullptr;
    return child ? child->AtomAt( aIndex ) : "";
}


std::string footprintLibraryNickname( const std::string& aLibId )
{
    std::string::size_type sep = aLibId.find( ':' );

    if( sep == std::string::npos || sep == 0 )
        return "";

    return aLibId.substr( 0, sep );
}


std::string footprintLibraryFootprintName( const std::string& aLibId )
{
    std::string::size_type sep = aLibId.find( ':' );

    if( sep == std::string::npos || sep + 1 >= aLibId.size() )
        return "";

    return aLibId.substr( sep + 1 );
}


bool projectLocalFootprintExists( const FS::path& aProjectDir,
                                  const std::string& aFootprintName )
{
    if( aFootprintName.empty() )
        return false;

    return FS::exists( aProjectDir / "Library.pretty"
                                    / ( aFootprintName + ".kicad_mod" ) );
}


void collectProjectLocalFootprintNicknames( SEXPR::NODE* aNode,
                                            const FS::path& aProjectDir,
                                            std::set<std::string>& aNicknames )
{
    if( !aNode || aNode->IsAtom() )
        return;

    std::string head = aNode->Head();

    if( head == "footprint" || head == "module" )
    {
        std::string libId = aNode->AtomAt( 1 );
        std::string nickname = footprintLibraryNickname( libId );
        std::string footprint = footprintLibraryFootprintName( libId );

        if( !nickname.empty() && projectLocalFootprintExists( aProjectDir, footprint ) )
            aNicknames.insert( nickname );
    }

    for( const std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
        collectProjectLocalFootprintNicknames( child.get(), aProjectDir, aNicknames );
}


std::set<std::string> collectProjectLocalFootprintNicknames(
        const FS::path& aProjectDir, const std::vector<PROJECT_COPY_ENTRY>& aCopied )
{
    std::set<std::string> nicknames;

    if( !FS::is_directory( aProjectDir / "Library.pretty" ) )
        return nicknames;

    for( const PROJECT_COPY_ENTRY& entry : aCopied )
    {
        if( !entry.IsDocument || Lower( entry.Output.extension().string() ) != ".kicad_pcb"
            || entry.Output.parent_path() != aProjectDir )
        {
            continue;
        }

        try
        {
            std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( ReadTextFile( entry.Output ) );
            collectProjectLocalFootprintNicknames( root.get(), aProjectDir, nicknames );
        }
        catch( ... )
        {
        }
    }

    return nicknames;
}


std::unique_ptr<SEXPR::NODE> libraryTableField( const std::string& aHead,
                                                const std::string& aValue )
{
    std::unique_ptr<SEXPR::NODE> field = listNode( aHead );
    field->Children.push_back( SEXPR::NODE::MakeAtom( aValue, true ) );
    return field;
}


std::unique_ptr<SEXPR::NODE> projectLocalFootprintLibraryEntry( const std::string& aNickname )
{
    std::unique_ptr<SEXPR::NODE> lib = listNode( "lib" );
    lib->Children.push_back( libraryTableField( "name", aNickname ) );
    lib->Children.push_back( libraryTableField( "type", "KiCad" ) );
    lib->Children.push_back( libraryTableField( "uri", "${KIPRJMOD}/Library.pretty" ) );
    lib->Children.push_back( libraryTableField( "options", "" ) );
    lib->Children.push_back( libraryTableField( "descr", "" ) );
    return lib;
}


std::unique_ptr<SEXPR::NODE> projectLocalSymbolLibraryEntry( const std::string& aNickname,
                                                             const std::string& aUri )
{
    std::unique_ptr<SEXPR::NODE> lib = listNode( "lib" );
    lib->Children.push_back( libraryTableField( "name", aNickname ) );
    lib->Children.push_back( libraryTableField( "type", "KiCad" ) );
    lib->Children.push_back( libraryTableField( "uri", aUri ) );
    lib->Children.push_back( libraryTableField( "options", "" ) );
    lib->Children.push_back( libraryTableField( "descr", "" ) );
    return lib;
}


std::string projectLibraryUri( const FS::path& aProjectDir,
                               const FS::path& aLibraryPath )
{
    std::error_code error;
    FS::path rel = FS::relative( aLibraryPath, aProjectDir, error );

    if( error )
        rel = aLibraryPath.filename();

    return "${KIPRJMOD}/" + rel.generic_string();
}


std::map<std::string, std::string> collectProjectLocalSymbolLibraries(
        const FS::path& aProjectDir, const std::vector<PROJECT_COPY_ENTRY>& aCopied,
        const std::set<std::string>& aAllowedNicknames = {}, bool aFilterNicknames = false )
{
    std::map<std::string, std::string> libraries;

    for( const PROJECT_COPY_ENTRY& entry : aCopied )
    {
        if( !entry.IsDocument || Lower( entry.Output.extension().string() ) != ".kicad_sym"
            || entry.Output.parent_path() != aProjectDir )
        {
            continue;
        }

        std::string nickname = entry.Output.stem().string();

        if( aFilterNicknames && aAllowedNicknames.count( nickname ) == 0 )
            continue;

        libraries[nickname] = projectLibraryUri( aProjectDir, entry.Output );
    }

    return libraries;
}


std::map<std::string, FS::path> collectProjectLocalSymbolLibraryPaths(
        const FS::path& aProjectDir, const std::vector<PROJECT_COPY_ENTRY>& aCopied )
{
    std::map<std::string, FS::path> libraries;

    for( const PROJECT_COPY_ENTRY& entry : aCopied )
    {
        if( !entry.IsDocument || Lower( entry.Output.extension().string() ) != ".kicad_sym"
            || entry.Output.parent_path() != aProjectDir )
        {
            continue;
        }

        libraries[entry.Output.stem().string()] = entry.Output;
    }

    return libraries;
}


std::string symbolLibraryNicknameFromLibId( const std::string& aLibId )
{
    std::string::size_type sep = aLibId.find( ':' );

    if( sep == std::string::npos || sep == 0 )
        return "";

    return aLibId.substr( 0, sep );
}


std::string symbolLibrarySymbolNameFromLibId( const std::string& aLibId )
{
    std::string::size_type sep = aLibId.find( ':' );

    if( sep == std::string::npos || sep + 1 >= aLibId.size() )
        return "";

    return aLibId.substr( sep + 1 );
}


void collectSchematicLibIds( SEXPR::NODE* aNode, std::set<std::string>& aLibIds )
{
    if( !aNode || aNode->IsAtom() )
        return;

    if( aNode->HeadView() == "symbol" )
    {
        std::string libId = childAtomOrEmpty( aNode, "lib_id" );

        if( !libId.empty() )
            aLibIds.insert( libId );
    }

    for( const std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
        collectSchematicLibIds( child.get(), aLibIds );
}


std::set<std::string> collectReferencedSchematicLibraryNicknames(
        const FS::path& aProjectDir, const std::vector<PROJECT_COPY_ENTRY>& aCopied )
{
    std::set<std::string> nicknames;

    for( const PROJECT_COPY_ENTRY& entry : aCopied )
    {
        if( !entry.IsDocument || Lower( entry.Output.extension().string() ) != ".kicad_sch"
            || entry.Output.parent_path() != aProjectDir )
        {
            continue;
        }

        try
        {
            std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( ReadTextFile( entry.Output ) );
            std::set<std::string> libIds;
            collectSchematicLibIds( root.get(), libIds );

            for( const std::string& libId : libIds )
            {
                std::string nickname = symbolLibraryNicknameFromLibId( libId );

                if( !nickname.empty() )
                    nicknames.insert( nickname );
            }
        }
        catch( ... )
        {
        }
    }

    return nicknames;
}


SEXPR::NODE* ensureSchematicLibSymbolsNode( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "kicad_sch" )
        return nullptr;

    SEXPR::NODE* existing = aRoot->ChildList( "lib_symbols" );

    if( existing )
    {
        std::vector<std::unique_ptr<SEXPR::NODE>> children;
        children.push_back( SEXPR::NODE::MakeAtom( "lib_symbols" ) );
        existing->Children = std::move( children );
        return existing;
    }

    std::unique_ptr<SEXPR::NODE> libSymbols = listNode( "lib_symbols" );
    SEXPR::NODE* raw = libSymbols.get();
    size_t insertAt = std::min<size_t>( 4, aRoot->Children.size() );
    aRoot->Children.insert( aRoot->Children.begin() + insertAt, std::move( libSymbols ) );
    return raw;
}


std::string makeSymbolLibId( const std::string& aNickname, const std::string& aSymbolName )
{
    if( aNickname.empty() || aSymbolName.empty() )
        return "";

    return aNickname + ":" + aSymbolName;
}


bool splitSymbolLibId( const std::string& aLibId, std::string& aNickname,
                       std::string& aSymbolName )
{
    std::string::size_type sep = aLibId.find( ':' );

    if( sep == std::string::npos || sep == 0 || sep + 1 >= aLibId.size() )
        return false;

    aNickname = aLibId.substr( 0, sep );
    aSymbolName = aLibId.substr( sep + 1 );
    return true;
}


const SEXPR::NODE* resolvedProjectLocalSymbolSource(
        const std::map<std::string, std::map<std::string, std::unique_ptr<SEXPR::NODE>>>&
                aSymbolsByLibrary,
        const std::string& aNickname, const std::string& aSymbolName,
        std::set<std::string>& aVisitingLibIds )
{
    std::string libId = makeSymbolLibId( aNickname, aSymbolName );

    if( libId.empty() || aVisitingLibIds.count( libId ) != 0 )
        return nullptr;

    auto libraryIt = aSymbolsByLibrary.find( aNickname );

    if( libraryIt == aSymbolsByLibrary.end() )
        return nullptr;

    auto symbolIt = libraryIt->second.find( aSymbolName );

    if( symbolIt == libraryIt->second.end() )
        return nullptr;

    const SEXPR::NODE* symbol = symbolIt->second.get();
    SEXPR::NODE* extends = symbolIt->second ? symbolIt->second->ChildList( "extends" )
                                            : nullptr;

    if( !extends || extends->AtomAtView( 1 ).empty() )
        return symbol;

    std::string parentNickname = aNickname;
    std::string parentName = extends->AtomAt( 1 );

    splitSymbolLibId( parentName, parentNickname, parentName );

    aVisitingLibIds.insert( libId );
    const SEXPR::NODE* parent = resolvedProjectLocalSymbolSource( aSymbolsByLibrary,
                                                                  parentNickname,
                                                                  parentName,
                                                                  aVisitingLibIds );
    aVisitingLibIds.erase( libId );

    return parent ? parent : symbol;
}


void setSymbolValueProperty( SEXPR::NODE* aSymbol, const std::string& aValue )
{
    if( !aSymbol )
        return;

    for( const std::unique_ptr<SEXPR::NODE>& child : aSymbol->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "property"
            && child->AtomAt( 1 ) == "Value" )
        {
            child->SetAtomAt( 2, aValue, true );
            return;
        }
    }
}


void renameNestedSymbolDefinitions( SEXPR::NODE* aNode, const std::string& aOldName,
                                    const std::string& aNewName )
{
    if( !aNode || aNode->IsAtom() || aOldName.empty() || aNewName.empty() )
        return;

    for( std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
    {
        if( !child || child->IsAtom() )
            continue;

        if( child->HeadView() == "symbol" )
        {
            std::string name = child->AtomAt( 1 );

            if( name.size() > aOldName.size() && name.substr( 0, aOldName.size() ) == aOldName
                && name[aOldName.size()] == '_' )
            {
                child->SetAtomAt( 1, aNewName + name.substr( aOldName.size() ), true );
            }

            if( SEXPR::NODE* extends = child->ChildList( "extends" ) )
            {
                std::string parent = extends->AtomAt( 1 );

                if( parent.size() > aOldName.size()
                    && parent.substr( 0, aOldName.size() ) == aOldName
                    && parent[aOldName.size()] == '_' )
                {
                    extends->SetAtomAt( 1, aNewName + parent.substr( aOldName.size() ), true );
                }
            }
        }

        renameNestedSymbolDefinitions( child.get(), aOldName, aNewName );
    }
}


bool embedProjectLocalSchematicSymbol(
        const std::map<std::string, std::map<std::string, std::unique_ptr<SEXPR::NODE>>>&
                aSymbolsByLibrary,
        const std::string& aNickname, const std::string& aSymbolName,
        SEXPR::NODE* aLibSymbols, std::set<std::string>& aEmbeddedLibIds,
        int& aAdded )
{
    std::string libId = makeSymbolLibId( aNickname, aSymbolName );

    if( libId.empty() || aEmbeddedLibIds.count( libId ) != 0 )
        return true;

    std::set<std::string> visitingLibIds;
    const SEXPR::NODE* source = resolvedProjectLocalSymbolSource( aSymbolsByLibrary, aNickname,
                                                                  aSymbolName,
                                                                  visitingLibIds );

    if( !source )
        return false;

    std::unique_ptr<SEXPR::NODE> embedded = cloneNode( source );
    removeDirectChildByHead( embedded.get(), "extends" );
    std::string oldName = embedded ? embedded->AtomAt( 1 ) : "";

    if( embedded && embedded->SetAtomAt( 1, libId, true ) )
    {
        renameNestedSymbolDefinitions( embedded.get(), oldName, aSymbolName );
        setSymbolValueProperty( embedded.get(), aSymbolName );
        aLibSymbols->Children.push_back( std::move( embedded ) );
        aEmbeddedLibIds.insert( libId );
        ++aAdded;
    }

    return true;
}


int embedProjectLocalSchematicSymbols(
        const FS::path& aProjectDir, const std::vector<PROJECT_COPY_ENTRY>& aCopied,
        std::vector<std::string>& aWarnings )
{
    std::map<std::string, FS::path> libraries =
            collectProjectLocalSymbolLibraryPaths( aProjectDir, aCopied );

    if( libraries.empty() )
        return 0;

    std::map<std::string, std::map<std::string, std::unique_ptr<SEXPR::NODE>>> symbolsByLibrary;

    for( const auto& library : libraries )
    {
        try
        {
            std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( ReadTextFile( library.second ) );

            if( !root || root->HeadView() != "kicad_symbol_lib" )
                continue;

            for( SEXPR::NODE* symbol : root->ChildLists( "symbol" ) )
            {
                std::string name = symbol ? symbol->AtomAt( 1 ) : "";

                if( !name.empty() )
                    symbolsByLibrary[library.first][name] = cloneNode( symbol );
            }
        }
        catch( const std::exception& e )
        {
            aWarnings.push_back( "could not read generated symbol library "
                                 + library.second.string() + ": " + e.what() );
        }
    }

    int changed = 0;

    for( const PROJECT_COPY_ENTRY& entry : aCopied )
    {
        if( !entry.IsDocument || Lower( entry.Output.extension().string() ) != ".kicad_sch"
            || entry.Output.parent_path() != aProjectDir )
        {
            continue;
        }

        try
        {
            std::string text = ReadTextFile( entry.Output );
            std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( text );
            std::set<std::string> libIds;
            collectSchematicLibIds( root.get(), libIds );

            SEXPR::NODE* libSymbols = ensureSchematicLibSymbolsNode( root.get() );

            if( !libSymbols )
                continue;

            int added = 0;
            std::set<std::string> embeddedLibIds;

            for( const std::string& libId : libIds )
            {
                std::string nickname = symbolLibraryNicknameFromLibId( libId );
                std::string symbolName = symbolLibrarySymbolNameFromLibId( libId );
                embedProjectLocalSchematicSymbol( symbolsByLibrary, nickname, symbolName,
                                                  libSymbols, embeddedLibIds, added );
            }

            if( added > 0 )
            {
                WriteTextFile( entry.Output, SEXPR::Format( root.get(), text.size() ) );
                ++changed;
            }
        }
        catch( const std::exception& e )
        {
            aWarnings.push_back( "could not embed generated schematic symbols in "
                                 + entry.Output.string() + ": " + e.what() );
        }
    }

    return changed;
}


int addProjectLocalSymbolLibraries( SEXPR::NODE* aRoot,
                                    const std::map<std::string, std::string>& aLibraries )
{
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "sym_lib_table" )
        return 0;

    std::map<std::string, SEXPR::NODE*> existing;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "lib" )
            continue;

        std::string name = childAtomOrEmpty( child.get(), "name" );

        if( !name.empty() )
            existing[name] = child.get();
    }

    int changed = 0;

    for( const auto& library : aLibraries )
    {
        auto found = existing.find( library.first );

        if( found != existing.end() )
        {
            SEXPR::NODE* lib = found->second;

            if( SEXPR::NODE* type = lib->ChildList( "type" ) )
            {
                if( type->SetAtomAt( 1, "KiCad", true ) )
                    ++changed;
            }
            else
            {
                lib->Children.push_back( libraryTableField( "type", "KiCad" ) );
                ++changed;
            }

            if( SEXPR::NODE* uri = lib->ChildList( "uri" ) )
            {
                if( uri->SetAtomAt( 1, library.second, true ) )
                    ++changed;
            }
            else
            {
                lib->Children.push_back( libraryTableField( "uri", library.second ) );
                ++changed;
            }

            continue;
        }

        aRoot->Children.push_back( projectLocalSymbolLibraryEntry( library.first,
                                                                    library.second ) );
        existing[library.first] = aRoot->Children.back().get();
        ++changed;
    }

    return changed;
}


FILE_REPORT ensureProjectLocalSymbolLibraryTable(
        const FS::path& aProjectDir, const std::vector<PROJECT_COPY_ENTRY>& aCopied,
        int aTargetMajor )
{
    FILE_REPORT report;
    FS::path tablePath = aProjectDir / "sym-lib-table";
    report.Path = tablePath.string();
    report.InputPath = tablePath.string();
    report.OutputPath = tablePath.string();
    report.Kind = "library-table";
    report.SourceKind = "library-table";
    report.TargetKind = "library-table";
    report.SourceVersion = "project-local";
    report.TargetVersion = "modern-compatible";

    std::set<std::string> referencedNicknames;

    if( aTargetMajor > 5 )
        referencedNicknames = collectReferencedSchematicLibraryNicknames( aProjectDir, aCopied );

    std::map<std::string, std::string> libraries =
            collectProjectLocalSymbolLibraries( aProjectDir, aCopied, referencedNicknames,
                                                aTargetMajor > 5 );

    if( libraries.empty() )
        return report;

    try
    {
        std::unique_ptr<SEXPR::NODE> root;
        std::string existingText;

        if( aTargetMajor > 5 )
        {
            root = listNode( "sym_lib_table" );

            if( aTargetMajor >= 7 )
            {
                std::unique_ptr<SEXPR::NODE> version = listNode( "version" );
                version->Children.push_back( SEXPR::NODE::MakeAtom( "7" ) );
                root->Children.push_back( std::move( version ) );
            }
        }
        else if( FS::exists( tablePath ) )
        {
            existingText = ReadTextFile( tablePath );
            root = SEXPR::Parse( existingText );
        }
        else
        {
            root = listNode( "sym_lib_table" );

            if( aTargetMajor >= 7 )
            {
                std::unique_ptr<SEXPR::NODE> version = listNode( "version" );
                version->Children.push_back( SEXPR::NODE::MakeAtom( "7" ) );
                root->Children.push_back( std::move( version ) );
            }
        }

        if( aTargetMajor <= 6 )
            removeDirectChildByHead( root.get(), "version" );

        int changed = addProjectLocalSymbolLibraries( root.get(), libraries );

        if( changed > 0 || existingText.empty() || aTargetMajor > 5 )
        {
            WriteTextFile( tablePath, SEXPR::Format( root.get(), existingText.size() + 512 ) );
            report.Changed = true;
            report.Warnings.push_back( "wrote project-local symbol library table for generated .kicad_sym files" );
        }
    }
    catch( const std::exception& e )
    {
        report.Warnings.push_back( std::string( "could not write project-local symbol library table: " )
                                   + e.what() );
    }

    return report;
}


int addProjectLocalFootprintLibraryAliases( SEXPR::NODE* aRoot,
                                            const std::set<std::string>& aNicknames )
{
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "fp_lib_table" )
        return 0;

    std::set<std::string> existing;

    for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "lib" )
            continue;

        std::string name = childAtomOrEmpty( child.get(), "name" );

        if( !name.empty() )
            existing.insert( name );
    }

    int changed = 0;

    for( const std::string& nickname : aNicknames )
    {
        if( existing.count( nickname ) != 0 )
            continue;

        aRoot->Children.push_back( projectLocalFootprintLibraryEntry( nickname ) );
        existing.insert( nickname );
        ++changed;
    }

    return changed;
}


int ensureLegacyFootprintLibraryAliases( const FS::path& aTablePath,
                                         const std::vector<PROJECT_COPY_ENTRY>& aCopied,
                                         int aTargetMajor,
                                         std::vector<std::string>& aWarnings )
{
    if( aTargetMajor > 5 || Lower( aTablePath.filename().string() ) != "fp-lib-table" )
        return 0;

    FS::path projectDir = aTablePath.parent_path();
    std::set<std::string> nicknames = collectProjectLocalFootprintNicknames( projectDir, aCopied );

    if( nicknames.empty() )
        return 0;

    try
    {
        std::string text = ReadTextFile( aTablePath );
        std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( text );
        int changed = addProjectLocalFootprintLibraryAliases( root.get(), nicknames );

        if( changed > 0 )
            WriteTextFile( aTablePath, SEXPR::Format( root.get(), text.size() ) );

        return changed;
    }
    catch( const std::exception& e )
    {
        aWarnings.push_back( std::string( "could not add project-local footprint aliases: " )
                             + e.what() );
    }

    return 0;
}


std::string schematicPropertyValue( SEXPR::NODE* aNode, const std::string& aName )
{
    if( !aNode )
        return "";

    for( const std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "property"
            && child->AtomAt( 1 ) == aName )
        {
            return child->AtomAt( 2 );
        }
    }

    return "";
}


std::string normalizedHiddenInstanceReference( const std::string& aReference,
                                               const std::string& aValue )
{
    if( aReference.size() < 3 || aReference[0] != '#' || aReference[1] != 'U' )
        return aReference;

    size_t suffixStart = 2;

    while( suffixStart < aReference.size()
           && !std::isdigit( static_cast<unsigned char>( aReference[suffixStart] ) ) )
    {
        ++suffixStart;
    }

    std::string suffix = suffixStart < aReference.size()
            ? aReference.substr( suffixStart ) : std::string();

    if( aValue == "PWR_FLAG" )
        return "#FLG" + suffix;

    if( !aValue.empty() && ( aValue[0] == '+' || aValue[0] == '-' || aValue == "GND"
                             || aValue == "VCC" || aValue == "VDD"
                             || aValue == "VSS" ) )
    {
        return "#PWR" + suffix;
    }

    return aReference;
}


std::string sheetFilePropertyValue( SEXPR::NODE* aSheet )
{
    std::string file = schematicPropertyValue( aSheet, "Sheet file" );

    if( file.empty() )
        file = schematicPropertyValue( aSheet, "Sheetfile" );

    return file;
}


std::string appendInstanceUuid( const std::string& aPrefix, const std::string& aUuid )
{
    if( aUuid.empty() )
        return aPrefix.empty() ? "/" : aPrefix;

    if( aPrefix.empty() || aPrefix == "/" )
        return "/" + aUuid;

    return aPrefix + "/" + aUuid;
}


std::unique_ptr<SEXPR::NODE> sheetInstanceNode( const std::string& aPath,
                                                const std::string& aPage )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( "path" );
    node->Children.push_back( SEXPR::NODE::MakeAtom( aPath, true ) );

    std::unique_ptr<SEXPR::NODE> page = listNode( "page" );
    page->Children.push_back( SEXPR::NODE::MakeAtom( aPage.empty() ? "1" : aPage, true ) );
    node->Children.push_back( std::move( page ) );
    return node;
}


std::unique_ptr<SEXPR::NODE> symbolInstanceNode( const std::string& aPath,
                                                 SEXPR::NODE* aSymbol )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( "path" );
    node->Children.push_back( SEXPR::NODE::MakeAtom( aPath, true ) );

    auto appendQuoted = [&]( const std::string& aHead, const std::string& aValue )
    {
        std::unique_ptr<SEXPR::NODE> child = listNode( aHead );
        child->Children.push_back( SEXPR::NODE::MakeAtom( aValue, true ) );
        node->Children.push_back( std::move( child ) );
    };

    std::string unit = childAtomOrEmpty( aSymbol, "unit" );

    if( unit.empty() )
        unit = "1";

    std::string value = schematicPropertyValue( aSymbol, "Value" );
    appendQuoted( "reference", normalizedHiddenInstanceReference(
                          schematicPropertyValue( aSymbol, "Reference" ), value ) );

    std::unique_ptr<SEXPR::NODE> unitNode = listNode( "unit" );
    unitNode->Children.push_back( SEXPR::NODE::MakeAtom( unit ) );
    node->Children.push_back( std::move( unitNode ) );

    appendQuoted( "value", value );
    appendQuoted( "footprint", schematicPropertyValue( aSymbol, "Footprint" ) );
    return node;
}


std::string uniquifiedReference( const std::string& aReference, int aDuplicateIndex )
{
    if( aDuplicateIndex <= 0 || aReference.empty() )
        return aReference;

    size_t digitStart = 0;

    while( digitStart < aReference.size()
           && !std::isdigit( static_cast<unsigned char>( aReference[digitStart] ) ) )
    {
        ++digitStart;
    }

    if( digitStart >= aReference.size() )
        return aReference + std::to_string( aDuplicateIndex + 1 );

    size_t digitEnd = digitStart;

    while( digitEnd < aReference.size()
           && std::isdigit( static_cast<unsigned char>( aReference[digitEnd] ) ) )
    {
        ++digitEnd;
    }

    int number = 0;

    try
    {
        number = std::stoi( aReference.substr( digitStart, digitEnd - digitStart ) );
    }
    catch( const std::exception& )
    {
        return aReference + std::to_string( aDuplicateIndex + 1 );
    }

    return aReference.substr( 0, digitStart )
           + std::to_string( number + aDuplicateIndex * 1000 )
           + aReference.substr( digitEnd );
}


void uniquifyRepeatedSymbolInstanceReferences( SEXPR::NODE* aSymbolInstances )
{
    if( !aSymbolInstances || aSymbolInstances->IsAtom()
        || aSymbolInstances->HeadView() != "symbol_instances" )
    {
        return;
    }

    std::map<std::string, int> seen;
    std::set<std::string> used;

    for( const std::unique_ptr<SEXPR::NODE>& child : aSymbolInstances->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "path" )
            continue;

        SEXPR::NODE* reference = child->ChildList( "reference" );

        if( !reference )
            continue;

        std::string original = reference->AtomAt( 1 );
        int duplicateIndex = seen[original]++;
        std::string candidate = uniquifiedReference( original, duplicateIndex );

        while( used.count( candidate ) != 0 )
            candidate = uniquifiedReference( original, ++duplicateIndex );

        if( candidate != original )
            reference->SetAtomAt( 1, candidate, true );

        used.insert( candidate );
    }
}


std::map<std::string, std::string> existingSheetInstancePages( SEXPR::NODE* aRoot )
{
    std::map<std::string, std::string> pages;
    SEXPR::NODE* sheetInstances = aRoot ? aRoot->ChildList( "sheet_instances" ) : nullptr;

    if( !sheetInstances )
        return pages;

    for( const std::unique_ptr<SEXPR::NODE>& child : sheetInstances->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "path" )
            continue;

        std::string path = child->AtomAt( 1 );
        std::string page = childAtomOrEmpty( child.get(), "page" );

        if( !path.empty() && !page.empty() )
            pages[path] = page;
    }

    return pages;
}


int nextSheetPage( const std::map<std::string, std::string>& aExistingPages )
{
    int page = 2;

    for( const auto& item : aExistingPages )
    {
        try
        {
            page = std::max( page, std::stoi( item.second ) + 1 );
        }
        catch( const std::exception& )
        {
        }
    }

    return page;
}


struct SCH_HIERARCHY_INSTANCE_BUILD
{
    std::unique_ptr<SEXPR::NODE> SheetInstances = listNode( "sheet_instances" );
    std::unique_ptr<SEXPR::NODE> SymbolInstances = listNode( "symbol_instances" );
    std::map<std::string, std::string> ExistingPages;
    std::map<std::string, std::unique_ptr<SEXPR::NODE>> ExistingSymbols;
    std::set<std::string> AddedSheetPaths;
    std::set<std::string> ActiveFiles;
    int NextPage = 2;
};


void collectExistingSymbolInstances( SEXPR::NODE* aRoot, SCH_HIERARCHY_INSTANCE_BUILD& aBuild )
{
    SEXPR::NODE* symbolInstances = aRoot ? aRoot->ChildList( "symbol_instances" ) : nullptr;

    if( !symbolInstances )
        return;

    for( const std::unique_ptr<SEXPR::NODE>& child : symbolInstances->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "path" )
            continue;

        std::string path = child->AtomAt( 1 );

        if( !path.empty() && aBuild.ExistingSymbols.count( path ) == 0 )
            aBuild.ExistingSymbols[path] = cloneNode( child.get() );
    }
}


void collectKiCad6HierarchyInstances( const FS::path& aSchematic,
                                      SEXPR::NODE* aRoot,
                                      const std::string& aPrefix,
                                      SCH_HIERARCHY_INSTANCE_BUILD& aBuild )
{
    collectExistingSymbolInstances( aRoot, aBuild );

    for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "symbol"
            && child->ChildList( "lib_id" ) )
        {
            std::string uuid = childAtomOrEmpty( child.get(), "uuid" );

            if( !uuid.empty() )
            {
                std::string path = appendInstanceUuid( aPrefix, uuid );
                auto existing = aBuild.ExistingSymbols.find( path );

                if( existing != aBuild.ExistingSymbols.end() )
                    aBuild.SymbolInstances->Children.push_back( cloneNode( existing->second.get() ) );
                else
                    aBuild.SymbolInstances->Children.push_back( symbolInstanceNode( path, child.get() ) );
            }
        }
    }

    for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "sheet" )
            continue;

        std::string uuid = childAtomOrEmpty( child.get(), "uuid" );

        if( uuid.empty() )
            continue;

        std::string sheetPath = appendInstanceUuid( aPrefix, uuid );

        if( aBuild.AddedSheetPaths.insert( sheetPath ).second )
        {
            std::string page;
            auto found = aBuild.ExistingPages.find( sheetPath );

            if( found != aBuild.ExistingPages.end() )
                page = found->second;
            else
                page = std::to_string( aBuild.NextPage++ );

            aBuild.SheetInstances->Children.push_back( sheetInstanceNode( sheetPath, page ) );
        }

        std::string sheetFile = sheetFilePropertyValue( child.get() );

        if( sheetFile.empty() )
            continue;

        FS::path childPath = aSchematic.parent_path() / sheetFile;

        if( !FS::exists( childPath ) )
            continue;

        std::string activeKey = FS::absolute( childPath ).lexically_normal().string();

        if( aBuild.ActiveFiles.count( activeKey ) != 0 )
            continue;

        aBuild.ActiveFiles.insert( activeKey );

        std::string text = ReadTextFile( childPath );
        std::unique_ptr<SEXPR::NODE> childRoot = SEXPR::Parse( text );
        collectKiCad6HierarchyInstances( childPath, childRoot.get(), sheetPath, aBuild );

        aBuild.ActiveFiles.erase( activeKey );
    }
}


bool hasTopLevelSheet( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return false;

    for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "sheet" )
            return true;
    }

    return false;
}


void replaceRootInstances( SEXPR::NODE* aRoot, SCH_HIERARCHY_INSTANCE_BUILD& aBuild )
{
    uniquifyRepeatedSymbolInstanceReferences( aBuild.SymbolInstances.get() );

    std::vector<std::unique_ptr<SEXPR::NODE>> kept;
    kept.reserve( aRoot->Children.size() + 2 );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom()
            && ( child->HeadView() == "sheet_instances"
                 || child->HeadView() == "symbol_instances" ) )
        {
            continue;
        }

        kept.push_back( std::move( child ) );
    }

    kept.push_back( std::move( aBuild.SheetInstances ) );
    kept.push_back( std::move( aBuild.SymbolInstances ) );
    aRoot->Children = std::move( kept );
}


bool rebuildKiCad6HierarchyInstances( const FS::path& aRootSchematic )
{
    std::string text = ReadTextFile( aRootSchematic );
    std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( text );

    if( !root || root->HeadView() != "kicad_sch" || !root->ChildList( "sheet_instances" )
        || !hasTopLevelSheet( root.get() ) )
    {
        return false;
    }

    SCH_HIERARCHY_INSTANCE_BUILD build;
    build.ExistingPages = existingSheetInstancePages( root.get() );
    build.NextPage = nextSheetPage( build.ExistingPages );
    build.SheetInstances->Children.push_back( sheetInstanceNode( "/", "1" ) );
    build.AddedSheetPaths.insert( "/" );
    build.ActiveFiles.insert( FS::absolute( aRootSchematic ).lexically_normal().string() );

    collectKiCad6HierarchyInstances( aRootSchematic, root.get(), "", build );
    replaceRootInstances( root.get(), build );
    WriteTextFile( aRootSchematic, SEXPR::Format( root.get(), text.size() ) );
    return true;
}


void rebuildKiCad6ProjectHierarchyInstances( const std::vector<PROJECT_COPY_ENTRY>& aEntries )
{
    for( const PROJECT_COPY_ENTRY& entry : aEntries )
    {
        if( Lower( entry.Output.extension().string() ) != ".kicad_sch" )
            continue;

        rebuildKiCad6HierarchyInstances( entry.Output );
    }
}


DOCUMENT loadDocumentImpl( const FS::path& aPath, long long* aReadUs,
                           long long* aParseUs )
{
    DOCUMENT doc;
    doc.Path = aPath;

    CLOCK::time_point readStart = CLOCK::now();
    std::string text = ReadTextFile( aPath );
    CLOCK::time_point readEnd = CLOCK::now();

    doc.SourceBytes = text.size();

    KIND extensionKind = DetectKind( aPath, std::string() );

    if( extensionKind == KIND::PROJECT )
    {
        doc.RawText = std::move( text );
        doc.Kind = KIND::PROJECT;
        doc.Version = "kicad-project-json";

        if( aReadUs )
            *aReadUs = elapsedMicros( readStart, readEnd );

        if( aParseUs )
            *aParseUs = 0;

        return doc;
    }

    if( IsLegacyKind( extensionKind ) )
    {
        doc = LoadLegacyDocument( aPath, std::move( text ) );

        if( aReadUs )
            *aReadUs = elapsedMicros( readStart, readEnd );

        if( aParseUs )
            *aParseUs = 0;

        return doc;
    }

    CLOCK::time_point parseStart = CLOCK::now();
    doc.Root = SEXPR::Parse( text );
    CLOCK::time_point parseEnd = CLOCK::now();

    doc.Kind = DetectKind( aPath, doc.Root->Head() );

    if( SEXPR::NODE* version = doc.Root->ChildList( "version" ) )
        doc.Version = version->AtomAt( 1 );

    if( aReadUs )
        *aReadUs = elapsedMicros( readStart, readEnd );

    if( aParseUs )
        *aParseUs = elapsedMicros( parseStart, parseEnd );

    return doc;
}


void printTiming( const FS::path& aPath, long long aReadUs, long long aParseUs,
                  long long aRulesUs, long long aFormatUs, long long aWriteUs,
                  long long aTotalUs )
{
    auto ms = []( long long us )
    {
        return static_cast<double>( us ) / 1000.0;
    };

    std::cerr << "timing: " << aPath.string()
              << " read=" << ms( aReadUs )
              << "ms parse=" << ms( aParseUs )
              << "ms rules=" << ms( aRulesUs )
              << "ms format=" << ms( aFormatUs )
              << "ms write=" << ms( aWriteUs )
              << "ms total=" << ms( aTotalUs ) << "ms\n";
}

} // namespace

// Dispatches subcommands and keeps all fatal errors on stderr for script use.
int CONVERTER::Run( int aArgc, char** aArgv )
{
    try
    {
        if( aArgc < 2 )
        {
            printUsage();
            return 2;
        }

        std::string command = aArgv[1];

        if( command == "version" || command == "--version" || command == "-version" )
        {
            std::cout << VERSION << '\n';
            return 0;
        }

        std::vector<std::string> args;

        for( int i = 2; i < aArgc; ++i )
            args.emplace_back( aArgv[i] );

        if( command == "convert" )
            return runConvert( args );

        if( command == "inspect" )
            return runInspect( args );

        if( command == "detect-versions" || command == "versions" )
            return runDetectVersions( args );

        printUsage();
        return 2;
    }
    catch( const std::exception& e )
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}


void CONVERTER::printUsage() const
{
    std::cerr << "usage:\n";
    std::cerr << "  kicad-backport convert [--quiet] --target-version <4.0|5.0|5.1|6.0|7.0|8.0|9.0|10.0|10.99|number> <input> <output>\n";
    std::cerr << "  kicad-backport inspect <input>\n";
    std::cerr << "  kicad-backport detect-versions [--json] <input>\n";
    std::cerr << "  kicad-backport version\n";
}


DOCUMENT CONVERTER::loadDocument( const FS::path& aPath ) const
{
    return loadDocumentImpl( aPath, nullptr, nullptr );
}


void CONVERTER::ensureVersion( DOCUMENT& aDocument, const std::string& aVersion ) const
{
    // KiCad expects the version field near the root head when it is present.
    if( SEXPR::NODE* version = aDocument.Root->ChildList( "version" ) )
    {
        if( !version->SetAtomAt( 1, aVersion, false ) )
            throw std::runtime_error( "document has an invalid top-level version field" );
    }
    else
    {
        std::unique_ptr<SEXPR::NODE> versionNode = SEXPR::NODE::MakeList();
        versionNode->Children.push_back( SEXPR::NODE::MakeAtom( "version" ) );
        versionNode->Children.push_back( SEXPR::NODE::MakeAtom( aVersion ) );

        if( aDocument.Root->Children.empty() )
            aDocument.Root->Children.push_back( std::move( versionNode ) );
        else
            aDocument.Root->Children.insert( aDocument.Root->Children.begin() + 1, std::move( versionNode ) );
    }

    aDocument.Version = aVersion;
}


FILE_REPORT CONVERTER::normalizeFile( const FS::path& aInput,
                                      const FS::path& aOutput,
                                      const std::string& aTarget,
                                      bool aPrintWarnings )
{
    const bool emitTiming = timingEnabled();
    CLOCK::time_point totalStart = CLOCK::now();
    long long readUs = 0;
    long long parseUs = 0;
    long long rulesUs = 0;
    long long formatUs = 0;
    long long writeUs = 0;

    DOCUMENT doc = loadDocumentImpl( aInput, &readUs, &parseUs );
    FILE_REPORT report;
    report.Path = aOutput.string();
    report.InputPath = aInput.string();
    report.OutputPath = aOutput.string();
    report.Kind = KindName( doc.Kind );
    report.SourceKind = report.Kind;
    report.TargetKind = report.Kind;
    report.SourceVersion = doc.Version;

    int targetMajor = TargetMajorVersion( aTarget );

    auto emitWarnings = [&]()
    {
        if( !aPrintWarnings )
            return;

        for( const std::string& warning : report.Warnings )
            std::cerr << "warning: " << aInput.string() << ": " << warning << '\n';
    };

    auto writeTextResult = [&]( const std::string& aText )
    {
        CLOCK::time_point writeStart = CLOCK::now();
        WriteTextFile( aOutput, aText );
        CLOCK::time_point writeEnd = CLOCK::now();
        writeUs = elapsedMicros( writeStart, writeEnd );

        if( emitTiming )
            printTiming( aInput, readUs, parseUs, rulesUs, formatUs, writeUs,
                         elapsedMicros( totalStart, CLOCK::now() ) );
    };

    if( IsLegacyKind( doc.Kind ) )
    {
        if( targetMajor <= 5 )
        {
            report.Warnings = {};
            std::string text = RewriteLegacyTextForTarget( doc, targetMajor, &report.Warnings );
            report.TargetKind = report.Kind;
            report.TargetVersion = LegacyTargetVersionForKind( doc.Kind, targetMajor );
            report.Changed = true;
            emitWarnings();
            writeTextResult( text );
            return report;
        }

        KIND targetKind = SexprKindForLegacyKind( doc.Kind );
        std::string resolved;

        if( targetKind == KIND::PROJECT )
            resolved = "kicad-project-json";
        else if( targetKind != KIND::UNKNOWN )
            resolved = ResolveTargetVersion( targetKind, aTarget );
        else
            throw std::runtime_error( "legacy KiCad " + report.Kind
                                      + " conversion is not defined for this target" );

        std::string text = ConvertLegacyToSexprText( doc, resolved, &targetKind,
                                                     &report.Warnings );
        report.TargetKind = targetKind == KIND::UNKNOWN ? report.Kind : KindName( targetKind );
        report.TargetVersion = resolved;
        report.Changed = true;
        emitWarnings();
        writeTextResult( text );
        return report;
    }

    if( doc.Kind == KIND::PROJECT && targetMajor > 5 )
    {
        if( aInput != aOutput )
            writeTextResult( doc.RawText );

        report.TargetVersion = doc.Version;
        return report;
    }

    if( targetMajor <= 5 && LegacyKindForSexprKind( doc.Kind ) != KIND::UNKNOWN )
    {
        KIND targetKind = KIND::UNKNOWN;
        std::string text = ConvertSexprToLegacyText( doc, targetMajor, &targetKind,
                                                     &report.Warnings );
        report.TargetKind = KindName( targetKind );
        report.TargetVersion = LegacyTargetVersionForKind( doc.Kind, targetMajor );
        report.Changed = true;

        if( targetKind == KIND::LEGACY_SYMBOL_LIBRARY )
        {
            FS::path dcmPath = aOutput.parent_path()
                    / ( aOutput.stem().string() + ".dcm" );
            WriteTextFile( dcmPath, LegacyDocumentationSidecarText( doc, targetMajor,
                                                                    &report.Warnings ) );
        }

        emitWarnings();
        writeTextResult( text );
        return report;
    }

    std::string resolved = resolveDocumentTargetVersion( doc, aTarget );
    int source = IsNumber( doc.Version ) ? std::stoi( doc.Version ) : 0;
    int target = std::stoi( resolved );

    if( source > 0 && source == target )
    {
        if( aInput != aOutput )
            FS::copy_file( aInput, aOutput, FS::copy_options::overwrite_existing );

        report.TargetVersion = resolved;
        return report;
    }

    CLOCK::time_point rulesStart = CLOCK::now();

    if( source > 0 && source < target )
    {
        report.Warnings = ApplyUpgradeRules( doc, target );
    }
    else
    {
        if( source > target && ( doc.Kind == KIND::BOARD || doc.Kind == KIND::FOOTPRINT ) )
        {
            EMBEDDED_MODEL_EXPORT_RESULT embeddedResult =
                    externalizeEmbeddedModelsForLegacyTargets( doc, aOutput );
            report.Warnings.insert( report.Warnings.end(), embeddedResult.Warnings.begin(),
                                    embeddedResult.Warnings.end() );
        }

        std::vector<std::string> downgradeWarnings = ApplyDowngradeRules( doc, target );
        report.Warnings.insert( report.Warnings.end(), downgradeWarnings.begin(),
                                downgradeWarnings.end() );
    }

    CLOCK::time_point rulesEnd = CLOCK::now();
    rulesUs = elapsedMicros( rulesStart, rulesEnd );

    ensureVersion( doc, resolved );
    report.TargetVersion = doc.Version;
    report.Changed = true;

    if( aPrintWarnings )
    {
        for( const std::string& warning : report.Warnings )
            std::cerr << "warning: " << aInput.string() << ": " << warning << '\n';
    }

    size_t reserveBytes = doc.SourceBytes + doc.SourceBytes / 10 + 1024;

    CLOCK::time_point formatStart = CLOCK::now();
    std::string formatted = SEXPR::Format( doc.Root.get(), reserveBytes );
    CLOCK::time_point formatEnd = CLOCK::now();
    formatUs = elapsedMicros( formatStart, formatEnd );

    CLOCK::time_point writeStart = CLOCK::now();
    WriteTextFile( aOutput, formatted );
    CLOCK::time_point writeEnd = CLOCK::now();
    writeUs = elapsedMicros( writeStart, writeEnd );

    if( emitTiming )
        printTiming( aInput, readUs, parseUs, rulesUs, formatUs, writeUs,
                     elapsedMicros( totalStart, CLOCK::now() ) );

    return report;
}


bool CONVERTER::isKiCadDocumentPath( const FS::path& aPath ) const
{
    std::string ext = Lower( aPath.extension().string() );
    return ext == ".kicad_sch" || ext == ".kicad_pcb" || ext == ".kicad_sym"
           || ext == ".kicad_mod" || ext == ".kicad_dru" || ext == ".kicad_wks"
           || ext == ".kicad_pro" || ext == ".sch" || ext == ".lib" || ext == ".dcm"
           || ext == ".pro";
}


bool CONVERTER::isKiCadProjectFilePath( const FS::path& aPath ) const
{
    std::string name = Lower( aPath.filename().string() );

    // Keep only source files needed to reopen and edit the converted KiCad project.
    if( name.empty() || StartsWith( name, ".#" ) || EndsWith( name, "~" ) )
        return false;

    std::string ext = Lower( aPath.extension().string() );

    if( ext == ".bak" || ext == ".backup" || ext == ".bck" || ext == ".orig"
        || ext == ".tmp" || ext == ".temp" )
    {
        return false;
    }

    static const std::set<std::string> modelExts = {
        ".step", ".stp", ".wrl", ".iges", ".igs", ".stl", ".obj"
    };

    return name == "fp-lib-table" || name == "sym-lib-table" || ext == ".kicad_pro"
           || ext == ".kicad_prl" || isKiCadDocumentPath( aPath )
           || modelExts.count( ext ) != 0;
}


bool CONVERTER::isExcludedProjectDirName( const std::string& aName ) const
{
    std::string name = Lower( aName );

    static const std::set<std::string> excluded = {
        ".git", ".svn", ".hg", ".history", ".backup", "__pycache__", "history",
        "histories", "backup", "backups", "archive", "archives", "old", "gerber",
        "gerbers", "gerberfiles", "gerber_files", "fab", "fabrication", "outputs",
        "production", "plot", "plots", "export", "exports", "bom", "ibom",
        "assembly", "jlcpcb", "oshpark"
    };

    return excluded.count( name ) || name.find( "backup" ) != std::string::npos
           || name.find( "history" ) != std::string::npos
           || name.find( "gerber" ) != std::string::npos;
}


std::vector<PROJECT_COPY_ENTRY> CONVERTER::copyProjectTree( const FS::path& aInput,
                                                            const FS::path& aOutput,
                                                            const std::string& aTarget ) const
{
    FS::path src = FS::absolute( aInput );
    FS::path dest = FS::absolute( aOutput );

    if( src == dest )
        throw std::runtime_error( "output directory must differ from input directory" );

    // Copy only editable KiCad project inputs, not generated manufacturing outputs.
    std::vector<PROJECT_COPY_ENTRY> copied;
    int targetMajor = TargetMajorVersion( aTarget );

    FS::recursive_directory_iterator it( src );
    const FS::recursive_directory_iterator end;

    for( ; it != end; ++it )
    {
        const FS::directory_entry& entry = *it;
        FS::path rel = FS::relative( entry.path(), src );

        if( entry.is_directory() )
        {
            if( isExcludedProjectDirName( entry.path().filename().string() ) )
            {
                it.disable_recursion_pending();
                continue;
            }

            continue;
        }

        if( !entry.is_regular_file() || !isKiCadProjectFilePath( entry.path() ) )
            continue;

        std::string ext = Lower( entry.path().extension().string() );

        if( targetMajor <= 5 && ext == ".kicad_prl" )
            continue;

        bool isDocument = isKiCadDocumentPath( entry.path() );
        bool hasCopyReport = false;
        FILE_REPORT copyReport;

        if( ext == ".kicad_dru" )
        {
            FILE_REPORT report = detectVersionFast( entry.path() );
            std::string targetVersion;

            try
            {
                targetVersion = ResolveTargetVersion( KIND::DESIGN_RULES, aTarget );
            }
            catch( const std::runtime_error& )
            {
                report.Path = entry.path().string();
                report.OutputPath.clear();
                report.TargetVersion = "unsupported";
                report.Warnings.push_back( "skipped design-rules file because the target KiCad "
                                           + aTarget + " format does not support .kicad_dru" );
                PROJECT_COPY_ENTRY skipped;
                skipped.Source = entry.path();
                skipped.Output = dest / rel;
                skipped.HasReport = true;
                skipped.Report = report;
                copied.push_back( skipped );
                continue;
            }

            if( report.SourceVersion != targetVersion )
            {
                throw std::runtime_error( "design-rules conversion is not implemented for "
                                          + entry.path().string() + " from version "
                                          + report.SourceVersion + " to " + targetVersion );
            }

            report.TargetVersion = targetVersion;
            copyReport = report;
            hasCopyReport = true;
            isDocument = false;
        }

        if( isDocument && targetMajor > 5 && ext == ".dcm" )
        {
            FS::path pairedLib = entry.path();
            pairedLib.replace_extension( ".lib" );

            if( FS::exists( pairedLib ) )
                continue;
        }

        FS::path out = dest / rel;

        if( isDocument )
            out = withTargetFamilyExtension( out, aTarget );

        FS::create_directories( out.parent_path() );

        if( !isDocument )
            FS::copy_file( entry.path(), out, FS::copy_options::overwrite_existing );

        PROJECT_COPY_ENTRY copiedEntry;
        copiedEntry.Source = entry.path();
        copiedEntry.Output = out;
        copiedEntry.IsDocument = isDocument;

        if( hasCopyReport )
        {
            copyReport.Path = out.string();
            copyReport.OutputPath = out.string();
            copiedEntry.HasReport = true;
            copiedEntry.Report = copyReport;
        }

        copied.push_back( copiedEntry );
    }

    return copied;
}


FS::path CONVERTER::versionedOutputPath( const FS::path& aPath,
                                                      const std::string& aTarget ) const
{
    std::string label = TargetVersionSuffix( aTarget );
    FS::path targetPath = withTargetFamilyExtension( aPath, aTarget );

    if( label.empty() )
        return targetPath;

    std::string stem = targetPath.stem().string();

    if( EndsWith( Lower( stem ), Lower( "_" + label ) ) )
        return targetPath;

    return targetPath.parent_path() / ( stem + "_" + label + targetPath.extension().string() );
}


void CONVERTER::writeReport( const FS::path& aPath,
                             const std::vector<FILE_REPORT>& aReports ) const
{
    WriteTextFile( aPath, FormatReportsJson( aReports ) );
}


void printReportWarnings( const std::vector<FILE_REPORT>& aReports )
{
    for( const FILE_REPORT& report : aReports )
    {
        for( const std::string& warning : report.Warnings )
            std::cerr << "warning: " << report.Path << ": " << warning << '\n';
    }
}


void printReportVersions( const std::vector<FILE_REPORT>& aReports )
{
    for( const FILE_REPORT& report : aReports )
    {
        std::string inputPath = report.InputPath.empty() ? report.Path : report.InputPath;
        std::string sourceKind = report.SourceKind.empty() ? report.Kind : report.SourceKind;
        std::string targetKind = report.TargetKind.empty() ? report.Kind : report.TargetKind;
        std::string sourceVersion = displayReportVersion( sourceKind, report.SourceVersion,
                                                          "unknown" );
        std::string targetVersion = displayReportVersion( targetKind, report.TargetVersion,
                                                          "unchanged" );

        std::cout << "version: " << inputPath << " [" << sourceKind << " -> " << targetKind
                  << "] " << sourceVersion << " -> " << targetVersion << '\n';
    }
}


std::string extractJsonStringField( const std::string& aText, const std::string& aName )
{
    std::string needle = "\"" + aName + "\"";
    size_t pos = aText.find( needle );

    if( pos == std::string::npos )
        return "";

    pos = aText.find( ':', pos + needle.size() );

    if( pos == std::string::npos )
        return "";

    pos = aText.find( '"', pos );

    if( pos == std::string::npos )
        return "";

    size_t start = ++pos;
    bool escape = false;
    std::string out;

    for( ; pos < aText.size(); ++pos )
    {
        char ch = aText[pos];

        if( escape )
        {
            out.push_back( ch );
            escape = false;
        }
        else if( ch == '\\' )
        {
            escape = true;
        }
        else if( ch == '"' )
        {
            return out;
        }
        else
        {
            out.push_back( ch );
        }
    }

    (void) start;
    return "";
}


int clearLegacyEmbeddedPageLayoutRefs( std::string& aText )
{
    int changed = 0;
    std::string needle = "\"page_layout_descr_file\"";
    size_t pos = 0;

    while( ( pos = aText.find( needle, pos ) ) != std::string::npos )
    {
        size_t colon = aText.find( ':', pos + needle.size() );

        if( colon == std::string::npos )
            break;

        size_t quote = aText.find( '"', colon + 1 );

        if( quote == std::string::npos )
            break;

        size_t valueStart = quote + 1;
        size_t valueEnd = valueStart;
        bool escape = false;

        for( ; valueEnd < aText.size(); ++valueEnd )
        {
            char ch = aText[valueEnd];

            if( escape )
            {
                escape = false;
            }
            else if( ch == '\\' )
            {
                escape = true;
            }
            else if( ch == '"' )
            {
                break;
            }
        }

        if( valueEnd >= aText.size() )
            break;

        std::string value = aText.substr( valueStart, valueEnd - valueStart );

        if( StartsWith( value, "kicad-embed://" ) && EndsWith( Lower( value ), ".kicad_wks" ) )
        {
            aText.erase( valueStart, valueEnd - valueStart );
            ++changed;
            pos = valueStart + 1;
        }
        else
        {
            pos = valueEnd + 1;
        }
    }

    return changed;
}


std::vector<std::string> extractJsonArrayItems( const std::string& aText,
                                                const std::string& aName )
{
    std::vector<std::string> items;
    std::string needle = "\"" + aName + "\"";
    size_t pos = aText.find( needle );

    if( pos == std::string::npos )
        return items;

    pos = aText.find( '[', pos + needle.size() );

    if( pos == std::string::npos )
        return items;

    ++pos;
    std::string token;
    bool inString = false;
    bool escape = false;

    auto flushToken = [&]()
    {
        std::string value = Trim( token );

        if( !value.empty() )
            items.push_back( value );

        token.clear();
    };

    for( ; pos < aText.size(); ++pos )
    {
        char ch = aText[pos];

        if( inString )
        {
            if( escape )
            {
                token.push_back( ch );
                escape = false;
            }
            else if( ch == '\\' )
            {
                escape = true;
            }
            else if( ch == '"' )
            {
                inString = false;
                flushToken();
            }
            else
            {
                token.push_back( ch );
            }

            continue;
        }

        if( ch == '"' )
        {
            inString = true;
            token.clear();
        }
        else if( ch == ',' )
        {
            flushToken();
        }
        else if( ch == ']' )
        {
            flushToken();
            break;
        }
        else
        {
            token.push_back( ch );
        }
    }

    return items;
}


std::vector<int> defaultLegacyVisibleItems()
{
    return {
        0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41
    };
}


int legacyVisibleItemId( const std::string& aName )
{
    static const std::map<std::string, int> itemIds = {
        { "vias", 0 },
        { "via_holes", 1 },
        { "through_via_holes", 1 },
        { "blind_buried_via_holes", 2 },
        { "micro_via_holes", 3 },
        { "non_plated_holes", 4 },
        { "drill_holes", 5 },
        { "footprint_text", 6 },
        { "footprint_anchors", 8 },
        { "ratsnest", 11 },
        { "grid", 12 },
        { "footprints_front", 15 },
        { "footprints_back", 16 },
        { "footprint_values", 17 },
        { "footprint_references", 18 },
        { "tracks", 19 },
        { "drc_errors", 20 },
        { "drawing_sheet", 23 },
        { "bitmaps", 24 },
        { "pads", 30 },
        { "zones", 32 },
        { "drc_warnings", 33 },
        { "drc_exclusions", 36 },
        { "locked_item_shadows", 37 },
        { "ly_points", 38 },
        { "conflict_shadows", 39 },
        { "shapes", 40 },
        { "board_outline_area", 41 }
    };

    auto it = itemIds.find( aName );
    return it == itemIds.end() ? -1 : it->second;
}


std::vector<int> legacyVisibleItemsFromSource( const FS::path& aSourcePath )
{
    if( aSourcePath.empty() || !FS::exists( aSourcePath ) )
        return {};

    std::vector<int> result;

    try
    {
        std::vector<std::string> items = extractJsonArrayItems( ReadTextFile( aSourcePath ),
                                                                "visible_items" );
        bool seededDefaults = false;

        for( const std::string& item : items )
        {
            int id = -1;

            if( IsNumber( item ) )
            {
                id = std::stoi( item );
            }
            else
            {
                if( !seededDefaults )
                {
                    std::vector<int> explicitItems = result;
                    result = defaultLegacyVisibleItems();

                    for( int explicitItem : explicitItems )
                    {
                        if( std::find( result.begin(), result.end(), explicitItem )
                            == result.end() )
                        {
                            result.push_back( explicitItem );
                        }
                    }

                    seededDefaults = true;
                }

                id = legacyVisibleItemId( item );
            }

            if( id >= 0 && std::find( result.begin(), result.end(), id ) == result.end() )
                result.push_back( id );
        }
    }
    catch( ... )
    {
        return {};
    }

    return result;
}


std::string visibleLayersFromSource( const FS::path& aSourcePath )
{
    if( aSourcePath.empty() || !FS::exists( aSourcePath ) )
        return "";

    try
    {
        return extractJsonStringField( ReadTextFile( aSourcePath ), "visible_layers" );
    }
    catch( ... )
    {
        return "";
    }
}


std::string normalizeLegacyVisibleLayers( const std::string& aValue )
{
    std::string digits;

    for( char ch : aValue )
    {
        if( ch == '_' )
            continue;

        if( !std::isxdigit( static_cast<unsigned char>( ch ) ) )
            return "";

        digits.push_back( static_cast<char>( std::tolower( static_cast<unsigned char>( ch ) ) ) );
    }

    if( digits.empty() )
        return "";

    if( digits.size() > 15 )
        digits = digits.substr( digits.size() - 15 );
    else if( digits.size() < 15 )
        digits.insert( digits.begin(), 15 - digits.size(), '0' );

    return digits.substr( 0, 7 ) + "_" + digits.substr( 7 );
}


void CONVERTER::ensureLegacyProjectLocalSettings( const FS::path& aPath,
                                                  const std::string& aTargetSuffix,
                                                  const FS::path& aSourcePath ) const
{
    // KiCad 6/7/8 expect numeric visible item IDs; newer string IDs hide many objects.
    int metaVersion = ( aTargetSuffix == "V6" || aTargetSuffix == "V7"
                        || aTargetSuffix == "V8" ) ? 3 : 4;
    std::vector<int> items = legacyVisibleItemsFromSource( aSourcePath );

    if( items.empty() )
        items = defaultLegacyVisibleItems();

    std::string visibleLayers = visibleLayersFromSource( aSourcePath );

    visibleLayers = normalizeLegacyVisibleLayers( visibleLayers );

    if( visibleLayers.empty() )
        visibleLayers = "fffffff_ffffffff";

    std::ostringstream out;
    out << "{\n";
    out << "  \"board\": {\n";
    out << "    \"visible_items\": [\n";

    for( size_t i = 0; i < items.size(); ++i )
    {
        out << "      " << items[i];

        if( i + 1 < items.size() )
            out << ",";

        out << "\n";
    }

    out << "    ],\n";
    out << "    \"visible_layers\": \"" << JsonEscape( visibleLayers ) << "\"\n";
    out << "  },\n";
    out << "  \"meta\": {\n";
    out << "    \"filename\": \"" << JsonEscape( aPath.filename().string() ) << "\",\n";
    out << "    \"version\": " << metaVersion << "\n";
    out << "  }\n";
    out << "}\n";

    WriteTextFile( aPath, out.str() );
}


FILE_REPORT CONVERTER::ensureLegacyProjectPageLayoutRefs( const FS::path& aPath,
                                                          const std::string& aTargetSuffix ) const
{
    FILE_REPORT report;
    report.Path = aPath.string();
    report.InputPath = aPath.string();
    report.OutputPath = aPath.string();
    report.Kind = "project";
    report.SourceKind = "project";
    report.TargetKind = "project";
    report.SourceVersion = "kicad-project-json";
    report.TargetVersion = "kicad-project-json";

    if( aTargetSuffix != "V6" && aTargetSuffix != "V7" && aTargetSuffix != "V8" )
        return report;

    if( Lower( aPath.extension().string() ) != ".kicad_pro" || !FS::exists( aPath ) )
        return report;

    std::string text = ReadTextFile( aPath );
    int changed = clearLegacyEmbeddedPageLayoutRefs( text );

    if( changed > 0 )
    {
        WriteTextFile( aPath, text );
        report.Changed = true;
        report.Warnings.push_back(
                "cleared embedded worksheet page layout references for KiCad "
                + aTargetSuffix.substr( 1 ) + " project compatibility" );
    }

    return report;
}


int CONVERTER::runConvert( const std::vector<std::string>& aArgs )
{
    CLOCK::time_point convertStart = CLOCK::now();
    std::string target;
    std::string reportPath;
    bool quiet = false;
    std::vector<std::string> positional;

    for( size_t i = 0; i < aArgs.size(); ++i )
    {
        if( aArgs[i] == "--target-version" && i + 1 < aArgs.size() )
            target = aArgs[++i];
        else if( aArgs[i] == "--report" && i + 1 < aArgs.size() )
            reportPath = aArgs[++i];
        else if( aArgs[i] == "--quiet" || aArgs[i] == "-q" )
            quiet = true;
        else
            positional.push_back( aArgs[i] );
    }

    if( target.empty() )
        throw std::runtime_error( "--target-version is required" );

    if( positional.size() != 2 )
        throw std::runtime_error( "convert requires input and output paths" );

    FS::path input = positional[0];
    FS::path output = versionedOutputPath( positional[1], target );

    std::vector<FILE_REPORT> reports;

    if( FS::is_directory( input ) || Lower( input.extension().string() ) == ".kicad_pro" )
    {
        FS::path srcDir = FS::is_directory( input ) ? input : input.parent_path();
        std::vector<PROJECT_COPY_ENTRY> copied = copyProjectTree( srcDir, output, target );
        std::vector<PROJECT_COPY_ENTRY> documents;

        for( const PROJECT_COPY_ENTRY& entry : copied )
        {
            if( entry.HasReport )
                reports.push_back( entry.Report );

            if( entry.IsDocument )
                documents.push_back( entry );
        }

        for( const PROJECT_COPY_ENTRY& entry : documents )
            reports.push_back( normalizeFile( entry.Source, entry.Output, target, false ) );

        if( !quiet )
            printReportWarnings( reports );

        std::string suffix = TargetVersionSuffix( target );
        int targetMajor = TargetMajorVersion( target );

        if( suffix == "V6" || suffix == "V7" || suffix == "V8" )
        {
            for( const PROJECT_COPY_ENTRY& entry : copied )
            {
                if( Lower( entry.Output.extension().string() ) == ".kicad_pcb" )
                    ensureLegacyProjectLocalSettings( ReplaceExtension( entry.Output, ".kicad_prl" ),
                                                      suffix,
                                                      ReplaceExtension( entry.Source, ".kicad_prl" ) );
                else if( Lower( entry.Output.extension().string() ) == ".kicad_prl" )
                    ensureLegacyProjectLocalSettings( entry.Output, suffix, entry.Source );
                else if( Lower( entry.Output.extension().string() ) == ".kicad_pro" )
                {
                    FILE_REPORT projectReport = ensureLegacyProjectPageLayoutRefs( entry.Output,
                                                                                   suffix );

                    if( projectReport.Changed || !projectReport.Warnings.empty() )
                        reports.push_back( projectReport );
                }
            }
        }

        if( targetMajor > 5 )
        {
            rebuildKiCad6ProjectHierarchyInstances( copied );
        }

        if( targetMajor > 5 )
        {
            std::set<FS::path> projectDirs;

            for( const PROJECT_COPY_ENTRY& entry : copied )
            {
                if( Lower( entry.Output.extension().string() ) == ".kicad_sym" )
                    projectDirs.insert( entry.Output.parent_path() );
            }

            for( const FS::path& projectDir : projectDirs )
            {
                FILE_REPORT tableReport = ensureProjectLocalSymbolLibraryTable( projectDir, copied,
                                                                                targetMajor );
                std::vector<std::string> embedWarnings;
                int embedded = embedProjectLocalSchematicSymbols( projectDir, copied,
                                                                  embedWarnings );

                if( embedded > 0 )
                {
                    tableReport.Changed = true;
                    tableReport.Warnings.push_back(
                            "embedded generated schematic symbols for standalone loading" );
                }

                tableReport.Warnings.insert( tableReport.Warnings.end(), embedWarnings.begin(),
                                             embedWarnings.end() );
                reports.push_back( tableReport );
            }
        }

        if( targetMajor <= 6 )
        {
            for( const PROJECT_COPY_ENTRY& entry : copied )
            {
                if( !entry.IsDocument && isLibraryTablePath( entry.Output ) )
                {
                    std::vector<std::string> aliasWarnings;
                    int aliasChanges = ensureLegacyFootprintLibraryAliases( entry.Output, copied,
                                                                            targetMajor,
                                                                            aliasWarnings );
                    FILE_REPORT tableReport = normalizeLegacyLibraryTable( entry.Output, targetMajor );
                    tableReport.Changed = tableReport.Changed || aliasChanges > 0;
                    tableReport.Warnings.insert( tableReport.Warnings.end(), aliasWarnings.begin(),
                                                 aliasWarnings.end() );
                    reports.push_back( tableReport );
                }
            }
        }

        if( targetMajor <= 5 )
        {
            std::set<FS::path> projectDirs;

            for( const PROJECT_COPY_ENTRY& entry : copied )
            {
                if( Lower( entry.Output.extension().string() ) == ".pro" )
                    projectDirs.insert( entry.Output.parent_path() );
            }

            for( const FS::path& projectDir : projectDirs )
            {
                FILE_REPORT cacheReport = ensureLegacySchematicCacheLibrary( projectDir, copied );

                if( cacheReport.Changed || !cacheReport.Warnings.empty() )
                    reports.push_back( cacheReport );
            }
        }
    }
    else
    {
        if( FS::absolute( input ) == FS::absolute( output ) )
            throw std::runtime_error( "output file must differ from input file" );

        reports.push_back( normalizeFile( input, output, target, !quiet ) );

        std::string suffix = TargetVersionSuffix( target );

        if( ( suffix == "V6" || suffix == "V7" || suffix == "V8" )
            && Lower( output.extension().string() ) == ".kicad_pcb" )
            ensureLegacyProjectLocalSettings( ReplaceExtension( output, ".kicad_prl" ), suffix,
                                              ReplaceExtension( input, ".kicad_prl" ) );
    }

    if( !quiet )
        printReportVersions( reports );

    if( !reportPath.empty() )
        writeReport( reportPath, reports );

    size_t changed = 0;

    for( const FILE_REPORT& report : reports )
    {
        if( report.Changed )
            ++changed;
    }

    std::cout << "wrote " << output.string() << "; normalized " << changed << " KiCad file(s)\n";

    if( timingEnabled() )
        std::cerr << "timing: convert total="
                  << static_cast<double>( elapsedMicros( convertStart, CLOCK::now() ) ) / 1000.0
                  << "ms\n";

    return 0;
}


std::vector<FILE_REPORT> CONVERTER::inspectPath( const FS::path& aPath ) const
{
    std::vector<FILE_REPORT> reports;

    if( !FS::is_directory( aPath ) && Lower( aPath.extension().string() ) != ".kicad_pro" )
    {
        DOCUMENT doc = loadDocument( aPath );
        FILE_REPORT report;
        report.Path = aPath.string();
        report.InputPath = aPath.string();
        report.OutputPath = aPath.string();
        report.Kind = KindName( doc.Kind );
        report.SourceKind = report.Kind;
        report.TargetKind = report.Kind;
        report.SourceVersion = doc.Version;
        reports.push_back( std::move( report ) );
        return reports;
    }

    FS::path root = FS::is_directory( aPath ) ? aPath : aPath.parent_path();

    for( FS::recursive_directory_iterator it( root ), end; it != end; ++it )
    {
        const FS::directory_entry& entry = *it;

        if( !entry.is_regular_file() || !isKiCadDocumentPath( entry.path() ) )
            continue;

        DOCUMENT doc = loadDocument( entry.path() );
        FILE_REPORT report;
        report.Path = entry.path().string();
        report.InputPath = entry.path().string();
        report.OutputPath = entry.path().string();
        report.Kind = KindName( doc.Kind );
        report.SourceKind = report.Kind;
        report.TargetKind = report.Kind;
        report.SourceVersion = doc.Version;
        reports.push_back( std::move( report ) );
    }

    return reports;
}


std::vector<FILE_REPORT> CONVERTER::detectVersionsPath( const FS::path& aPath ) const
{
    std::vector<FILE_REPORT> reports;

    if( !FS::is_directory( aPath ) && Lower( aPath.extension().string() ) != ".kicad_pro" )
    {
        if( !isKiCadDocumentPath( aPath ) )
            return reports;

        FILE_REPORT report = detectVersionFast( aPath );

        if( hasDetectedVersion( report ) )
            reports.push_back( std::move( report ) );

        return reports;
    }

    FS::path root = FS::is_directory( aPath ) ? aPath : aPath.parent_path();

    for( FS::recursive_directory_iterator it( root ), end; it != end; ++it )
    {
        const FS::directory_entry& entry = *it;

        if( entry.is_directory() )
        {
            if( isExcludedProjectDirName( entry.path().filename().string() ) )
                it.disable_recursion_pending();

            continue;
        }

        if( !entry.is_regular_file() || !isKiCadDocumentPath( entry.path() ) )
            continue;

        FILE_REPORT report = detectVersionFast( entry.path() );

        if( hasDetectedVersion( report ) )
            reports.push_back( std::move( report ) );
    }

    std::sort( reports.begin(), reports.end(),
               []( const FILE_REPORT& aLeft, const FILE_REPORT& aRight )
               {
                   return aLeft.Path < aRight.Path;
               } );

    return reports;
}


int CONVERTER::runInspect( const std::vector<std::string>& aArgs )
{
    if( aArgs.size() != 1 )
        throw std::runtime_error( "inspect requires one input path" );

    std::cout << FormatReportsJson( inspectPath( aArgs[0] ) );
    return 0;
}


int CONVERTER::runDetectVersions( const std::vector<std::string>& aArgs )
{
    bool json = false;
    std::vector<std::string> positional;

    for( const std::string& arg : aArgs )
    {
        if( arg == "--json" )
            json = true;
        else
            positional.push_back( arg );
    }

    if( positional.size() != 1 )
        throw std::runtime_error( "detect-versions requires one input path" );

    std::vector<FILE_REPORT> reports = detectVersionsPath( positional[0] );

    if( json )
    {
        std::cout << FormatReportsJson( reports );
        return 0;
    }

    std::cout << "kind\tversion\tpath\n";

    for( const FILE_REPORT& report : reports )
    {
        std::cout << report.Kind << '\t'
                  << displayReportVersion( report.Kind, report.SourceVersion, "unknown" )
                  << '\t' << report.Path << '\n';
    }

    return 0;
}

} // namespace KICAD_BACKPORT
