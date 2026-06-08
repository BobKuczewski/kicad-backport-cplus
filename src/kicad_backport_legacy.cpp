#include "kicad_backport/kicad_backport_legacy.h"
#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>


namespace KICAD_BACKPORT
{
namespace
{

struct LEGACY_SCHEMATIC_META
{
    std::string Paper = "A4";
    std::string Title;
    std::string Date;
    std::string Rev;
    std::string Company;
    std::array<std::string, 9> Comments;
};


struct LEGACY_DOC_META
{
    std::string Description;
    std::string Keywords;
    std::string Datasheet;
};


struct LEGACY_PROJECT_META
{
    std::map<std::string, std::string> Settings;
    std::vector<std::string> Libraries;
};


struct LEGACY_SYMBOL_DEF
{
    std::string Reference;
    bool ShowPinNumbers = true;
    bool ShowPinNames = true;
    int UnitCount = 1;
    std::vector<std::pair<int, std::vector<std::string>>> PinWords;
    std::vector<std::pair<int, std::string>> DrawLines;
};


struct LEGACY_SYMBOL_TRANSFORM
{
    int M11 = 1;
    int M12 = 0;
    int M21 = 0;
    int M22 = -1;
};


struct SEXPR_SYMBOL_TRANSFORM
{
    std::string Angle = "0";
    std::string Mirror;
};


std::vector<std::string> linesOf( const std::string& aText )
{
    std::vector<std::string> lines;
    std::string line;
    std::istringstream in( aText );

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        if( lines.empty() && line.size() >= 3
            && static_cast<unsigned char>( line[0] ) == 0xEF
            && static_cast<unsigned char>( line[1] ) == 0xBB
            && static_cast<unsigned char>( line[2] ) == 0xBF )
        {
            line.erase( 0, 3 );
        }

        lines.push_back( line );
    }

    return lines;
}


std::vector<std::string> splitWords( const std::string& aLine )
{
    std::vector<std::string> out;
    std::istringstream in( aLine );
    std::string word;

    while( in >> word )
        out.push_back( word );

    return out;
}


std::string unquoteLegacy( std::string aValue )
{
    aValue = Trim( aValue );

    if( aValue.size() >= 2 && aValue.front() == '"' && aValue.back() == '"' )
        aValue = aValue.substr( 1, aValue.size() - 2 );

    std::string out;
    out.reserve( aValue.size() );
    bool escape = false;

    for( char ch : aValue )
    {
        if( escape )
        {
            out.push_back( ch );
            escape = false;
        }
        else if( ch == '\\' )
        {
            escape = true;
        }
        else
        {
            out.push_back( ch );
        }
    }

    return out;
}


std::string legacyQuote( const std::string& aValue )
{
    std::string out = "\"";

    for( char ch : aValue )
    {
        if( ch == '\\' || ch == '"' )
            out.push_back( '\\' );

        out.push_back( ch );
    }

    out.push_back( '"' );
    return out;
}


uint32_t fnv1a( const std::string& aText )
{
    uint32_t hash = 2166136261u;

    for( unsigned char ch : aText )
    {
        hash ^= ch;
        hash *= 16777619u;
    }

    return hash;
}


std::string deterministicUuid( const std::string& aSeed )
{
    uint32_t a = fnv1a( "a:" + aSeed );
    uint32_t b = fnv1a( "b:" + aSeed );
    uint32_t c = fnv1a( "c:" + aSeed );
    uint32_t d = fnv1a( "d:" + aSeed );

    std::ostringstream out;
    out << std::hex << std::setfill( '0' )
        << std::setw( 8 ) << a << "-"
        << std::setw( 4 ) << ( ( b >> 16 ) & 0xffff ) << "-"
        << std::setw( 4 ) << ( b & 0xffff ) << "-"
        << std::setw( 4 ) << ( ( c >> 16 ) & 0xffff ) << "-"
        << std::setw( 4 ) << ( c & 0xffff )
        << std::setw( 8 ) << d;
    return out.str();
}


std::string legacyUuidFromTstamp( const std::string& aTstamp )
{
    return deterministicUuid( "legacy-tstamp:" + aTstamp );
}


std::string legacyInstancePathFromARPath( const std::string& aPath )
{
    if( aPath.empty() || aPath[0] != '/' )
        return aPath;

    std::string out;
    size_t start = 1;

    while( start <= aPath.size() )
    {
        size_t end = aPath.find( '/', start );
        std::string part = end == std::string::npos
                ? aPath.substr( start ) : aPath.substr( start, end - start );

        if( !part.empty() )
        {
            out += "/";
            out += legacyUuidFromTstamp( part );
        }

        if( end == std::string::npos )
            break;

        start = end + 1;
    }

    return out.empty() ? "/" : out;
}


std::string deterministicTstamp( const std::string& aSeed )
{
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill( '0' ) << std::setw( 8 )
        << fnv1a( aSeed );
    return out.str();
}


bool isLegacyTstamp( const std::string& aValue )
{
    if( aValue.size() != 8 )
        return false;

    for( char ch : aValue )
    {
        if( !( ( ch >= '0' && ch <= '9' ) || ( ch >= 'a' && ch <= 'f' )
               || ( ch >= 'A' && ch <= 'F' ) ) )
        {
            return false;
        }
    }

    return true;
}


std::string legacyTstamp( const std::string& aValue, const std::string& aSeed )
{
    if( isLegacyTstamp( aValue ) )
    {
        std::string out = aValue;
        std::transform( out.begin(), out.end(), out.begin(),
                        []( unsigned char ch ) { return static_cast<char>( std::toupper( ch ) ); } );
        return out;
    }

    if( !aValue.empty() )
        return deterministicTstamp( aValue );

    return deterministicTstamp( aSeed );
}


std::string formatDouble( double aValue )
{
    if( std::abs( aValue ) < 0.0000005 )
        aValue = 0.0;

    std::ostringstream out;
    out << std::fixed << std::setprecision( 6 ) << aValue;
    std::string text = out.str();

    while( text.size() > 1 && text.back() == '0' )
        text.pop_back();

    if( !text.empty() && text.back() == '.' )
        text.pop_back();

    return text.empty() ? "0" : text;
}


double parseDoubleOrZero( const std::string& aValue )
{
    char* end = nullptr;
    double value = std::strtod( aValue.c_str(), &end );
    return end == aValue.c_str() ? 0.0 : value;
}


bool parseIntStrict( const std::string& aValue, int& aOut )
{
    errno = 0;
    char* end = nullptr;
    long value = std::strtol( aValue.c_str(), &end, 10 );

    if( end == aValue.c_str() || *end != '\0' || errno == ERANGE
        || value < std::numeric_limits<int>::min()
        || value > std::numeric_limits<int>::max() )
    {
        return false;
    }

    aOut = static_cast<int>( value );
    return true;
}


int legacyIntAtOrDefault( const std::vector<std::string>& aWords, size_t aIndex, int aDefault )
{
    int value = aDefault;
    return aIndex < aWords.size() && parseIntStrict( aWords[aIndex], value ) ? value : aDefault;
}


int legacyPinUnit( const std::vector<std::string>& aWords )
{
    return legacyIntAtOrDefault( aWords, 9, 1 );
}


int legacyDrawUnit( const std::vector<std::string>& aWords )
{
    if( aWords.empty() )
        return 1;

    if( aWords[0] == "S" )
        return legacyIntAtOrDefault( aWords, 5, 1 );

    if( aWords[0] == "P" )
        return legacyIntAtOrDefault( aWords, 2, 1 );

    if( aWords[0] == "C" )
        return legacyIntAtOrDefault( aWords, 4, 1 );

    if( aWords[0] == "A" )
        return legacyIntAtOrDefault( aWords, 6, 1 );

    if( aWords[0] == "T" )
        return legacyIntAtOrDefault( aWords, 5, 1 );

    return 1;
}


std::string legacyCoordToMm( const std::string& aCoord )
{
    char* end = nullptr;
    double value = std::strtod( aCoord.c_str(), &end );

    if( end == aCoord.c_str() )
        return "0";

    return formatDouble( value * 0.0254 );
}


int mmToLegacyCoord( const std::string& aCoord )
{
    char* end = nullptr;
    double value = std::strtod( aCoord.c_str(), &end );

    if( end == aCoord.c_str() )
        return 0;

    return static_cast<int>( std::llround( value / 0.0254 ) );
}


int roundedRightAngle( const std::string& aAngle )
{
    int angle = static_cast<int>( std::lround( parseDoubleOrZero( aAngle ) ) );
    angle %= 360;

    if( angle < 0 )
        angle += 360;

    int snapped = ( ( angle + 45 ) / 90 ) * 90;
    return snapped % 360;
}


std::string firstQuotedValue( const std::string& aLine )
{
    size_t first = aLine.find( '"' );

    if( first == std::string::npos )
        return "";

    size_t pos = first + 1;
    std::string value;
    bool escape = false;

    while( pos < aLine.size() )
    {
        char ch = aLine[pos++];

        if( escape )
        {
            value.push_back( ch );
            escape = false;
        }
        else if( ch == '\\' )
        {
            escape = true;
        }
        else if( ch == '"' )
        {
            return value;
        }
        else
        {
            value.push_back( ch );
        }
    }

    return value;
}


std::vector<std::string> quotedValues( const std::string& aLine )
{
    std::vector<std::string> values;
    size_t pos = 0;

    while( pos < aLine.size() )
    {
        size_t first = aLine.find( '"', pos );

        if( first == std::string::npos )
            break;

        pos = first + 1;
        std::string value;
        bool escape = false;

        while( pos < aLine.size() )
        {
            char ch = aLine[pos++];

            if( escape )
            {
                value.push_back( ch );
                escape = false;
            }
            else if( ch == '\\' )
            {
                escape = true;
            }
            else if( ch == '"' )
            {
                values.push_back( value );
                break;
            }
            else
            {
                value.push_back( ch );
            }
        }
    }

    return values;
}


std::string quotedAttributeValue( const std::string& aLine, const std::string& aName )
{
    std::string marker = aName + "=\"";
    size_t start = aLine.find( marker );

    if( start == std::string::npos )
        return "";

    start += marker.size();
    size_t end = aLine.find( '"', start );

    if( end == std::string::npos )
        return "";

    return aLine.substr( start, end - start );
}


std::vector<std::string> wordsAfterFirstQuotedValue( const std::string& aLine )
{
    size_t first = aLine.find( '"' );

    if( first == std::string::npos )
        return {};

    bool escape = false;

    for( size_t pos = first + 1; pos < aLine.size(); ++pos )
    {
        char ch = aLine[pos];

        if( escape )
        {
            escape = false;
            continue;
        }

        if( ch == '\\' )
        {
            escape = true;
            continue;
        }

        if( ch == '"' )
            return splitWords( aLine.substr( pos + 1 ) );
    }

    return {};
}


std::string replaceTrailingExtension( const std::string& aValue, const std::string& aFrom,
                                      const std::string& aTo )
{
    std::string lower = Lower( aValue );

    if( !EndsWith( lower, aFrom ) )
        return aValue;

    return aValue.substr( 0, aValue.size() - aFrom.size() ) + aTo;
}


std::string jsonUnescape( std::string aValue )
{
    std::string out;
    out.reserve( aValue.size() );
    bool escape = false;

    for( char ch : aValue )
    {
        if( escape )
        {
            switch( ch )
            {
            case 'n': out.push_back( '\n' ); break;
            case 'r': break;
            case 't': out.push_back( '\t' ); break;
            case '"': out.push_back( '"' ); break;
            case '\\': out.push_back( '\\' ); break;
            default: out.push_back( ch ); break;
            }

            escape = false;
        }
        else if( ch == '\\' )
        {
            escape = true;
        }
        else
        {
            out.push_back( ch );
        }
    }

    return out;
}


std::string jsonStringValue( const std::string& aJson, const std::string& aKey )
{
    std::string needle = "\"" + aKey + "\"";
    size_t pos = aJson.find( needle );

    if( pos == std::string::npos )
        return "";

    pos = aJson.find( ':', pos + needle.size() );

    if( pos == std::string::npos )
        return "";

    pos = aJson.find( '"', pos );

    if( pos == std::string::npos )
        return "";

    ++pos;
    std::string value;

    for( ; pos < aJson.size(); ++pos )
    {
        char ch = aJson[pos];

        if( ch == '"' )
            return jsonUnescape( value );

        value.push_back( ch );
    }

    return "";
}


std::vector<std::string> jsonStringArray( const std::string& aJson, const std::string& aKey )
{
    std::vector<std::string> values;
    std::string needle = "\"" + aKey + "\"";
    size_t pos = aJson.find( needle );

    if( pos == std::string::npos )
        return values;

    pos = aJson.find( '[', pos + needle.size() );

    if( pos == std::string::npos )
        return values;

    while( pos < aJson.size() )
    {
        pos = aJson.find( '"', pos );

        if( pos == std::string::npos )
            break;

        ++pos;
        std::string value;
        bool escape = false;

        for( ; pos < aJson.size(); ++pos )
        {
            char ch = aJson[pos];

            if( escape )
            {
                value.push_back( ch );
                escape = false;
            }
            else if( ch == '\\' )
            {
                escape = true;
            }
            else if( ch == '"' )
            {
                values.push_back( jsonUnescape( value ) );
                break;
            }
            else
            {
                value.push_back( ch );
            }
        }

        pos = aJson.find_first_of( ",]", pos );

        if( pos == std::string::npos || aJson[pos] == ']' )
            break;

        ++pos;
    }

    return values;
}


std::unique_ptr<SEXPR::NODE> listNode( const std::string& aHead )
{
    std::unique_ptr<SEXPR::NODE> node = SEXPR::NODE::MakeList();
    node->Children.push_back( SEXPR::NODE::MakeAtom( aHead ) );
    return node;
}


void appendAtom( SEXPR::NODE* aNode, const std::string& aValue, bool aQuoted = false )
{
    aNode->Children.push_back( SEXPR::NODE::MakeAtom( aValue, aQuoted ) );
}


void appendChild( SEXPR::NODE* aNode, std::unique_ptr<SEXPR::NODE> aChild )
{
    aNode->Children.push_back( std::move( aChild ) );
}


std::unique_ptr<SEXPR::NODE> atomList( const std::string& aHead, const std::string& aValue,
                                       bool aQuoted = false )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( aHead );
    appendAtom( node.get(), aValue, aQuoted );
    return node;
}


std::unique_ptr<SEXPR::NODE> atNode( const std::string& aX, const std::string& aY,
                                     const std::string& aAngle = "0" )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( "at" );
    appendAtom( node.get(), aX );
    appendAtom( node.get(), aY );
    appendAtom( node.get(), aAngle );
    return node;
}


std::unique_ptr<SEXPR::NODE> atNodeXY( const std::string& aX, const std::string& aY )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( "at" );
    appendAtom( node.get(), aX );
    appendAtom( node.get(), aY );
    return node;
}


std::unique_ptr<SEXPR::NODE> effectsNode( bool aHidden = false,
                                           const std::string& aSize = "1.27" )
{
    std::unique_ptr<SEXPR::NODE> effects = listNode( "effects" );
    std::unique_ptr<SEXPR::NODE> font = listNode( "font" );
    std::unique_ptr<SEXPR::NODE> size = listNode( "size" );
    appendAtom( size.get(), aSize );
    appendAtom( size.get(), aSize );
    appendChild( font.get(), std::move( size ) );
    appendChild( effects.get(), std::move( font ) );

    if( aHidden )
        appendAtom( effects.get(), "hide" );

    return effects;
}


std::unique_ptr<SEXPR::NODE> legacyPinEffectsNode( const std::string& aSize )
{
    std::string sizeValue = legacyCoordToMm( aSize.empty() ? "50" : aSize );
    std::unique_ptr<SEXPR::NODE> effects = listNode( "effects" );
    std::unique_ptr<SEXPR::NODE> font = listNode( "font" );
    std::unique_ptr<SEXPR::NODE> size = listNode( "size" );
    appendAtom( size.get(), sizeValue );
    appendAtom( size.get(), sizeValue );
    appendChild( font.get(), std::move( size ) );
    appendChild( effects.get(), std::move( font ) );
    return effects;
}


std::unique_ptr<SEXPR::NODE> propertyNode( const std::string& aName, const std::string& aValue,
                                           const std::string& aId, bool aHidden,
                                           bool aUseId = true )
{
    std::unique_ptr<SEXPR::NODE> prop = listNode( "property" );
    appendAtom( prop.get(), aName, true );
    appendAtom( prop.get(), aValue, true );
    appendChild( prop.get(), atNode( "0", "0" ) );

    if( aUseId )
        appendChild( prop.get(), atomList( "id", aId ) );

    appendChild( prop.get(), effectsNode( aHidden ) );
    return prop;
}


std::unique_ptr<SEXPR::NODE> schematicPropertyNode( const std::string& aName,
                                                    const std::string& aValue,
                                                    const std::string& aX,
                                                    const std::string& aY,
                                                    const std::string& aId,
                                                    bool aHidden,
                                                    const std::string& aAngle = "0",
                                                    bool aUseId = true,
                                                    const std::string& aLegacySize = "50" )
{
    std::unique_ptr<SEXPR::NODE> prop = listNode( "property" );
    appendAtom( prop.get(), aName, true );
    appendAtom( prop.get(), aValue, true );
    appendChild( prop.get(), atNode( aX, aY, aAngle ) );

    if( aUseId )
        appendChild( prop.get(), atomList( "id", aId ) );

    appendChild( prop.get(), effectsNode( aHidden, legacyCoordToMm( aLegacySize ) ) );
    return prop;
}


std::unique_ptr<SEXPR::NODE> xyNode( const std::string& aX, const std::string& aY )
{
    std::unique_ptr<SEXPR::NODE> xy = listNode( "xy" );
    appendAtom( xy.get(), aX );
    appendAtom( xy.get(), aY );
    return xy;
}


std::unique_ptr<SEXPR::NODE> strokeNode( const std::string& aWidth )
{
    std::unique_ptr<SEXPR::NODE> stroke = listNode( "stroke" );
    appendChild( stroke.get(), atomList( "width", legacyCoordToMm( aWidth ) ) );
    appendChild( stroke.get(), atomList( "type", "default" ) );
    return stroke;
}


std::unique_ptr<SEXPR::NODE> fillNode( const std::string& aLegacyFill )
{
    std::unique_ptr<SEXPR::NODE> fill = listNode( "fill" );
    std::string type = "none";

    if( aLegacyFill == "F" )
        type = "outline";
    else if( aLegacyFill == "f" )
        type = "background";

    appendChild( fill.get(), atomList( "type", type ) );
    return fill;
}


std::string strokeWidthLegacy( SEXPR::NODE* aNode )
{
    SEXPR::NODE* stroke = aNode ? aNode->ChildList( "stroke" ) : nullptr;
    SEXPR::NODE* widthNode = stroke ? stroke->ChildList( "width" ) : nullptr;
    std::string width = widthNode ? widthNode->AtomAt( 1 ) : "0";
    return std::to_string( mmToLegacyCoord( width ) );
}


std::string fillLegacy( SEXPR::NODE* aNode )
{
    SEXPR::NODE* fill = aNode ? aNode->ChildList( "fill" ) : nullptr;
    SEXPR::NODE* typeNode = fill ? fill->ChildList( "type" ) : nullptr;
    std::string type = typeNode ? typeNode->AtomAt( 1 ) : "";

    if( type == "outline" )
        return "F";

    if( type == "background" )
        return "f";

    return "N";
}


std::string legacyTextOrientationToAngle( const std::string& aOrientation )
{
    if( aOrientation == "1" ) return "90";
    if( aOrientation == "2" ) return "180";
    if( aOrientation == "3" ) return "270";
    return "0";
}


std::string legacyFieldOrientationToAngle( const std::string& aOrientation )
{
    return aOrientation == "V" ? "90" : "0";
}


std::string sexprAngleToLegacyTextOrientation( const std::string& aAngle )
{
    int angle = 0;

    if( IsNumber( aAngle ) )
        angle = std::stoi( aAngle ) % 360;

    if( angle < 0 )
        angle += 360;

    if( angle == 90 ) return "1";
    if( angle == 180 ) return "2";
    if( angle == 270 ) return "3";
    return "0";
}


SEXPR_SYMBOL_TRANSFORM sexprTransformFromLegacyMatrix( int aM11, int aM12, int aM21, int aM22 )
{
    if( aM11 == 0 && aM12 == -1 && aM21 == -1 && aM22 == 0 )
        return { "90", "" };

    if( aM11 == -1 && aM12 == 0 && aM21 == 0 && aM22 == 1 )
        return { "180", "" };

    if( aM11 == 0 && aM12 == 1 && aM21 == 1 && aM22 == 0 )
        return { "270", "" };

    if( aM11 == 1 && aM12 == 0 && aM21 == 0 && aM22 == 1 )
        return { "0", "x" };

    if( aM11 == 0 && aM12 == -1 && aM21 == 1 && aM22 == 0 )
        return { "90", "x" };

    if( aM11 == -1 && aM12 == 0 && aM21 == 0 && aM22 == -1 )
        return { "180", "x" };

    if( aM11 == 0 && aM12 == 1 && aM21 == -1 && aM22 == 0 )
        return { "270", "x" };

    return { "0", "" };
}


std::string legacyLabelShapeToSexpr( const std::string& aShape )
{
    std::string shape = Lower( aShape );

    if( shape == "input" ) return "input";
    if( shape == "output" ) return "output";
    if( shape == "bidi" || shape == "bidirectional" ) return "bidirectional";
    if( shape == "tristate" || shape == "tri_state" ) return "tri_state";
    return "passive";
}


bool legacyFieldHidden( const std::vector<std::string>& aWords, bool aDefaultHidden )
{
    if( aWords.size() <= 7 )
        return aDefaultHidden;

    return aWords[7] != "0000";
}


bool legacyLibraryFieldHidden( const std::vector<std::string>& aWords, bool aDefaultHidden )
{
    if( aWords.size() <= 6 )
        return aDefaultHidden;

    return aWords[6] == "I";
}


std::string legacySheetPinTypeToSexpr( const std::string& aType )
{
    std::string type = Lower( aType );

    if( type == "i" || type == "input" ) return "input";
    if( type == "o" || type == "output" ) return "output";
    if( type == "b" || type == "bidi" || type == "bidirectional" ) return "bidirectional";
    if( type == "t" || type == "tristate" || type == "tri_state" ) return "tri_state";
    return "passive";
}


std::string sexprSheetPinTypeToLegacy( const std::string& aType )
{
    if( aType == "input" ) return "I";
    if( aType == "output" ) return "O";
    if( aType == "bidirectional" ) return "B";
    if( aType == "tri_state" ) return "T";
    return "U";
}


std::string sexprLabelShapeToLegacy( const std::string& aShape )
{
    if( aShape == "input" ) return "Input";
    if( aShape == "output" ) return "Output";
    if( aShape == "bidirectional" ) return "BiDi";
    if( aShape == "tri_state" ) return "TriState";
    return "UnSpc";
}


std::unique_ptr<SEXPR::NODE> schematicAtItemNode( const std::string& aHead,
                                                  const std::string& aX,
                                                  const std::string& aY,
                                                  const std::string& aSeed )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( aHead );
    appendChild( node.get(), atNodeXY( legacyCoordToMm( aX ), legacyCoordToMm( aY ) ) );
    appendChild( node.get(), atomList( "uuid", deterministicUuid( aSeed ) ) );
    return node;
}


std::unique_ptr<SEXPR::NODE> schematicLineNode( const std::string& aHead,
                                                const std::vector<std::string>& aWords,
                                                const std::string& aSeed )
{
    if( aWords.size() < 4 )
        return nullptr;

    std::unique_ptr<SEXPR::NODE> node = listNode( aHead );
    std::unique_ptr<SEXPR::NODE> pts = listNode( "pts" );
    appendChild( pts.get(), xyNode( legacyCoordToMm( aWords[0] ),
                                    legacyCoordToMm( aWords[1] ) ) );
    appendChild( pts.get(), xyNode( legacyCoordToMm( aWords[2] ),
                                    legacyCoordToMm( aWords[3] ) ) );
    appendChild( node.get(), std::move( pts ) );
    appendChild( node.get(), atomList( "uuid", deterministicUuid( aSeed ) ) );
    return node;
}


std::unique_ptr<SEXPR::NODE> schematicBusEntryNode( const std::vector<std::string>& aWords,
                                                    const std::string& aSeed )
{
    if( aWords.size() < 4 )
        return nullptr;

    std::unique_ptr<SEXPR::NODE> node = listNode( "bus_entry" );
    appendChild( node.get(), atNodeXY( legacyCoordToMm( aWords[0] ),
                                       legacyCoordToMm( aWords[1] ) ) );

    int dx = std::stoi( aWords[2] ) - std::stoi( aWords[0] );
    int dy = std::stoi( aWords[3] ) - std::stoi( aWords[1] );
    std::unique_ptr<SEXPR::NODE> size = listNode( "size" );
    appendAtom( size.get(), legacyCoordToMm( std::to_string( dx ) ) );
    appendAtom( size.get(), legacyCoordToMm( std::to_string( dy ) ) );
    appendChild( node.get(), std::move( size ) );
    appendChild( node.get(), atomList( "uuid", deterministicUuid( aSeed ) ) );
    return node;
}


std::unique_ptr<SEXPR::NODE> schematicLabelNode( const std::string& aHead,
                                                 const std::string& aText,
                                                 const std::string& aX,
                                                 const std::string& aY,
                                                 const std::string& aOrientation,
                                                 const std::string& aShape,
                                                 const std::string& aSize,
                                                 const std::string& aSeed )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( aHead );
    appendAtom( node.get(), aText, true );

    if( aHead == "global_label" || aHead == "hierarchical_label" )
        appendChild( node.get(), atomList( "shape", legacyLabelShapeToSexpr( aShape ) ) );

    appendChild( node.get(), atNode( legacyCoordToMm( aX ), legacyCoordToMm( aY ),
                                     legacyTextOrientationToAngle( aOrientation ) ) );
    appendChild( node.get(), legacyPinEffectsNode( aSize ) );
    appendChild( node.get(), atomList( "uuid", deterministicUuid( aSeed ) ) );
    return node;
}


std::string sexprFormatted( std::unique_ptr<SEXPR::NODE> aRoot )
{
    return SEXPR::Format( aRoot.get() );
}


std::unique_ptr<SEXPR::NODE> legacyPinNode( const std::vector<std::string>& aWords );


int parseSchematicHeaderVersion( const std::string& aText )
{
    for( const std::string& line : linesOf( aText ) )
    {
        if( StartsWith( line, "EESchema Schematic File Version" ) )
        {
            std::vector<std::string> words = splitWords( line );

            if( !words.empty() && IsNumber( words.back() ) )
                return std::stoi( words.back() );
        }
    }

    return 4;
}


std::string parseLibraryHeaderVersion( const std::string& aText )
{
    for( const std::string& line : linesOf( aText ) )
    {
        if( StartsWith( line, "EESchema-LIBRARY Version" ) )
        {
            std::vector<std::string> words = splitWords( line );

            if( !words.empty() )
                return words.back();
        }
    }

    return "2.4";
}


LEGACY_PROJECT_META parseLegacyProjectMeta( const std::string& aText )
{
    LEGACY_PROJECT_META meta;

    for( const std::string& line : linesOf( aText ) )
    {
        std::string trimmed = Trim( line );

        if( trimmed.empty() || StartsWith( trimmed, "#" ) || StartsWith( trimmed, "[" ) )
            continue;

        size_t eq = trimmed.find( '=' );

        if( eq == std::string::npos )
            continue;

        std::string key = Trim( trimmed.substr( 0, eq ) );
        std::string value = Trim( trimmed.substr( eq + 1 ) );

        if( StartsWith( key, "LibName" ) )
        {
            if( !value.empty() )
                meta.Libraries.push_back( value );
            continue;
        }

        if( key == "LibDir" || key == "NetIExt" || key == "CmpExt"
            || key == "PageLayoutDescrFile" || key == "PlotDirectoryName"
            || key == "SubpartIdSeparator" || key == "SubpartFirstId"
            || key == "update" || key == "version" || key == "last_client" )
        {
            meta.Settings[key] = value;
        }
    }

    return meta;
}


LEGACY_SCHEMATIC_META parseLegacySchematicMeta( const std::string& aText )
{
    LEGACY_SCHEMATIC_META meta;

    for( const std::string& line : linesOf( aText ) )
    {
        std::vector<std::string> words = splitWords( line );

        if( words.empty() )
            continue;

        if( words[0] == "$Descr" && words.size() > 1 )
            meta.Paper = words[1];
        else if( StartsWith( line, "Title " ) )
            meta.Title = unquoteLegacy( line.substr( 6 ) );
        else if( StartsWith( line, "Date " ) )
            meta.Date = unquoteLegacy( line.substr( 5 ) );
        else if( StartsWith( line, "Rev " ) )
            meta.Rev = unquoteLegacy( line.substr( 4 ) );
        else if( StartsWith( line, "Comp " ) )
            meta.Company = unquoteLegacy( line.substr( 5 ) );
        else if( StartsWith( line, "Comment" ) && words.size() > 1 )
        {
            std::string number = words[0].substr( 7 );

            if( IsNumber( number ) )
            {
                int index = std::stoi( number );

                if( index >= 1 && index <= 9 )
                    meta.Comments[static_cast<size_t>( index - 1 )] =
                            unquoteLegacy( line.substr( words[0].size() + 1 ) );
            }
        }
    }

    return meta;
}


std::unique_ptr<SEXPR::NODE> titleBlockNode( const LEGACY_SCHEMATIC_META& aMeta )
{
    std::unique_ptr<SEXPR::NODE> title = listNode( "title_block" );

    if( !aMeta.Title.empty() )
        appendChild( title.get(), atomList( "title", aMeta.Title, true ) );

    if( !aMeta.Date.empty() )
        appendChild( title.get(), atomList( "date", aMeta.Date, true ) );

    if( !aMeta.Rev.empty() )
        appendChild( title.get(), atomList( "rev", aMeta.Rev, true ) );

    if( !aMeta.Company.empty() )
        appendChild( title.get(), atomList( "company", aMeta.Company, true ) );

    for( size_t i = 0; i < aMeta.Comments.size(); ++i )
    {
        if( aMeta.Comments[i].empty() )
            continue;

        std::unique_ptr<SEXPR::NODE> comment = listNode( "comment" );
        appendAtom( comment.get(), std::to_string( i + 1 ) );
        appendAtom( comment.get(), aMeta.Comments[i], true );
        appendChild( title.get(), std::move( comment ) );
    }

    return title;
}


std::string cacheNameForLibId( std::string aLibId )
{
    for( char& ch : aLibId )
    {
        if( ch == ':' )
            ch = '_';
    }

    return aLibId;
}


std::string symbolNameWithoutLibraryPrefix( const std::string& aName )
{
    std::string::size_type sep = aName.find( ':' );

    if( sep == std::string::npos || sep + 1 >= aName.size() )
        return aName;

    return aName.substr( sep + 1 );
}


std::string legacyLibraryReferencePrefix( const std::string& aReference,
                                          const std::string& aFallbackName );


std::map<std::string, LEGACY_SYMBOL_DEF> schematicCacheSymbolDefs(
        const DOCUMENT& aDocument, const std::vector<std::string>& aLines )
{
    std::vector<std::string> libraryNames;

    for( const std::string& line : aLines )
    {
        if( StartsWith( line, "LIBS:" ) )
        {
            std::string name = Trim( line.substr( 5 ) );

            if( !name.empty() )
                libraryNames.push_back( name );
        }
    }

    std::string fallback = aDocument.Path.stem().string() + "-cache";

    if( std::find( libraryNames.begin(), libraryNames.end(), fallback ) == libraryNames.end() )
        libraryNames.push_back( fallback );

    std::map<std::string, LEGACY_SYMBOL_DEF> defs;

    for( const std::string& libraryName : libraryNames )
    {
        std::filesystem::path libPath = aDocument.Path.parent_path() / ( libraryName + ".lib" );

        if( !std::filesystem::exists( libPath ) )
            continue;

        std::string currentName;
        LEGACY_SYMBOL_DEF current;
        std::vector<std::string> aliases;

        auto flush = [&]()
        {
            if( currentName.empty() )
                return;

            defs[currentName] = current;

            for( const std::string& alias : aliases )
                defs[alias] = current;

            currentName.clear();
            current = LEGACY_SYMBOL_DEF();
            aliases.clear();
        };

        for( const std::string& line : linesOf( ReadTextFile( libPath ) ) )
        {
            std::vector<std::string> words = splitWords( line );

            if( words.empty() )
                continue;

            if( words[0] == "DEF" && words.size() > 1 )
            {
                flush();
                currentName = words[1];
                current.Reference = words.size() > 2
                        ? legacyLibraryReferencePrefix( words[2], currentName )
                        : legacyLibraryReferencePrefix( "", currentName );
                current.ShowPinNumbers = words.size() > 5 ? words[5] != "N" : true;
                current.ShowPinNames = words.size() > 6 ? words[6] != "N" : true;
                current.UnitCount = legacyIntAtOrDefault( words, 7, 1 );
            }
            else if( words[0] == "ALIAS" )
            {
                for( size_t i = 1; i < words.size(); ++i )
                    aliases.push_back( words[i] );
            }
            else if( words[0] == "X" )
            {
                current.PinWords.push_back( { legacyPinUnit( words ), words } );
            }
            else if( words[0] == "S" || words[0] == "P" || words[0] == "C"
                     || words[0] == "A" || words[0] == "T" )
            {
                current.DrawLines.push_back( { legacyDrawUnit( words ), line } );
            }
            else if( words[0] == "ENDDEF" )
            {
                flush();
            }
        }

        flush();
    }

    return defs;
}


std::map<std::string, std::string> schematicCacheSymbolLibraries(
        const DOCUMENT& aDocument, const std::vector<std::string>& aLines )
{
    std::vector<std::string> libraryNames;

    for( const std::string& line : aLines )
    {
        if( StartsWith( line, "LIBS:" ) )
        {
            std::string name = Trim( line.substr( 5 ) );

            if( !name.empty() )
                libraryNames.push_back( name );
        }
    }

    std::string fallback = aDocument.Path.stem().string() + "-cache";

    if( std::find( libraryNames.begin(), libraryNames.end(), fallback ) == libraryNames.end() )
        libraryNames.push_back( fallback );

    std::map<std::string, std::string> libraries;

    for( const std::string& libraryName : libraryNames )
    {
        std::filesystem::path libPath = aDocument.Path.parent_path() / ( libraryName + ".lib" );

        if( !std::filesystem::exists( libPath ) )
            continue;

        std::string currentName;
        std::vector<std::string> aliases;

        auto flush = [&]()
        {
            if( currentName.empty() )
                return;

            libraries[currentName] = libraryName;

            for( const std::string& alias : aliases )
                libraries[alias] = libraryName;

            currentName.clear();
            aliases.clear();
        };

        for( const std::string& line : linesOf( ReadTextFile( libPath ) ) )
        {
            std::vector<std::string> words = splitWords( line );

            if( words.empty() )
                continue;

            if( words[0] == "DEF" && words.size() > 1 )
            {
                flush();
                currentName = words[1];
            }
            else if( words[0] == "ALIAS" )
            {
                for( size_t i = 1; i < words.size(); ++i )
                    aliases.push_back( words[i] );
            }
            else if( words[0] == "ENDDEF" )
            {
                flush();
            }
        }

        flush();
    }

    return libraries;
}


std::string valueFromLibId( const std::string& aLibId )
{
    size_t pos = aLibId.find( ':' );
    return pos == std::string::npos ? aLibId : aLibId.substr( pos + 1 );
}


std::string normalizeLegacyHiddenReference( const std::string& aReference,
                                            const std::string& aValue,
                                            const LEGACY_SYMBOL_DEF* aDef )
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

    if( aDef && aDef->Reference.size() >= 2 && aDef->Reference[0] == '#' )
        return aDef->Reference + suffix;

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


bool isLegacyUnannotatedReference( const std::string& aReference )
{
    return aReference.find( '?' ) != std::string::npos;
}


std::string legacyLibraryReferencePrefix( const std::string& aReference,
                                          const std::string& aFallbackName )
{
    std::string reference = Trim( aReference );
    std::string prefix;

    if( !reference.empty() && reference[0] == '#' )
    {
        prefix = "#";

        for( size_t i = 1; i < reference.size(); ++i )
        {
            unsigned char ch = static_cast<unsigned char>( reference[i] );

            if( std::isalpha( ch ) || reference[i] == '_' )
                prefix.push_back( reference[i] );
            else
                break;
        }

        if( prefix.size() > 1 )
            return prefix;
    }
    else
    {
        for( char ch : reference )
        {
            unsigned char uch = static_cast<unsigned char>( ch );

            if( std::isalpha( uch ) )
                prefix.push_back( ch );
            else
                break;
        }

        if( !prefix.empty() )
            return prefix;
    }

    for( char ch : aFallbackName )
    {
        if( ch >= 'A' && ch <= 'Z' )
            return std::string( 1, ch );
    }

    return "U";
}


void appendPinVisibilityNodes( SEXPR::NODE* aSymbol, bool aShowPinNumbers, bool aShowPinNames );


bool appendLegacyDrawItemNode( SEXPR::NODE* aSymbol, const std::string& aLine )
{
    if( !aSymbol )
        return false;

    std::vector<std::string> itemWords = splitWords( aLine );

    if( itemWords.empty() )
        return false;

    if( itemWords[0] == "S" && itemWords.size() >= 9 )
    {
        std::unique_ptr<SEXPR::NODE> rect = listNode( "rectangle" );
        std::unique_ptr<SEXPR::NODE> start = listNode( "start" );
        appendAtom( start.get(), legacyCoordToMm( itemWords[1] ) );
        appendAtom( start.get(), legacyCoordToMm( itemWords[2] ) );
        appendChild( rect.get(), std::move( start ) );

        std::unique_ptr<SEXPR::NODE> end = listNode( "end" );
        appendAtom( end.get(), legacyCoordToMm( itemWords[3] ) );
        appendAtom( end.get(), legacyCoordToMm( itemWords[4] ) );
        appendChild( rect.get(), std::move( end ) );
        appendChild( rect.get(), strokeNode( itemWords[7] ) );
        appendChild( rect.get(), fillNode( itemWords[8] ) );
        appendChild( aSymbol, std::move( rect ) );
        return true;
    }

    if( itemWords[0] == "P" && itemWords.size() >= 7 && IsNumber( itemWords[1] ) )
    {
        int pointCount = std::stoi( itemWords[1] );
        size_t firstCoord = 5;

        if( itemWords.size() < firstCoord + static_cast<size_t>( pointCount ) * 2 )
            return false;

        std::unique_ptr<SEXPR::NODE> poly = listNode( "polyline" );
        std::unique_ptr<SEXPR::NODE> pts = listNode( "pts" );

        for( int p = 0; p < pointCount; ++p )
        {
            size_t coord = firstCoord + static_cast<size_t>( p ) * 2;
            appendChild( pts.get(), xyNode( legacyCoordToMm( itemWords[coord] ),
                                            legacyCoordToMm( itemWords[coord + 1] ) ) );
        }

        appendChild( poly.get(), std::move( pts ) );
        appendChild( poly.get(), strokeNode( itemWords[4] ) );
        appendChild( poly.get(), fillNode( itemWords.back() ) );
        appendChild( aSymbol, std::move( poly ) );
        return true;
    }

    if( itemWords[0] == "C" && itemWords.size() >= 8 )
    {
        std::unique_ptr<SEXPR::NODE> circle = listNode( "circle" );
        std::unique_ptr<SEXPR::NODE> center = listNode( "center" );
        appendAtom( center.get(), legacyCoordToMm( itemWords[1] ) );
        appendAtom( center.get(), legacyCoordToMm( itemWords[2] ) );
        appendChild( circle.get(), std::move( center ) );
        appendChild( circle.get(), atomList( "radius", legacyCoordToMm( itemWords[3] ) ) );
        appendChild( circle.get(), strokeNode( itemWords[6] ) );
        appendChild( circle.get(), fillNode( itemWords[7] ) );
        appendChild( aSymbol, std::move( circle ) );
        return true;
    }

    if( itemWords[0] == "A" && itemWords.size() >= 13 )
    {
        std::unique_ptr<SEXPR::NODE> arc = listNode( "arc" );
        std::unique_ptr<SEXPR::NODE> start = listNode( "start" );
        appendAtom( start.get(), legacyCoordToMm( itemWords[9] ) );
        appendAtom( start.get(), legacyCoordToMm( itemWords[10] ) );
        appendChild( arc.get(), std::move( start ) );

        std::unique_ptr<SEXPR::NODE> mid = listNode( "mid" );
        appendAtom( mid.get(), legacyCoordToMm( itemWords[1] ) );
        appendAtom( mid.get(), legacyCoordToMm( itemWords[2] ) );
        appendChild( arc.get(), std::move( mid ) );

        std::unique_ptr<SEXPR::NODE> end = listNode( "end" );
        appendAtom( end.get(), legacyCoordToMm( itemWords[11] ) );
        appendAtom( end.get(), legacyCoordToMm( itemWords[12] ) );
        appendChild( arc.get(), std::move( end ) );
        appendChild( arc.get(), strokeNode( itemWords[7] ) );
        appendChild( arc.get(), fillNode( itemWords[8] ) );
        appendChild( aSymbol, std::move( arc ) );
        return true;
    }

    if( itemWords[0] == "T" && itemWords.size() >= 8 )
    {
        std::string text = firstQuotedValue( aLine );

        if( text.empty() && itemWords.size() > 7 )
            text = itemWords[7];

        std::unique_ptr<SEXPR::NODE> textNode = listNode( "text" );
        appendAtom( textNode.get(), text, true );
        appendChild( textNode.get(), atNode( legacyCoordToMm( itemWords[2] ),
                                             legacyCoordToMm( itemWords[3] ),
                                             itemWords[1] ) );
        appendChild( textNode.get(), effectsNode() );
        appendChild( aSymbol, std::move( textNode ) );
        return true;
    }

    return false;
}


bool legacySymbolHasMultipleUnits( const LEGACY_SYMBOL_DEF& aDef )
{
    if( aDef.UnitCount > 1 )
        return true;

    for( const auto& pin : aDef.PinWords )
    {
        if( pin.first > 1 )
            return true;
    }

    for( const auto& draw : aDef.DrawLines )
    {
        if( draw.first > 1 )
            return true;
    }

    return false;
}


bool legacyItemAppliesToUnit( int aItemUnit, int aUnit )
{
    return aItemUnit == aUnit;
}


void appendLegacySymbolItemsForUnit( SEXPR::NODE* aSymbol, const LEGACY_SYMBOL_DEF& aDef,
                                     int aUnit, bool aIncludeCommon = false )
{
    for( const auto& drawLine : aDef.DrawLines )
    {
        if( legacyItemAppliesToUnit( drawLine.first, aUnit )
            || ( aIncludeCommon && drawLine.first == 0 ) )
        {
            appendLegacyDrawItemNode( aSymbol, drawLine.second );
        }
    }

    for( const auto& pinWords : aDef.PinWords )
    {
        if( !legacyItemAppliesToUnit( pinWords.first, aUnit )
            && !( aIncludeCommon && pinWords.first == 0 ) )
        {
            continue;
        }

        std::unique_ptr<SEXPR::NODE> pin = legacyPinNode( pinWords.second );

        if( pin )
            appendChild( aSymbol, std::move( pin ) );
    }
}


void appendLegacyUnitSubSymbols( SEXPR::NODE* aSymbol, const std::string& aName,
                                 const LEGACY_SYMBOL_DEF& aDef )
{
    int maxUnit = std::max( 1, aDef.UnitCount );

    for( const auto& pin : aDef.PinWords )
        maxUnit = std::max( maxUnit, pin.first );

    for( const auto& draw : aDef.DrawLines )
        maxUnit = std::max( maxUnit, draw.first );

    for( int unit = 0; unit <= maxUnit; ++unit )
    {
        bool hasItems = false;

        for( const auto& draw : aDef.DrawLines )
            hasItems = hasItems || legacyItemAppliesToUnit( draw.first, unit );

        for( const auto& pin : aDef.PinWords )
            hasItems = hasItems || legacyItemAppliesToUnit( pin.first, unit );

        if( !hasItems )
            continue;

        std::unique_ptr<SEXPR::NODE> sub = listNode( "symbol" );
        appendAtom( sub.get(), aName + "_" + std::to_string( unit ) + "_1", true );
        appendLegacySymbolItemsForUnit( sub.get(), aDef, unit );
        appendChild( aSymbol, std::move( sub ) );
    }
}


void appendAliasUnitSubSymbols( SEXPR::NODE* aAliasSymbol, const std::string& aAlias,
                                const std::string& aParent, const LEGACY_SYMBOL_DEF& aDef )
{
    int maxUnit = std::max( 1, aDef.UnitCount );

    for( const auto& pin : aDef.PinWords )
        maxUnit = std::max( maxUnit, pin.first );

    for( const auto& draw : aDef.DrawLines )
        maxUnit = std::max( maxUnit, draw.first );

    for( int unit = 0; unit <= maxUnit; ++unit )
    {
        bool hasItems = false;

        for( const auto& draw : aDef.DrawLines )
            hasItems = hasItems || legacyItemAppliesToUnit( draw.first, unit );

        for( const auto& pin : aDef.PinWords )
            hasItems = hasItems || legacyItemAppliesToUnit( pin.first, unit );

        if( !hasItems )
            continue;

        std::unique_ptr<SEXPR::NODE> sub = listNode( "symbol" );
        appendAtom( sub.get(), aAlias + "_" + std::to_string( unit ) + "_1", true );
        appendChild( sub.get(), atomList( "extends",
                                          aParent + "_" + std::to_string( unit ) + "_1",
                                          true ) );
        appendChild( aAliasSymbol, std::move( sub ) );
    }
}


void appendEmbeddedLibSymbol( SEXPR::NODE* aLibSymbols, const std::string& aLibId,
                              const LEGACY_SYMBOL_DEF* aDef )
{
    if( !aLibSymbols )
        return;

    std::string symbolName = symbolNameWithoutLibraryPrefix( aLibId );

    std::unique_ptr<SEXPR::NODE> symbol = listNode( "symbol" );
    appendAtom( symbol.get(), aLibId, true );
    appendPinVisibilityNodes( symbol.get(), aDef ? aDef->ShowPinNumbers : true,
                              aDef ? aDef->ShowPinNames : true );
    appendChild( symbol.get(), atomList( "in_bom", "yes" ) );
    appendChild( symbol.get(), atomList( "on_board", "yes" ) );

    std::string reference = aDef && !aDef->Reference.empty()
            ? legacyLibraryReferencePrefix( aDef->Reference, symbolName )
            : legacyLibraryReferencePrefix( "", symbolName );
    appendChild( symbol.get(), propertyNode( "Reference", reference, "0", false, false ) );
    appendChild( symbol.get(), propertyNode( "Value", valueFromLibId( aLibId ), "1", false, false ) );
    appendChild( symbol.get(), propertyNode( "Footprint", "", "2", true, false ) );
    appendChild( symbol.get(), propertyNode( "Datasheet", "", "3", true, false ) );

    if( aDef )
    {
        if( legacySymbolHasMultipleUnits( *aDef ) )
            appendLegacyUnitSubSymbols( symbol.get(), symbolName, *aDef );
        else
            appendLegacySymbolItemsForUnit( symbol.get(), *aDef, 1, true );
    }

    appendChild( aLibSymbols, std::move( symbol ) );
}


void appendPinVisibilityNodes( SEXPR::NODE* aSymbol, bool aShowPinNumbers, bool aShowPinNames )
{
    if( !aSymbol )
        return;

    if( !aShowPinNumbers )
    {
        std::unique_ptr<SEXPR::NODE> pinNumbers = listNode( "pin_numbers" );
        appendAtom( pinNumbers.get(), "hide" );
        appendChild( aSymbol, std::move( pinNumbers ) );
    }

    if( !aShowPinNames )
    {
        std::unique_ptr<SEXPR::NODE> pinNames = listNode( "pin_names" );
        appendAtom( pinNames.get(), "hide" );
        appendChild( aSymbol, std::move( pinNames ) );
    }
}


void appendSchematicSymbolPins( SEXPR::NODE* aSymbol, const LEGACY_SYMBOL_DEF* aDef,
                                const std::string& aSeed )
{
    if( !aSymbol || !aDef )
        return;

    for( const auto& pinEntry : aDef->PinWords )
    {
        const std::vector<std::string>& pinWords = pinEntry.second;

        if( pinWords.size() < 3 )
            continue;

        std::unique_ptr<SEXPR::NODE> pin = listNode( "pin" );
        appendAtom( pin.get(), pinWords[2], true );
        appendChild( pin.get(), atomList( "uuid",
                                          deterministicUuid( aSeed + ":pin:" + pinWords[2] ) ) );
        appendChild( aSymbol, std::move( pin ) );
    }
}


std::string legacySchematicToSexpr( const DOCUMENT& aDocument,
                                    const std::string& aTargetVersion,
                                    std::vector<std::string>* aWarnings )
{
    LEGACY_SCHEMATIC_META meta = parseLegacySchematicMeta( aDocument.RawText );
    std::vector<std::string> lines = linesOf( aDocument.RawText );
    bool usePropertyIds = !IsNumber( aTargetVersion ) || std::stoi( aTargetVersion ) < 20230121;
    std::map<std::string, LEGACY_SYMBOL_DEF> cacheDefs = schematicCacheSymbolDefs( aDocument, lines );
    std::map<std::string, std::string> cacheLibraries = schematicCacheSymbolLibraries( aDocument,
                                                                                       lines );

    std::unique_ptr<SEXPR::NODE> root = listNode( "kicad_sch" );
    appendChild( root.get(), atomList( "version", aTargetVersion ) );
    appendChild( root.get(), atomList( "generator", "kicad-backport" ) );
    appendChild( root.get(), atomList( "uuid", deterministicUuid( aDocument.Path.string()
                                                                   + ":schematic-root" ) ) );
    appendChild( root.get(), atomList( "paper", meta.Paper, true ) );

    std::unique_ptr<SEXPR::NODE> title = titleBlockNode( meta );

    if( title->Children.size() > 1 )
        appendChild( root.get(), std::move( title ) );

    std::unique_ptr<SEXPR::NODE> sheetInstances = listNode( "sheet_instances" );
    std::unique_ptr<SEXPR::NODE> rootPath = listNode( "path" );
    appendAtom( rootPath.get(), "/", true );
    appendChild( rootPath.get(), atomList( "page", "1", true ) );
    appendChild( sheetInstances.get(), std::move( rootPath ) );

    std::unique_ptr<SEXPR::NODE> symbolInstances = listNode( "symbol_instances" );
    int convertedSymbols = 0;
    int convertedWires = 0;
    int convertedSheets = 0;
    int convertedLabels = 0;
    int convertedJunctions = 0;
    int convertedNoConnects = 0;
    int convertedBuses = 0;
    int convertedBusEntries = 0;
    int convertedSheetPins = 0;
    int nextSheetPage = 2;

    for( size_t i = 0; i < lines.size(); ++i )
    {
        const std::string& line = lines[i];

        if( line == "$Sheet" )
        {
            std::ostringstream block;
            std::string x = "0";
            std::string y = "0";
            std::string w = "20.32";
            std::string h = "12.7";
            std::string tstamp;
            std::string sheetName = "Sheet";
            std::string sheetFile;
            std::string sheetNameSize = "50";
            std::string sheetFileSize = "50";
            std::vector<std::unique_ptr<SEXPR::NODE>> sheetPins;

            while( i + 1 < lines.size() )
            {
                const std::string& item = lines[++i];
                block << item << "\n";

                if( item == "$EndSheet" )
                    break;

                std::vector<std::string> words = splitWords( item );

                if( words.empty() )
                    continue;

                if( words[0] == "S" && words.size() > 4 )
                {
                    x = legacyCoordToMm( words[1] );
                    y = legacyCoordToMm( words[2] );
                    w = legacyCoordToMm( words[3] );
                    h = legacyCoordToMm( words[4] );
                }
                else if( words[0] == "U" && words.size() > 1 )
                {
                    tstamp = words[1];
                }
                else if( words[0] == "F0" )
                {
                    sheetName = firstQuotedValue( item );

                    if( words.size() > 2 )
                        sheetNameSize = words[2];
                }
                else if( words[0] == "F1" )
                {
                    sheetFile = replaceTrailingExtension( firstQuotedValue( item ), ".sch",
                                                          ".kicad_sch" );

                    if( words.size() > 2 )
                        sheetFileSize = words[2];
                }
                else if( words[0].size() > 1 && words[0][0] == 'F'
                         && IsNumber( words[0].substr( 1 ) )
                         && std::stoi( words[0].substr( 1 ) ) >= 2 )
                {
                    std::string pinName = firstQuotedValue( item );
                    std::vector<std::string> pinWords = wordsAfterFirstQuotedValue( item );

                    if( pinWords.size() >= 5 )
                    {
                        std::unique_ptr<SEXPR::NODE> pin = listNode( "pin" );
                        appendAtom( pin.get(), pinName, true );
                        appendChild( pin.get(), atomList( "type",
                                                          legacySheetPinTypeToSexpr( pinWords[0] ) ) );
                        appendChild( pin.get(), atNode( legacyCoordToMm( pinWords[2] ),
                                                        legacyCoordToMm( pinWords[3] ),
                                                        legacyTextOrientationToAngle( pinWords[1] ) ) );
                        appendChild( pin.get(), legacyPinEffectsNode( pinWords[4] ) );
                        appendChild( pin.get(), atomList( "uuid", deterministicUuid(
                                aDocument.Path.string() + ":sheet-pin:" + item ) ) );
                        sheetPins.push_back( std::move( pin ) );
                        ++convertedSheetPins;
                    }
                }
            }

            std::string uuid = !tstamp.empty()
                    ? legacyUuidFromTstamp( tstamp )
                    : deterministicUuid( aDocument.Path.string() + ":sheet:" + block.str() );
            std::unique_ptr<SEXPR::NODE> sheet = listNode( "sheet" );
            appendChild( sheet.get(), atNodeXY( x, y ) );

            std::unique_ptr<SEXPR::NODE> size = listNode( "size" );
            appendAtom( size.get(), w );
            appendAtom( size.get(), h );
            appendChild( sheet.get(), std::move( size ) );
            appendChild( sheet.get(), atomList( "uuid", uuid ) );
            appendChild( sheet.get(), schematicPropertyNode( "Sheet name", sheetName, x, y, "0",
                                                             false, "0", usePropertyIds,
                                                             sheetNameSize ) );
            appendChild( sheet.get(), schematicPropertyNode( "Sheet file", sheetFile, x, y, "1",
                                                             false, "0", usePropertyIds,
                                                             sheetFileSize ) );

            for( std::unique_ptr<SEXPR::NODE>& pin : sheetPins )
                appendChild( sheet.get(), std::move( pin ) );

            appendChild( root.get(), std::move( sheet ) );

            std::unique_ptr<SEXPR::NODE> path = listNode( "path" );
            appendAtom( path.get(), "/" + uuid, true );
            appendChild( path.get(), atomList( "page", std::to_string( nextSheetPage++ ), true ) );
            appendChild( sheetInstances.get(), std::move( path ) );
            ++convertedSheets;
        }
        else if( line == "$Comp" )
        {
            std::ostringstream block;
            std::string libId;
            std::string reference;
            std::string unit = "1";
            std::string value;
            std::string footprint;
            std::string datasheet;
            bool referenceHidden = false;
            bool valueHidden = false;
            bool footprintHidden = true;
            bool datasheetHidden = true;
            std::string x = "0";
            std::string y = "0";
            std::string tstamp;
            SEXPR_SYMBOL_TRANSFORM transform;
            struct FIELD_STATE
            {
                std::string X = "0";
                std::string Y = "0";
                std::string Angle = "0";
                std::string Size = "50";
            };
            FIELD_STATE referenceField;
            FIELD_STATE valueField;
            FIELD_STATE footprintField;
            FIELD_STATE datasheetField;
            struct CUSTOM_FIELD
            {
                std::string Name;
                std::string Value;
                std::string Id;
                bool Hidden = false;
                FIELD_STATE State;
            };
            std::vector<CUSTOM_FIELD> customFields;
            struct AR_INSTANCE
            {
                std::string Path;
                std::string Reference;
                std::string Part;
            };
            std::vector<AR_INSTANCE> arInstances;

            while( i + 1 < lines.size() )
            {
                const std::string& item = lines[++i];
                block << item << "\n";

                if( item == "$EndComp" )
                    break;

                std::vector<std::string> words = splitWords( item );

                if( words.empty() )
                    continue;

                if( words[0] == "L" && words.size() > 2 )
                {
                    libId = words[1];
                    reference = words[2];
                }
                else if( words[0] == "U" && words.size() > 1 )
                {
                    unit = words[1];

                    if( words.size() > 3 )
                        tstamp = words[3];
                }
                else if( words[0] == "P" && words.size() > 2 )
                {
                    x = legacyCoordToMm( words[1] );
                    y = legacyCoordToMm( words[2] );
                }
                else if( words[0] == "F" && words.size() > 1 )
                {
                    std::string fieldValue = firstQuotedValue( item );
                    std::vector<std::string> fieldWords = wordsAfterFirstQuotedValue( item );

                    auto parseFieldState = [&]() -> FIELD_STATE
                    {
                        FIELD_STATE field;

                        if( fieldWords.size() >= 3 )
                        {
                            field.Angle = legacyFieldOrientationToAngle( fieldWords[0] );
                            field.X = legacyCoordToMm( fieldWords[1] );
                            field.Y = legacyCoordToMm( fieldWords[2] );
                        }

                        if( fieldWords.size() >= 4 )
                            field.Size = fieldWords[3];

                        return field;
                    };

                    if( words[1] == "0" )
                    {
                        if( !fieldValue.empty()
                            && ( reference.empty() || isLegacyUnannotatedReference( reference )
                                 || !isLegacyUnannotatedReference( fieldValue ) ) )
                        {
                            reference = fieldValue;
                        }

                        referenceField = parseFieldState();
                        referenceHidden = legacyFieldHidden( words, false );
                    }
                    else if( words[1] == "1" )
                    {
                        value = fieldValue;
                        valueField = parseFieldState();
                        valueHidden = legacyFieldHidden( words, false );
                    }
                    else if( words[1] == "2" )
                    {
                        footprint = fieldValue;
                        footprintField = parseFieldState();
                        footprintHidden = legacyFieldHidden( words, true );
                    }
                    else if( words[1] == "3" )
                    {
                        datasheet = fieldValue;
                        datasheetField = parseFieldState();
                        datasheetHidden = legacyFieldHidden( words, true );
                    }
                    else if( IsNumber( words[1] ) && std::stoi( words[1] ) >= 4 )
                    {
                        std::vector<std::string> values = quotedValues( item );
                        CUSTOM_FIELD field;
                        field.Name = values.size() > 1 && !values.back().empty()
                                ? values.back() : "Field" + words[1];
                        field.Value = !values.empty() ? values.front() : fieldValue;
                        field.Id = words[1];
                        field.Hidden = legacyFieldHidden( words, false );
                        field.State = parseFieldState();
                        customFields.push_back( field );
                    }
                }
                else if( words[0] == "AR" )
                {
                    AR_INSTANCE instance;
                    instance.Path = quotedAttributeValue( item, "Path" );
                    instance.Reference = quotedAttributeValue( item, "Ref" );
                    instance.Part = quotedAttributeValue( item, "Part" );

                    if( !instance.Path.empty() )
                        arInstances.push_back( instance );
                }
                else if( words.size() == 4 )
                {
                    int m11 = 0;
                    int m12 = 0;
                    int m21 = 0;
                    int m22 = 0;

                    if( parseIntStrict( words[0], m11 ) && parseIntStrict( words[1], m12 )
                        && parseIntStrict( words[2], m21 ) && parseIntStrict( words[3], m22 ) )
                    {
                        transform = sexprTransformFromLegacyMatrix( m11, m12, m21, m22 );
                    }
                }
            }

            if( libId.empty() )
                continue;

            if( value.empty() )
                value = libId;

            std::string uuid = !tstamp.empty()
                    ? legacyUuidFromTstamp( tstamp )
                    : deterministicUuid( aDocument.Path.string() + ":comp:" + block.str() );
            std::string cacheName = cacheNameForLibId( libId );
            auto defIt = cacheDefs.find( cacheName );
            const LEGACY_SYMBOL_DEF* symbolDef = defIt == cacheDefs.end() ? nullptr : &defIt->second;
            auto libraryIt = cacheLibraries.find( cacheName );
            bool explicitLibrary = libId.find( ':' ) != std::string::npos;
            std::string outputLibId = explicitLibrary || libraryIt == cacheLibraries.end()
                    ? libId : libraryIt->second + ":" + cacheName;

            if( reference.empty() || isLegacyUnannotatedReference( reference ) )
            {
                for( const AR_INSTANCE& ar : arInstances )
                {
                    if( !ar.Reference.empty() && !isLegacyUnannotatedReference( ar.Reference ) )
                    {
                        reference = ar.Reference;
                        break;
                    }
                }
            }

            reference = normalizeLegacyHiddenReference( reference, value, symbolDef );

            std::unique_ptr<SEXPR::NODE> symbol = listNode( "symbol" );
            appendChild( symbol.get(), atomList( "lib_id", outputLibId, true ) );
            appendChild( symbol.get(), atNode( x, y, transform.Angle ) );

            if( !transform.Mirror.empty() )
                appendChild( symbol.get(), atomList( "mirror", transform.Mirror ) );

            appendChild( symbol.get(), atomList( "unit", unit ) );
            appendChild( symbol.get(), atomList( "in_bom", "yes" ) );
            appendChild( symbol.get(), atomList( "on_board", "yes" ) );
            appendChild( symbol.get(), atomList( "uuid", uuid ) );
            appendChild( symbol.get(), schematicPropertyNode( "Reference", reference,
                    referenceField.X, referenceField.Y, "0", referenceHidden,
                    referenceField.Angle, usePropertyIds, referenceField.Size ) );
            appendChild( symbol.get(), schematicPropertyNode( "Value", value,
                    valueField.X, valueField.Y, "1", valueHidden,
                    valueField.Angle, usePropertyIds, valueField.Size ) );
            appendChild( symbol.get(), schematicPropertyNode( "Footprint", footprint,
                    footprintField.X, footprintField.Y, "2", footprintHidden,
                    footprintField.Angle, usePropertyIds, footprintField.Size ) );
            appendChild( symbol.get(), schematicPropertyNode( "Datasheet", datasheet,
                    datasheetField.X, datasheetField.Y, "3", datasheetHidden,
                    datasheetField.Angle, usePropertyIds, datasheetField.Size ) );

            for( const CUSTOM_FIELD& field : customFields )
            {
                appendChild( symbol.get(), schematicPropertyNode( field.Name, field.Value,
                        field.State.X, field.State.Y, field.Id, field.Hidden,
                        field.State.Angle, usePropertyIds, field.State.Size ) );
            }

            appendSchematicSymbolPins( symbol.get(), symbolDef,
                                       aDocument.Path.string() + ":comp:" + block.str() );
            appendChild( root.get(), std::move( symbol ) );

            std::unique_ptr<SEXPR::NODE> instance = listNode( "path" );
            appendAtom( instance.get(), "/" + uuid, true );
            appendChild( instance.get(), atomList( "reference", reference, true ) );
            appendChild( instance.get(), atomList( "unit", unit ) );
            appendChild( instance.get(), atomList( "value", value, true ) );
            appendChild( instance.get(), atomList( "footprint", footprint, true ) );
            appendChild( symbolInstances.get(), std::move( instance ) );

            for( const AR_INSTANCE& ar : arInstances )
            {
                std::string arPath = legacyInstancePathFromARPath( ar.Path );

                if( arPath.empty() )
                    continue;

                std::unique_ptr<SEXPR::NODE> arNode = listNode( "path" );
                appendAtom( arNode.get(), arPath, true );
                appendChild( arNode.get(), atomList( "reference",
                                                     ar.Reference.empty() ? reference : ar.Reference,
                                                     true ) );
                appendChild( arNode.get(), atomList( "unit",
                                                     ar.Part.empty() ? unit : ar.Part ) );
                appendChild( arNode.get(), atomList( "value", value, true ) );
                appendChild( arNode.get(), atomList( "footprint", footprint, true ) );
                appendChild( symbolInstances.get(), std::move( arNode ) );
            }

            ++convertedSymbols;
        }
        else if( ( line == "Wire Wire Line" || line == "Wire Bus Line" )
                 && i + 1 < lines.size() )
        {
            std::vector<std::string> words = splitWords( lines[++i] );
            std::string head = line == "Wire Bus Line" ? "bus" : "wire";
            std::unique_ptr<SEXPR::NODE> node = schematicLineNode( head, words,
                    aDocument.Path.string() + ":" + head + ":" + lines[i] );

            if( node )
            {
                appendChild( root.get(), std::move( node ) );

                if( head == "bus" )
                    ++convertedBuses;
                else
                    ++convertedWires;
            }
        }
        else if( StartsWith( line, "Entry " ) && i + 1 < lines.size() )
        {
            std::vector<std::string> words = splitWords( lines[++i] );
            std::unique_ptr<SEXPR::NODE> entry = schematicBusEntryNode( words,
                    aDocument.Path.string() + ":entry:" + line + ":" + lines[i] );

            if( entry )
            {
                appendChild( root.get(), std::move( entry ) );
                ++convertedBusEntries;
            }
        }
        else if( StartsWith( line, "Connection " ) )
        {
            std::vector<std::string> words = splitWords( line );

            if( words.size() > 3 )
            {
                appendChild( root.get(), schematicAtItemNode( "junction", words[2], words[3],
                                aDocument.Path.string() + ":junction:" + line ) );
                ++convertedJunctions;
            }
        }
        else if( StartsWith( line, "NoConn " ) )
        {
            std::vector<std::string> words = splitWords( line );

            if( words.size() > 3 )
            {
                appendChild( root.get(), schematicAtItemNode( "no_connect", words[2], words[3],
                                aDocument.Path.string() + ":noconnect:" + line ) );
                ++convertedNoConnects;
            }
        }
        else if( StartsWith( line, "Text " ) && i + 1 < lines.size() )
        {
            std::vector<std::string> words = splitWords( line );

            if( words.size() < 6 )
                continue;

            std::string text = lines[++i];
            std::string head;
            std::string shape;
            std::string size = words[5];

            if( words[1] == "Label" )
                head = "label";
            else if( words[1] == "GLabel" )
            {
                head = "global_label";
                shape = words.size() > 6 ? words[6] : "UnSpc";
            }
            else if( words[1] == "HLabel" )
            {
                head = "hierarchical_label";
                shape = words.size() > 6 ? words[6] : "UnSpc";
            }
            else if( words[1] == "Notes" )
                head = "text";

            if( !head.empty() )
            {
                appendChild( root.get(), schematicLabelNode( head, text, words[2], words[3],
                        words[4], shape, size,
                        aDocument.Path.string() + ":text:" + line + ":" + text ) );
                ++convertedLabels;
            }
        }
    }

    appendChild( root.get(), std::move( sheetInstances ) );
    appendChild( root.get(), std::move( symbolInstances ) );

    if( aWarnings )
    {
        if( convertedSymbols > 0 )
            aWarnings->push_back( "converted legacy schematic component records to symbol instances" );

        if( convertedWires > 0 )
            aWarnings->push_back( "converted legacy schematic wire line records" );

        if( convertedSheets > 0 )
            aWarnings->push_back( "converted legacy schematic sheet records" );

        if( convertedLabels > 0 )
            aWarnings->push_back( "converted legacy schematic text/label records" );

        if( convertedJunctions > 0 )
            aWarnings->push_back( "converted legacy schematic junction records" );

        if( convertedNoConnects > 0 )
            aWarnings->push_back( "converted legacy schematic no-connect records" );

        if( convertedBuses > 0 )
            aWarnings->push_back( "converted legacy schematic bus records" );

        if( convertedBusEntries > 0 )
            aWarnings->push_back( "converted legacy schematic bus-entry records" );

        if( convertedSheetPins > 0 )
            aWarnings->push_back( "converted legacy schematic sheet pin records" );

        aWarnings->push_back( "converted legacy schematic metadata; non-wire drawing items are not yet fully mapped" );
    }

    return sexprFormatted( std::move( root ) );
}


std::string libraryReferencePrefix( const std::string& aName )
{
    for( char ch : aName )
    {
        if( ch >= 'A' && ch <= 'Z' )
            return std::string( 1, ch );
    }

    return "U";
}


std::string legacyPinTypeToSexpr( const std::string& aType )
{
    if( aType == "I" ) return "input";
    if( aType == "O" ) return "output";
    if( aType == "B" ) return "bidirectional";
    if( aType == "T" ) return "tri_state";
    if( aType == "W" ) return "power_in";
    if( aType == "w" ) return "power_out";
    if( aType == "C" ) return "open_collector";
    if( aType == "E" ) return "open_emitter";
    if( aType == "N" ) return "no_connect";
    return "passive";
}


std::string legacyPinShapeToSexpr( const std::string& aShape )
{
    std::string value = Lower( aShape );

    if( value.find( 'x' ) != std::string::npos ) return "non_logic";
    if( value.find( 'f' ) != std::string::npos ) return "edge_clock_high";
    if( value.find( 'v' ) != std::string::npos ) return "output_low";

    if( value.find( 'l' ) != std::string::npos )
        return value.find( 'c' ) != std::string::npos ? "clock_low" : "input_low";

    if( value.find( 'c' ) != std::string::npos
        && value.find( 'i' ) != std::string::npos )
        return "inverted_clock";

    if( value.find( 'c' ) != std::string::npos ) return "clock";
    if( value.find( 'i' ) != std::string::npos ) return "inverted";
    return "line";
}


std::string sexprPinTypeToLegacy( const std::string& aType )
{
    if( aType == "input" ) return "I";
    if( aType == "output" ) return "O";
    if( aType == "bidirectional" ) return "B";
    if( aType == "tri_state" ) return "T";
    if( aType == "power_in" ) return "W";
    if( aType == "power_out" ) return "w";
    if( aType == "open_collector" ) return "C";
    if( aType == "open_emitter" ) return "E";
    if( aType == "no_connect" ) return "N";
    return "P";
}


std::string sexprPinShapeToLegacy( const std::string& aShape )
{
    if( aShape == "inverted" ) return "I";
    if( aShape == "clock" ) return "C";
    if( aShape == "inverted_clock" ) return "IC";
    if( aShape == "input_low" ) return "L";
    if( aShape == "clock_low" ) return "CL";
    if( aShape == "output_low" ) return "V";
    if( aShape == "edge_clock_high" ) return "F";
    if( aShape == "non_logic" ) return "X";
    return "";
}


std::string legacyPinOrientationToAngle( const std::string& aOrientation )
{
    if( aOrientation == "U" ) return "90";
    if( aOrientation == "L" ) return "180";
    if( aOrientation == "D" ) return "270";
    return "0";
}


std::string sexprPinAngleToOrientation( const std::string& aAngle )
{
    int angle = 0;

    if( IsNumber( aAngle ) )
        angle = std::stoi( aAngle ) % 360;

    if( angle < 0 )
        angle += 360;

    if( angle == 90 ) return "U";
    if( angle == 180 ) return "L";
    if( angle == 270 ) return "D";
    return "R";
}


std::unique_ptr<SEXPR::NODE> legacyPinNode( const std::vector<std::string>& aWords )
{
    if( aWords.size() < 12 )
        return nullptr;

    std::unique_ptr<SEXPR::NODE> pin = listNode( "pin" );
    std::string shape = aWords.size() > 12 ? aWords[12] : "";
    appendAtom( pin.get(), legacyPinTypeToSexpr( aWords[11] ) );
    appendAtom( pin.get(), legacyPinShapeToSexpr( shape ) );
    appendChild( pin.get(), atNode( legacyCoordToMm( aWords[3] ),
                                    legacyCoordToMm( aWords[4] ),
                                    legacyPinOrientationToAngle( aWords[6] ) ) );
    appendChild( pin.get(), atomList( "length", legacyCoordToMm( aWords[5] ) ) );

    std::unique_ptr<SEXPR::NODE> name = listNode( "name" );
    appendAtom( name.get(), aWords[1], true );
    appendChild( name.get(), legacyPinEffectsNode( aWords.size() > 7 ? aWords[7] : "50" ) );
    appendChild( pin.get(), std::move( name ) );

    std::unique_ptr<SEXPR::NODE> number = listNode( "number" );
    appendAtom( number.get(), aWords[2], true );
    appendChild( number.get(), legacyPinEffectsNode( aWords.size() > 8 ? aWords[8] : "50" ) );
    appendChild( pin.get(), std::move( number ) );

    if( Lower( shape ).find( 'n' ) != std::string::npos )
        appendAtom( pin.get(), "hide" );

    return pin;
}


std::string sanitizeSymbolName( std::string aName )
{
    if( aName.empty() )
        return "LegacySymbol";

    for( char& ch : aName )
    {
        if( ch == '"' || ch == '\'' || ch == '\\' )
            ch = '_';
    }

    return aName;
}


void appendUnique( std::vector<std::string>& aValues, const std::string& aValue )
{
    if( !aValue.empty() && std::find( aValues.begin(), aValues.end(), aValue ) == aValues.end() )
        aValues.push_back( aValue );
}


std::string firstSymbolLibraryNickname( const std::filesystem::path& aSchematicPath )
{
    std::filesystem::path tablePath = aSchematicPath.parent_path() / "sym-lib-table";

    if( !std::filesystem::exists( tablePath ) )
        return "";

    try
    {
        std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( ReadTextFile( tablePath ) );

        if( !root || root->HeadView() != "sym_lib_table" )
            return "";

        std::vector<SEXPR::NODE*> libs = root->ChildLists( "lib" );

        for( SEXPR::NODE* lib : libs )
        {
            SEXPR::NODE* nameNode = lib ? lib->ChildList( "name" ) : nullptr;
            std::string name = nameNode ? nameNode->AtomAt( 1 ) : "";

            if( !name.empty() )
                return name;
        }
    }
    catch( ... )
    {
    }

    return "";
}


std::vector<std::string> symbolLibraryNicknames( const std::filesystem::path& aProjectPath )
{
    std::vector<std::string> names;
    std::filesystem::path tablePath = aProjectPath.parent_path() / "sym-lib-table";

    if( !std::filesystem::exists( tablePath ) )
        return names;

    try
    {
        std::unique_ptr<SEXPR::NODE> root = SEXPR::Parse( ReadTextFile( tablePath ) );

        if( !root || root->HeadView() != "sym_lib_table" )
            return names;

        for( SEXPR::NODE* lib : root->ChildLists( "lib" ) )
        {
            SEXPR::NODE* nameNode = lib ? lib->ChildList( "name" ) : nullptr;
            std::string name = nameNode ? nameNode->AtomAt( 1 ) : "";
            appendUnique( names, name );
        }
    }
    catch( ... )
    {
    }

    return names;
}


std::vector<std::string> localSymbolLibraryStems( const std::filesystem::path& aProjectPath )
{
    std::vector<std::string> names;
    std::filesystem::path dir = aProjectPath.parent_path();

    if( !std::filesystem::exists( dir ) )
        return names;

    std::error_code error;

    for( const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator( dir, error ) )
    {
        if( error )
            break;

        if( !entry.is_regular_file() )
            continue;

        if( Lower( entry.path().extension().string() ) == ".kicad_sym" )
            appendUnique( names, entry.path().stem().string() );
    }

    return names;
}


std::string legacySchematicSymbolName( const std::string& aLibId )
{
    std::string::size_type sep = aLibId.rfind( ':' );
    std::string name = sep == std::string::npos ? aLibId : aLibId.substr( sep + 1 );
    return sanitizeSymbolName( name );
}


std::string legacySchematicLibraryName( const std::string& aLibId,
                                        const std::filesystem::path& aSchematicPath )
{
    std::string::size_type sep = aLibId.rfind( ':' );

    if( sep != std::string::npos )
        return sanitizeSymbolName( aLibId.substr( 0, sep ) );

    std::string nickname = firstSymbolLibraryNickname( aSchematicPath );

    if( !nickname.empty() )
        return sanitizeSymbolName( nickname );

    return "";
}


std::string legacySchematicComponentLibId( const std::string& aLibId,
                                           const std::filesystem::path& aSchematicPath,
                                           int aTargetMajor )
{
    std::string symbolName = legacySchematicSymbolName( aLibId );

    if( aTargetMajor <= 4 )
        return symbolName;

    std::string libraryName = legacySchematicLibraryName( aLibId, aSchematicPath );

    if( libraryName.empty() )
        return symbolName;

    return sanitizeSymbolName( libraryName ) + ":" + symbolName;
}


std::vector<std::string> schematicLegacyLibraryNames( SEXPR::NODE* aRoot,
                                                      const std::filesystem::path& aSchematicPath )
{
    std::vector<std::string> names;

    if( aRoot )
    {
        for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->HeadView() != "symbol" )
                continue;

            SEXPR::NODE* libIdNode = child->ChildList( "lib_id" );
            std::string libId = libIdNode ? libIdNode->AtomAt( 1 ) : "";
            appendUnique( names, legacySchematicLibraryName( libId, aSchematicPath ) );
        }
    }

    for( const std::string& nickname : symbolLibraryNicknames( aSchematicPath ) )
        appendUnique( names, sanitizeSymbolName( nickname ) );

    if( names.empty() )
        appendUnique( names, firstSymbolLibraryNickname( aSchematicPath ) );

    if( !aRoot )
        return names;

    return names;
}


std::string legacyLibraryToSexpr( const DOCUMENT& aDocument, const std::string& aTargetVersion,
                                  std::vector<std::string>* aWarnings )
{
    std::unique_ptr<SEXPR::NODE> root = listNode( "kicad_symbol_lib" );
    bool usePropertyIds = !IsNumber( aTargetVersion ) || std::stoi( aTargetVersion ) < 20220914;

    appendChild( root.get(), atomList( "version", aTargetVersion ) );
    appendChild( root.get(), atomList( "generator", "kicad-backport" ) );

    std::map<std::string, LEGACY_DOC_META> docMeta;
    std::filesystem::path dcmPath = aDocument.Path;
    dcmPath.replace_extension( ".dcm" );

    if( std::filesystem::exists( dcmPath ) )
    {
        std::string currentName;
        LEGACY_DOC_META currentMeta;

        auto flushDoc = [&]()
        {
            if( !currentName.empty() )
                docMeta[currentName] = currentMeta;

            currentName.clear();
            currentMeta = LEGACY_DOC_META();
        };

        for( const std::string& line : linesOf( ReadTextFile( dcmPath ) ) )
        {
            if( StartsWith( line, "$CMP " ) )
            {
                flushDoc();
                currentName = sanitizeSymbolName( Trim( line.substr( 5 ) ) );
            }
            else if( StartsWith( line, "D " ) )
            {
                currentMeta.Description = Trim( line.substr( 2 ) );
            }
            else if( StartsWith( line, "K " ) )
            {
                currentMeta.Keywords = Trim( line.substr( 2 ) );
            }
            else if( StartsWith( line, "F " ) )
            {
                currentMeta.Datasheet = Trim( line.substr( 2 ) );
            }
            else if( line == "$ENDCMP" )
            {
                flushDoc();
            }
        }

        flushDoc();
    }

    int converted = 0;
    int convertedPins = 0;
    int convertedDrawItems = 0;
    int convertedAliases = 0;
    std::vector<std::string> lines = linesOf( aDocument.RawText );

    for( size_t i = 0; i < lines.size(); ++i )
    {
        const std::string& line = lines[i];

        if( !StartsWith( line, "DEF " ) )
            continue;

        std::vector<std::string> words = splitWords( line );

        if( words.size() < 2 )
            continue;

        std::string name = sanitizeSymbolName( words[1] );
        std::string value = name;
        std::string reference = words.size() > 2
                ? legacyLibraryReferencePrefix( words[2], name )
                : legacyLibraryReferencePrefix( "", name );

        std::unique_ptr<SEXPR::NODE> symbol = listNode( "symbol" );
        appendAtom( symbol.get(), name, true );
        bool showPinNumbers = words.size() > 5 ? words[5] != "N" : true;
        bool showPinNames = words.size() > 6 ? words[6] != "N" : true;
        LEGACY_SYMBOL_DEF symbolDef;
        symbolDef.Reference = reference;
        symbolDef.ShowPinNumbers = showPinNumbers;
        symbolDef.ShowPinNames = showPinNames;
        symbolDef.UnitCount = legacyIntAtOrDefault( words, 7, 1 );
        appendPinVisibilityNodes( symbol.get(), showPinNumbers, showPinNames );
        appendChild( symbol.get(), atomList( "in_bom", "yes" ) );
        appendChild( symbol.get(), atomList( "on_board", "yes" ) );
        std::string footprint;
        std::string datasheet;
        std::vector<std::string> aliases;
        struct LIB_FIELD_STATE
        {
            std::string Value;
            std::string X = "0";
            std::string Y = "0";
            std::string Angle = "0";
            bool Hidden = false;
        };
        LIB_FIELD_STATE referenceField{ reference, "0", "0", "0", false };
        LIB_FIELD_STATE valueField{ value, "0", "0", "0", false };
        LIB_FIELD_STATE footprintField{ footprint, "0", "0", "0", true };
        LIB_FIELD_STATE datasheetField{ datasheet, "0", "0", "0", true };

        while( i + 1 < lines.size() )
        {
            const std::string& item = lines[++i];

            if( item == "ENDDEF" )
                break;

            std::vector<std::string> itemWords = splitWords( item );

            if( itemWords.empty() )
                continue;

            if( itemWords[0] == "F0" )
            {
                reference = legacyLibraryReferencePrefix( firstQuotedValue( item ), name );
                referenceField.Value = reference;

                if( itemWords.size() >= 6 )
                {
                    referenceField.X = legacyCoordToMm( itemWords[2] );
                    referenceField.Y = legacyCoordToMm( itemWords[3] );
                    referenceField.Angle = legacyFieldOrientationToAngle( itemWords[5] );
                    referenceField.Hidden = legacyLibraryFieldHidden( itemWords, false );
                }
            }
            else if( itemWords[0] == "F1" )
            {
                std::string parsedValue = firstQuotedValue( item );

                if( !parsedValue.empty() )
                    value = parsedValue;

                valueField.Value = value;

                if( itemWords.size() >= 6 )
                {
                    valueField.X = legacyCoordToMm( itemWords[2] );
                    valueField.Y = legacyCoordToMm( itemWords[3] );
                    valueField.Angle = legacyFieldOrientationToAngle( itemWords[5] );
                    valueField.Hidden = legacyLibraryFieldHidden( itemWords, false );
                }
            }
            else if( itemWords[0] == "F2" )
            {
                footprint = firstQuotedValue( item );
                footprintField.Value = footprint;

                if( itemWords.size() >= 6 )
                {
                    footprintField.X = legacyCoordToMm( itemWords[2] );
                    footprintField.Y = legacyCoordToMm( itemWords[3] );
                    footprintField.Angle = legacyFieldOrientationToAngle( itemWords[5] );
                    footprintField.Hidden = legacyLibraryFieldHidden( itemWords, true );
                }
            }
            else if( itemWords[0] == "F3" )
            {
                datasheet = firstQuotedValue( item );
                datasheetField.Value = datasheet;

                if( itemWords.size() >= 6 )
                {
                    datasheetField.X = legacyCoordToMm( itemWords[2] );
                    datasheetField.Y = legacyCoordToMm( itemWords[3] );
                    datasheetField.Angle = legacyFieldOrientationToAngle( itemWords[5] );
                    datasheetField.Hidden = legacyLibraryFieldHidden( itemWords, true );
                }
            }
            else if( itemWords[0] == "X" )
            {
                symbolDef.PinWords.push_back( { legacyPinUnit( itemWords ), itemWords } );
                ++convertedPins;
            }
            else if( itemWords[0] == "ALIAS" )
            {
                for( size_t aliasIndex = 1; aliasIndex < itemWords.size(); ++aliasIndex )
                    aliases.push_back( sanitizeSymbolName( itemWords[aliasIndex] ) );
            }
            else if( itemWords[0] == "S" && itemWords.size() >= 9 )
            {
                symbolDef.DrawLines.push_back( { legacyDrawUnit( itemWords ), item } );
                ++convertedDrawItems;
            }
            else if( itemWords[0] == "P" && itemWords.size() >= 7 && IsNumber( itemWords[1] ) )
            {
                int pointCount = std::stoi( itemWords[1] );
                size_t firstCoord = 5;

                if( itemWords.size() >= firstCoord + static_cast<size_t>( pointCount ) * 2 )
                {
                    symbolDef.DrawLines.push_back( { legacyDrawUnit( itemWords ), item } );
                    ++convertedDrawItems;
                }
            }
            else if( itemWords[0] == "C" && itemWords.size() >= 8 )
            {
                symbolDef.DrawLines.push_back( { legacyDrawUnit( itemWords ), item } );
                ++convertedDrawItems;
            }
            else if( itemWords[0] == "A" && itemWords.size() >= 13 )
            {
                symbolDef.DrawLines.push_back( { legacyDrawUnit( itemWords ), item } );
                ++convertedDrawItems;
            }
            else if( itemWords[0] == "T" && itemWords.size() >= 8 )
            {
                symbolDef.DrawLines.push_back( { legacyDrawUnit( itemWords ), item } );
                ++convertedDrawItems;
            }
        }

        appendChild( symbol.get(), schematicPropertyNode( "Reference", referenceField.Value,
                referenceField.X, referenceField.Y, "0", referenceField.Hidden,
                referenceField.Angle, usePropertyIds ) );
        appendChild( symbol.get(), schematicPropertyNode( "Value", valueField.Value,
                valueField.X, valueField.Y, "1", valueField.Hidden,
                valueField.Angle, usePropertyIds ) );
        appendChild( symbol.get(), schematicPropertyNode( "Footprint", footprintField.Value,
                footprintField.X, footprintField.Y, "2", footprintField.Hidden,
                footprintField.Angle, usePropertyIds ) );
        LEGACY_DOC_META meta = docMeta[name];
        std::string mergedDatasheet = meta.Datasheet.empty() ? datasheet : meta.Datasheet;
        datasheetField.Value = mergedDatasheet;
        appendChild( symbol.get(), schematicPropertyNode( "Datasheet", datasheetField.Value,
                datasheetField.X, datasheetField.Y, "3", datasheetField.Hidden,
                datasheetField.Angle, usePropertyIds ) );

        if( !meta.Description.empty() )
            appendChild( symbol.get(), propertyNode( "ki_description", meta.Description, "4",
                                                     false, usePropertyIds ) );

        if( !meta.Keywords.empty() )
            appendChild( symbol.get(), propertyNode( "ki_keywords", meta.Keywords, "5",
                                                     false, usePropertyIds ) );

        std::string unitBaseName = symbolNameWithoutLibraryPrefix( name );

        if( legacySymbolHasMultipleUnits( symbolDef ) )
            appendLegacyUnitSubSymbols( symbol.get(), unitBaseName, symbolDef );
        else
            appendLegacySymbolItemsForUnit( symbol.get(), symbolDef, 1, true );

        appendChild( root.get(), std::move( symbol ) );

        for( const std::string& alias : aliases )
        {
            if( alias.empty() || alias == name )
                continue;

            std::unique_ptr<SEXPR::NODE> aliasSymbol = listNode( "symbol" );
            appendAtom( aliasSymbol.get(), alias, true );
            appendChild( aliasSymbol.get(), atomList( "extends", name, true ) );

            if( legacySymbolHasMultipleUnits( symbolDef ) )
                appendAliasUnitSubSymbols( aliasSymbol.get(),
                                           symbolNameWithoutLibraryPrefix( alias ),
                                           unitBaseName, symbolDef );

            appendChild( root.get(), std::move( aliasSymbol ) );
            ++convertedAliases;
        }

        ++converted;
    }

    if( aWarnings )
    {
        if( converted == 0 )
            aWarnings->push_back( "converted legacy symbol library header only; no DEF records were found" );
        else
            aWarnings->push_back( "converted legacy symbol DEF records; drawing primitives and pins are not yet fully mapped" );

        if( convertedPins > 0 )
            aWarnings->push_back( "converted legacy symbol pin records" );

        if( convertedDrawItems > 0 )
            aWarnings->push_back( "converted legacy symbol drawing primitives" );

        if( convertedAliases > 0 )
            aWarnings->push_back( "converted legacy symbol aliases" );

        if( !docMeta.empty() )
            aWarnings->push_back( "merged paired legacy .dcm documentation metadata into symbol properties" );
    }

    return sexprFormatted( std::move( root ) );
}


std::string legacyDocumentationToSexpr( const DOCUMENT& aDocument,
                                        const std::string& aTargetVersion,
                                        std::vector<std::string>* aWarnings )
{
    std::unique_ptr<SEXPR::NODE> root = listNode( "kicad_symbol_lib" );
    bool usePropertyIds = !IsNumber( aTargetVersion ) || std::stoi( aTargetVersion ) < 20220914;

    appendChild( root.get(), atomList( "version", aTargetVersion ) );
    appendChild( root.get(), atomList( "generator", "kicad-backport" ) );

    int docs = 0;

    for( const std::string& line : linesOf( aDocument.RawText ) )
    {
        if( !StartsWith( line, "$CMP " ) )
            continue;

        std::vector<std::string> words = splitWords( line );

        if( words.size() < 2 )
            continue;

        std::string name = sanitizeSymbolName( words[1] );
        std::unique_ptr<SEXPR::NODE> symbol = listNode( "symbol" );
        appendAtom( symbol.get(), name, true );
        appendChild( symbol.get(), atomList( "in_bom", "yes" ) );
        appendChild( symbol.get(), atomList( "on_board", "yes" ) );
        appendChild( symbol.get(), propertyNode( "Reference", libraryReferencePrefix( name ), "0",
                                                 false, usePropertyIds ) );
        appendChild( symbol.get(), propertyNode( "Value", name, "1", false,
                                                 usePropertyIds ) );
        appendChild( symbol.get(), propertyNode( "Footprint", "", "2", true,
                                                 usePropertyIds ) );
        appendChild( symbol.get(), propertyNode( "Datasheet", "", "3", true,
                                                 usePropertyIds ) );
        appendChild( root.get(), std::move( symbol ) );
        ++docs;
    }

    if( aWarnings )
    {
        if( docs == 0 )
            aWarnings->push_back( "converted legacy documentation file to an empty symbol library" );
        else
            aWarnings->push_back( "converted legacy documentation component names; descriptions are not yet fully mapped" );
    }

    return sexprFormatted( std::move( root ) );
}


std::string legacyProjectToJson( const DOCUMENT& aDocument,
                                 std::vector<std::string>* aWarnings )
{
    std::string projectName = aDocument.Path.stem().string();
    LEGACY_PROJECT_META meta = parseLegacyProjectMeta( aDocument.RawText );
    const std::vector<std::string> settingOrder = {
        "update", "version", "last_client", "LibDir", "NetIExt", "CmpExt",
        "PageLayoutDescrFile", "PlotDirectoryName", "SubpartIdSeparator",
        "SubpartFirstId"
    };
    std::ostringstream out;
    out << "{\n";
    out << "  \"board\": {},\n";
    out << "  \"libraries\": {\n";
    out << "    \"legacy_symbol_libraries\": [";

    for( size_t i = 0; i < meta.Libraries.size(); ++i )
    {
        if( i > 0 )
            out << ", ";

        out << "\"" << JsonEscape( meta.Libraries[i] ) << "\"";
    }

    out << "]\n";
    out << "  },\n";
    out << "  \"legacy\": {\n";
    out << "    \"project_settings\": {\n";

    bool wroteSetting = false;

    for( const std::string& key : settingOrder )
    {
        auto found = meta.Settings.find( key );

        if( found == meta.Settings.end() )
            continue;

        if( wroteSetting )
            out << ",\n";

        out << "      \"" << JsonEscape( key ) << "\": \""
            << JsonEscape( found->second ) << "\"";
        wroteSetting = true;
    }

    out << "\n";
    out << "    }\n";
    out << "  },\n";
    out << "  \"meta\": {\n";
    out << "    \"filename\": \"" << JsonEscape( projectName + ".kicad_pro" ) << "\",\n";
    out << "    \"version\": 1\n";
    out << "  },\n";
    out << "  \"net_settings\": {},\n";
    out << "  \"schematic\": {}\n";
    out << "}\n";

    if( aWarnings )
    {
        aWarnings->push_back( "converted legacy project to minimal KiCad 6+ project JSON" );

        if( !meta.Settings.empty() )
            aWarnings->push_back( "preserved legacy project settings in JSON legacy.project_settings" );

        if( !meta.Libraries.empty() )
            aWarnings->push_back( "preserved legacy project symbol library names" );
    }

    return out.str();
}


std::string childAtomOrEmpty( SEXPR::NODE* aNode, const std::string& aHead,
                              size_t aIndex = 1 )
{
    SEXPR::NODE* child = aNode ? aNode->ChildList( aHead ) : nullptr;
    return child ? child->AtomAt( aIndex ) : "";
}


LEGACY_SYMBOL_TRANSFORM legacySymbolTransform( SEXPR::NODE* aSymbol )
{
    SEXPR::NODE* at = aSymbol ? aSymbol->ChildList( "at" ) : nullptr;
    int angle = at ? roundedRightAngle( at->AtomAt( 3 ) ) : 0;
    std::string mirror = childAtomOrEmpty( aSymbol, "mirror" );

    LEGACY_SYMBOL_TRANSFORM transform;

    switch( angle )
    {
    case 90:
        transform = { 0, -1, -1, 0 };
        break;

    case 180:
        transform = { -1, 0, 0, 1 };
        break;

    case 270:
        transform = { 0, 1, 1, 0 };
        break;

    default:
        transform = { 1, 0, 0, -1 };
        break;
    }

    // KiCad 5 stores schematic mirrors as a post-transform flip in the $Comp matrix.
    if( mirror == "x" )
    {
        transform.M21 = -transform.M21;
        transform.M22 = -transform.M22;
    }
    else if( mirror == "y" )
    {
        transform.M11 = -transform.M11;
        transform.M12 = -transform.M12;
    }

    return transform;
}


std::string propertyValue( SEXPR::NODE* aNode, const std::string& aName )
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


SEXPR::NODE* propertyNode( SEXPR::NODE* aNode, const std::string& aName )
{
    if( !aNode )
        return nullptr;

    for( const std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "property"
            && child->AtomAt( 1 ) == aName )
        {
            return child.get();
        }
    }

    return nullptr;
}


std::string legacyFieldOrientation( SEXPR::NODE* aAt )
{
    int angle = aAt ? roundedRightAngle( aAt->AtomAt( 3 ) ) : 0;
    return angle == 90 || angle == 270 ? "V" : "H";
}


std::string legacyLibraryTextOrientation( SEXPR::NODE* aAt )
{
    int angle = aAt ? roundedRightAngle( aAt->AtomAt( 3 ) ) : 0;
    return angle == 90 || angle == 270 ? "1" : "0";
}


std::string legacyLibraryFieldOrientation( SEXPR::NODE* aAt )
{
    int angle = aAt ? roundedRightAngle( aAt->AtomAt( 3 ) ) : 0;
    return angle == 90 || angle == 270 ? "V" : "H";
}


bool sexprBoolValue( const std::string& aValue, bool aDefault )
{
    std::string value = Lower( aValue );

    if( value == "yes" || value == "true" || value == "1" )
        return true;

    if( value == "no" || value == "false" || value == "0" )
        return false;

    return aDefault;
}


bool nodeHasAtom( SEXPR::NODE* aNode, const std::string& aAtom )
{
    if( !aNode )
        return false;

    for( const std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
    {
        if( child && child->IsAtom()
            && std::string_view( child->Atom.data(), child->Atom.size() ) == aAtom )
            return true;
    }

    return false;
}


bool propertyHidden( SEXPR::NODE* aNode, bool aDefaultHidden )
{
    if( !aNode )
        return aDefaultHidden;

    SEXPR::NODE* hide = aNode->ChildList( "hide" );

    if( hide )
        return sexprBoolValue( hide->AtomAt( 1 ), true );

    SEXPR::NODE* effects = aNode->ChildList( "effects" );

    if( !effects )
        return false;

    if( nodeHasAtom( effects, "hide" ) )
        return true;

    return false;
}


std::string symbolPinVisibilityFlag( SEXPR::NODE* aSymbol, const std::string& aHead )
{
    SEXPR::NODE* node = aSymbol ? aSymbol->ChildList( aHead ) : nullptr;

    if( !node )
        return "Y";

    if( node->AtomAt( 1 ) == "hide" )
        return "N";

    SEXPR::NODE* hide = node->ChildList( "hide" );
    return hide && sexprBoolValue( hide->AtomAt( 1 ), true ) ? "N" : "Y";
}


std::string effectsFontSizeLegacy( SEXPR::NODE* aNode )
{
    SEXPR::NODE* effects = aNode ? aNode->ChildList( "effects" ) : nullptr;
    SEXPR::NODE* font = effects ? effects->ChildList( "font" ) : nullptr;
    SEXPR::NODE* size = font ? font->ChildList( "size" ) : nullptr;

    if( size && !size->AtomAt( 1 ).empty() )
        return std::to_string( mmToLegacyCoord( size->AtomAt( 1 ) ) );

    return "50";
}


std::string pinTextSizeLegacy( SEXPR::NODE* aPin, const std::string& aHead )
{
    SEXPR::NODE* node = aPin ? aPin->ChildList( aHead ) : nullptr;
    return effectsFontSizeLegacy( node );
}


bool sexprPinHidden( SEXPR::NODE* aPin )
{
    if( !aPin )
        return false;

    SEXPR::NODE* hide = aPin->ChildList( "hide" );

    if( hide )
        return sexprBoolValue( hide->AtomAt( 1 ), true );

    return nodeHasAtom( aPin, "hide" );
}


std::pair<std::string, std::string> legacySubsymbolUnitConvert( const std::string& aName )
{
    size_t second = aName.rfind( '_' );

    if( second == std::string::npos || second == 0 )
        return { "1", "1" };

    size_t first = aName.rfind( '_', second - 1 );

    if( first == std::string::npos )
        return { "1", "1" };

    std::string unit = aName.substr( first + 1, second - first - 1 );
    std::string convert = aName.substr( second + 1 );

    if( IsNumber( unit ) && IsNumber( convert ) )
        return { unit, convert };

    return { "1", "1" };
}


int symbolLegacyUnitCount( SEXPR::NODE* aSymbol )
{
    int count = 1;

    if( !aSymbol )
        return count;

    for( const std::unique_ptr<SEXPR::NODE>& child : aSymbol->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "symbol" )
        {
            auto unitConvert = legacySubsymbolUnitConvert( child->AtomAt( 1 ) );

            if( IsNumber( unitConvert.first ) )
                count = std::max( count, std::stoi( unitConvert.first ) );
        }
    }

    return count;
}


void writeLegacyLibraryStandardPropertyField( std::ostringstream& aOut, SEXPR::NODE* aSymbol,
                                              int aIndex, const std::string& aName,
                                              bool aDefaultHidden )
{
    SEXPR::NODE* prop = propertyNode( aSymbol, aName );
    SEXPR::NODE* at = prop ? prop->ChildList( "at" ) : nullptr;
    int x = at ? mmToLegacyCoord( at->AtomAt( 1 ) ) : 0;
    int y = at ? mmToLegacyCoord( at->AtomAt( 2 ) ) : 0;
    bool hidden = propertyHidden( prop, aDefaultHidden );

    aOut << "F" << aIndex << " "
         << legacyQuote( prop ? prop->AtomAt( 2 ) : "" )
         << " " << x << " " << y << " 50 "
         << legacyLibraryFieldOrientation( at ) << " " << ( hidden ? "I" : "V" )
         << " C CNN\n";
}


void writeLegacyLibraryCustomPropertyFields( std::ostringstream& aOut, SEXPR::NODE* aSymbol )
{
    static const std::set<std::string> skip =
    {
        "Reference", "Value", "Footprint", "Datasheet",
        "Description", "ki_description", "ki_keywords"
    };

    if( !aSymbol )
        return;

    int index = 4;

    for( const std::unique_ptr<SEXPR::NODE>& child : aSymbol->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "property" )
            continue;

        std::string name = child->AtomAt( 1 );

        if( skip.count( name ) )
            continue;

        SEXPR::NODE* at = child->ChildList( "at" );
        int x = at ? mmToLegacyCoord( at->AtomAt( 1 ) ) : 0;
        int y = at ? mmToLegacyCoord( at->AtomAt( 2 ) ) : 0;

        aOut << "F" << index++ << " " << legacyQuote( child->AtomAt( 2 ) )
             << " " << x << " " << y << " 50 "
             << legacyLibraryFieldOrientation( at ) << " "
             << ( propertyHidden( child.get(), false ) ? "I" : "V" )
             << " C CNN " << legacyQuote( name ) << "\n";
    }
}


void writeLegacySchematicField( std::ostringstream& aOut, int aIndex,
                                SEXPR::NODE* aSymbol, const std::string& aName,
                                const std::string& aValue, int aFallbackX,
                                int aFallbackY, bool aHidden )
{
    SEXPR::NODE* prop = propertyNode( aSymbol, aName );
    SEXPR::NODE* at = prop ? prop->ChildList( "at" ) : nullptr;
    int x = at ? mmToLegacyCoord( at->AtomAt( 1 ) ) : aFallbackX;
    int y = at ? mmToLegacyCoord( at->AtomAt( 2 ) ) : aFallbackY;

    aOut << "F " << aIndex << " " << legacyQuote( aValue ) << " "
         << legacyFieldOrientation( at ) << " " << x << " " << y
         << " 50  " << ( aHidden ? "0001" : "0000" ) << " C CNN\n";
}


std::string propertyValueAny( SEXPR::NODE* aNode, const std::vector<std::string>& aNames )
{
    for( const std::string& name : aNames )
    {
        std::string value = propertyValue( aNode, name );

        if( !value.empty() )
            return value;
    }

    return "";
}


LEGACY_SCHEMATIC_META schematicMetaFromSexpr( SEXPR::NODE* aRoot )
{
    LEGACY_SCHEMATIC_META meta;
    meta.Paper = childAtomOrEmpty( aRoot, "paper" );

    if( meta.Paper.empty() )
        meta.Paper = "A4";

    SEXPR::NODE* title = aRoot ? aRoot->ChildList( "title_block" ) : nullptr;

    if( !title )
        return meta;

    meta.Title = childAtomOrEmpty( title, "title" );
    meta.Date = childAtomOrEmpty( title, "date" );
    meta.Rev = childAtomOrEmpty( title, "rev" );
    meta.Company = childAtomOrEmpty( title, "company" );

    for( const std::unique_ptr<SEXPR::NODE>& child : title->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "comment"
            && IsNumber( child->AtomAt( 1 ) ) )
        {
            int index = std::stoi( child->AtomAt( 1 ) );

            if( index >= 1 && index <= 9 )
                meta.Comments[static_cast<size_t>( index - 1 )] = child->AtomAt( 2 );
        }
    }

    return meta;
}


std::string sexprSchematicToLegacy( const DOCUMENT& aDocument, int aTargetMajor,
                                    std::vector<std::string>* aWarnings )
{
    LEGACY_SCHEMATIC_META meta = schematicMetaFromSexpr( aDocument.Root.get() );
    int version = aTargetMajor <= 4 ? 2 : 4;
    std::vector<std::string> libraries = schematicLegacyLibraryNames( aDocument.Root.get(),
                                                                      aDocument.Path );
    std::ostringstream out;

    out << "EESchema Schematic File Version " << version << "\n";

    for( const std::string& library : libraries )
        out << "LIBS:" << library << "\n";

    out << "EELAYER 30 0\n";
    out << "EELAYER END\n";
    out << "$Descr " << ( meta.Paper.empty() ? "A4" : meta.Paper ) << " 11693 8268\n";
    out << "encoding utf-8\n";
    out << "Sheet 1 1\n";
    out << "Title " << legacyQuote( meta.Title ) << "\n";
    out << "Date " << legacyQuote( meta.Date ) << "\n";
    out << "Rev " << legacyQuote( meta.Rev ) << "\n";
    out << "Comp " << legacyQuote( meta.Company ) << "\n";

    for( size_t i = 0; i < meta.Comments.size(); ++i )
        out << "Comment" << ( i + 1 ) << " " << legacyQuote( meta.Comments[i] ) << "\n";

    out << "$EndDescr\n";

    int symbolCount = 0;
    int wireCount = 0;
    int sheetCount = 0;
    int labelCount = 0;
    int junctionCount = 0;
    int noConnectCount = 0;
    int busCount = 0;
    int busEntryCount = 0;
    int sheetPinCount = 0;

    if( aDocument.Root )
    {
        for( const std::unique_ptr<SEXPR::NODE>& child : aDocument.Root->Children )
        {
            if( !child || child->IsAtom() )
                continue;

            if( child->HeadView() == "wire" )
            {
                SEXPR::NODE* pts = child->ChildList( "pts" );
                std::vector<SEXPR::NODE*> xy = pts ? pts->ChildLists( "xy" )
                                                   : std::vector<SEXPR::NODE*>();

                if( xy.size() < 2 )
                    continue;

                out << "Wire Wire Line\n";
                out << "\t" << mmToLegacyCoord( xy[0]->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( xy[0]->AtomAt( 2 ) )
                    << " " << mmToLegacyCoord( xy[1]->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( xy[1]->AtomAt( 2 ) ) << "\n";
                ++wireCount;
            }
            else if( child->HeadView() == "bus" )
            {
                SEXPR::NODE* pts = child->ChildList( "pts" );
                std::vector<SEXPR::NODE*> xy = pts ? pts->ChildLists( "xy" )
                                                   : std::vector<SEXPR::NODE*>();

                if( xy.size() < 2 )
                    continue;

                out << "Wire Bus Line\n";
                out << "\t" << mmToLegacyCoord( xy[0]->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( xy[0]->AtomAt( 2 ) )
                    << " " << mmToLegacyCoord( xy[1]->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( xy[1]->AtomAt( 2 ) ) << "\n";
                ++busCount;
            }
            else if( child->HeadView() == "bus_entry" )
            {
                SEXPR::NODE* at = child->ChildList( "at" );
                SEXPR::NODE* size = child->ChildList( "size" );

                if( !at || !size )
                    continue;

                int x = mmToLegacyCoord( at->AtomAt( 1 ) );
                int y = mmToLegacyCoord( at->AtomAt( 2 ) );
                int x2 = x + mmToLegacyCoord( size->AtomAt( 1 ) );
                int y2 = y + mmToLegacyCoord( size->AtomAt( 2 ) );

                out << "Entry Wire Line\n";
                out << "\t" << x << " " << y << " " << x2 << " " << y2 << "\n";
                ++busEntryCount;
            }
            else if( child->HeadView() == "junction" )
            {
                SEXPR::NODE* at = child->ChildList( "at" );

                if( at )
                {
                    out << "Connection ~ " << mmToLegacyCoord( at->AtomAt( 1 ) )
                        << " " << mmToLegacyCoord( at->AtomAt( 2 ) ) << "\n";
                    ++junctionCount;
                }
            }
            else if( child->HeadView() == "no_connect" )
            {
                SEXPR::NODE* at = child->ChildList( "at" );

                if( at )
                {
                    out << "NoConn ~ " << mmToLegacyCoord( at->AtomAt( 1 ) )
                        << " " << mmToLegacyCoord( at->AtomAt( 2 ) ) << "\n";
                    ++noConnectCount;
                }
            }
            else if( child->HeadView() == "label" || child->HeadView() == "global_label"
                     || child->HeadView() == "hierarchical_label" || child->HeadView() == "text" )
            {
                SEXPR::NODE* at = child->ChildList( "at" );

                if( !at )
                    continue;

                std::string legacyType = "Notes";

                if( child->HeadView() == "label" )
                    legacyType = "Label";
                else if( child->HeadView() == "global_label" )
                    legacyType = "GLabel";
                else if( child->HeadView() == "hierarchical_label" )
                    legacyType = "HLabel";

                out << "Text " << legacyType << " " << mmToLegacyCoord( at->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( at->AtomAt( 2 ) )
                    << " " << sexprAngleToLegacyTextOrientation( at->AtomAt( 3 ) )
                    << " 50";

                if( legacyType == "GLabel" || legacyType == "HLabel" )
                    out << " " << sexprLabelShapeToLegacy( childAtomOrEmpty( child.get(), "shape" ) );

                out << " ~ 0\n";
                out << child->AtomAt( 1 ) << "\n";
                ++labelCount;
            }
            else if( child->HeadView() == "sheet" )
            {
                SEXPR::NODE* at = child->ChildList( "at" );
                SEXPR::NODE* size = child->ChildList( "size" );

                if( !at || !size )
                    continue;

                std::string sheetName = propertyValueAny( child.get(), { "Sheet name", "Sheetname" } );
                std::string sheetFile = propertyValueAny( child.get(), { "Sheet file", "Sheetfile" } );
                std::string uuid = childAtomOrEmpty( child.get(), "tstamp" );

                if( uuid.empty() )
                    uuid = childAtomOrEmpty( child.get(), "uuid" );

                uuid = legacyTstamp( uuid, aDocument.Path.string() + ":sheet:" + sheetFile );

                if( sheetName.empty() && !sheetFile.empty() )
                    sheetName = replaceTrailingExtension( sheetFile, ".kicad_sch", "" );

                if( sheetName.empty() )
                    sheetName = "Sheet_" + uuid;

                if( sheetFile.empty() )
                    sheetFile = sheetName + ".sch";
                else
                    sheetFile = replaceTrailingExtension( sheetFile, ".kicad_sch", ".sch" );

                out << "$Sheet\n";
                out << "S " << mmToLegacyCoord( at->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( at->AtomAt( 2 ) )
                    << " " << mmToLegacyCoord( size->AtomAt( 1 ) )
                    << " " << mmToLegacyCoord( size->AtomAt( 2 ) ) << "\n";
                out << "U " << uuid << "\n";
                out << "F0 " << legacyQuote( sheetName ) << " 50\n";
                out << "F1 " << legacyQuote( sheetFile ) << " 50\n";

                int fieldIndex = 2;

                for( const std::unique_ptr<SEXPR::NODE>& pin : child->Children )
                {
                    if( !pin || pin->IsAtom() || pin->HeadView() != "pin" )
                        continue;

                    SEXPR::NODE* pinAt = pin->ChildList( "at" );

                    if( !pinAt )
                        continue;

                    out << "F" << fieldIndex++ << " " << legacyQuote( pin->AtomAt( 1 ) )
                        << " " << sexprSheetPinTypeToLegacy( childAtomOrEmpty( pin.get(), "type" ) )
                        << " " << sexprPinAngleToOrientation( pinAt->AtomAt( 3 ) )
                        << " " << mmToLegacyCoord( pinAt->AtomAt( 1 ) )
                        << " " << mmToLegacyCoord( pinAt->AtomAt( 2 ) )
                        << " 50\n";
                    ++sheetPinCount;
                }

                out << "$EndSheet\n";
                ++sheetCount;
            }
            else if( child->HeadView() == "symbol" )
            {
                std::string libId = childAtomOrEmpty( child.get(), "lib_id" );

                if( libId.empty() )
                    continue;

                std::string legacySymbolName = legacySchematicComponentLibId( libId,
                                                                              aDocument.Path,
                                                                              aTargetMajor );
                std::string reference = propertyValue( child.get(), "Reference" );
                std::string value = propertyValue( child.get(), "Value" );
                std::string footprint = propertyValue( child.get(), "Footprint" );
                std::string datasheet = propertyValue( child.get(), "Datasheet" );
                std::string unit = childAtomOrEmpty( child.get(), "unit" );

                if( reference.empty() )
                    reference = libraryReferencePrefix( libId );

                if( value.empty() )
                    value = libId;

                if( unit.empty() )
                    unit = "1";

                SEXPR::NODE* at = child->ChildList( "at" );
                int x = at ? mmToLegacyCoord( at->AtomAt( 1 ) ) : 0;
                int y = at ? mmToLegacyCoord( at->AtomAt( 2 ) ) : 0;
                LEGACY_SYMBOL_TRANSFORM transform = legacySymbolTransform( child.get() );
                std::string stamp = childAtomOrEmpty( child.get(), "tstamp" );

                if( stamp.empty() )
                    stamp = childAtomOrEmpty( child.get(), "uuid" );

                stamp = legacyTstamp( stamp, aDocument.Path.string() + ":" + reference
                                      + ":" + libId );

                out << "$Comp\n";
                out << "L " << legacySymbolName << " " << reference << "\n";
                out << "U " << unit << " 1 " << stamp << "\n";
                out << "P " << x << " " << y << "\n";
                writeLegacySchematicField( out, 0, child.get(), "Reference", reference,
                                           x, y, false );
                writeLegacySchematicField( out, 1, child.get(), "Value", value,
                                           x, y + 100, false );
                writeLegacySchematicField( out, 2, child.get(), "Footprint", footprint,
                                           x, y, true );
                writeLegacySchematicField( out, 3, child.get(), "Datasheet", datasheet,
                                           x, y, true );
                out << "\t1    " << x << " " << y << "\n";
                out << "\t" << transform.M11 << "    " << transform.M12 << "    "
                    << transform.M21 << "    " << transform.M22 << "\n";
                out << "$EndComp\n";
                ++symbolCount;
            }
        }
    }

    out << "$EndSCHEMATC\n";

    if( aWarnings )
    {
        if( symbolCount > 0 )
            aWarnings->push_back( "converted schematic symbols to legacy $Comp records" );

        if( wireCount > 0 )
            aWarnings->push_back( "converted schematic wires to legacy Wire records" );

        if( sheetCount > 0 )
            aWarnings->push_back( "converted schematic sheets to legacy $Sheet records" );

        if( labelCount > 0 )
            aWarnings->push_back( "converted schematic labels/text to legacy Text records" );

        if( junctionCount > 0 )
            aWarnings->push_back( "converted schematic junctions to legacy Connection records" );

        if( noConnectCount > 0 )
            aWarnings->push_back( "converted schematic no-connects to legacy NoConn records" );

        if( busCount > 0 )
            aWarnings->push_back( "converted schematic buses to legacy Wire Bus records" );

        if( busEntryCount > 0 )
            aWarnings->push_back( "converted schematic bus entries to legacy Entry records" );

        if( sheetPinCount > 0 )
            aWarnings->push_back( "converted schematic sheet pins to legacy sheet fields" );

        aWarnings->push_back( "converted schematic page metadata to legacy .sch; modern objects are lossy" );
    }

    return out.str();
}


std::vector<SEXPR::NODE*> topLevelSymbols( SEXPR::NODE* aRoot )
{
    std::vector<SEXPR::NODE*> symbols;

    if( !aRoot )
        return symbols;

    for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "symbol" )
            symbols.push_back( child.get() );
    }

    return symbols;
}


void writeLegacyPins( std::ostringstream& aOut, SEXPR::NODE* aSymbol, int& aCount,
                      const std::string& aUnit = "1", const std::string& aConvert = "1" )
{
    if( !aSymbol )
        return;

    for( const std::unique_ptr<SEXPR::NODE>& child : aSymbol->Children )
    {
        if( !child || child->IsAtom() )
            continue;

        if( child->HeadView() == "pin" )
        {
            SEXPR::NODE* at = child->ChildList( "at" );
            std::string length = childAtomOrEmpty( child.get(), "length" );
            std::string name = childAtomOrEmpty( child.get(), "name" );
            std::string number = childAtomOrEmpty( child.get(), "number" );

            if( name.empty() )
                name = "~";

            if( number.empty() )
                number = "~";

            int x = at ? mmToLegacyCoord( at->AtomAt( 1 ) ) : 0;
            int y = at ? mmToLegacyCoord( at->AtomAt( 2 ) ) : 0;
            int len = length.empty() ? 100 : mmToLegacyCoord( length );
            std::string angle = at ? at->AtomAt( 3 ) : "0";
            std::string shape = sexprPinShapeToLegacy( child->AtomAt( 2 ) );

            if( sexprPinHidden( child.get() ) && shape.find( 'N' ) == std::string::npos )
                shape += "N";

            aOut << "X " << name << " " << number << " " << x << " " << y << " "
                 << len << " " << sexprPinAngleToOrientation( angle )
                 << " " << pinTextSizeLegacy( child.get(), "name" )
                 << " " << pinTextSizeLegacy( child.get(), "number" )
                 << " " << aUnit << " " << aConvert << " "
                 << sexprPinTypeToLegacy( child->AtomAt( 1 ) );

            if( !shape.empty() )
                aOut << " " << shape;

            aOut << "\n";
            ++aCount;
        }
        else if( child->HeadView() == "symbol" )
        {
            auto unitConvert = legacySubsymbolUnitConvert( child->AtomAt( 1 ) );
            writeLegacyPins( aOut, child.get(), aCount, unitConvert.first, unitConvert.second );
        }
    }
}


void writeLegacyDrawItems( std::ostringstream& aOut, SEXPR::NODE* aSymbol, int& aCount,
                           const std::string& aUnit = "1", const std::string& aConvert = "1" )
{
    if( !aSymbol )
        return;

    for( const std::unique_ptr<SEXPR::NODE>& child : aSymbol->Children )
    {
        if( !child || child->IsAtom() )
            continue;

        if( child->HeadView() == "rectangle" )
        {
            SEXPR::NODE* start = child->ChildList( "start" );
            SEXPR::NODE* end = child->ChildList( "end" );

            if( !start || !end )
                continue;

            aOut << "S " << mmToLegacyCoord( start->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( start->AtomAt( 2 ) )
                 << " " << mmToLegacyCoord( end->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( end->AtomAt( 2 ) )
                 << " " << aUnit << " " << aConvert << " " << strokeWidthLegacy( child.get() )
                 << " " << fillLegacy( child.get() ) << "\n";
            ++aCount;
        }
        else if( child->HeadView() == "polyline" )
        {
            SEXPR::NODE* pts = child->ChildList( "pts" );
            std::vector<SEXPR::NODE*> points = pts ? pts->ChildLists( "xy" )
                                                   : std::vector<SEXPR::NODE*>();

            if( points.empty() )
                continue;

            aOut << "P " << points.size() << " " << aUnit << " " << aConvert
                 << " " << strokeWidthLegacy( child.get() );

            for( SEXPR::NODE* point : points )
            {
                aOut << " " << mmToLegacyCoord( point->AtomAt( 1 ) )
                     << " " << mmToLegacyCoord( point->AtomAt( 2 ) );
            }

            aOut << " " << fillLegacy( child.get() ) << "\n";
            ++aCount;
        }
        else if( child->HeadView() == "circle" )
        {
            SEXPR::NODE* center = child->ChildList( "center" );
            std::string radius = childAtomOrEmpty( child.get(), "radius" );

            if( !center )
                continue;

            aOut << "C " << mmToLegacyCoord( center->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( center->AtomAt( 2 ) )
                 << " " << mmToLegacyCoord( radius )
                 << " " << aUnit << " " << aConvert << " " << strokeWidthLegacy( child.get() )
                 << " " << fillLegacy( child.get() ) << "\n";
            ++aCount;
        }
        else if( child->HeadView() == "arc" )
        {
            SEXPR::NODE* start = child->ChildList( "start" );
            SEXPR::NODE* mid = child->ChildList( "mid" );
            SEXPR::NODE* end = child->ChildList( "end" );

            if( !start || !mid || !end )
                continue;

            double sx = parseDoubleOrZero( start->AtomAt( 1 ) );
            double sy = parseDoubleOrZero( start->AtomAt( 2 ) );
            double mx = parseDoubleOrZero( mid->AtomAt( 1 ) );
            double my = parseDoubleOrZero( mid->AtomAt( 2 ) );
            int radius = static_cast<int>( std::llround( std::hypot( sx - mx, sy - my ) / 0.0254 ) );

            aOut << "A " << mmToLegacyCoord( mid->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( mid->AtomAt( 2 ) )
                 << " " << radius
                 << " 0 0 " << aUnit << " " << aConvert << " " << strokeWidthLegacy( child.get() )
                 << " " << fillLegacy( child.get() )
                 << " " << mmToLegacyCoord( start->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( start->AtomAt( 2 ) )
                 << " " << mmToLegacyCoord( end->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( end->AtomAt( 2 ) ) << "\n";
            ++aCount;
        }
        else if( child->HeadView() == "text" )
        {
            SEXPR::NODE* at = child->ChildList( "at" );

            if( !at )
                continue;

            aOut << "T " << legacyLibraryTextOrientation( at )
                 << " " << mmToLegacyCoord( at->AtomAt( 1 ) )
                 << " " << mmToLegacyCoord( at->AtomAt( 2 ) )
                 << " " << effectsFontSizeLegacy( child.get() )
                 << " 0 " << aUnit << " " << aConvert << " "
                 << legacyQuote( child->AtomAt( 1 ) )
                 << " Normal 0 C C\n";
            ++aCount;
        }
        else if( child->HeadView() == "symbol" )
        {
            auto unitConvert = legacySubsymbolUnitConvert( child->AtomAt( 1 ) );
            writeLegacyDrawItems( aOut, child.get(), aCount, unitConvert.first,
                                  unitConvert.second );
        }
    }
}


std::string sexprSymbolLibraryToLegacy( const DOCUMENT& aDocument, int aTargetMajor,
                                        std::vector<std::string>* aWarnings )
{
    std::ostringstream out;
    out << "EESchema-LIBRARY Version " << ( aTargetMajor <= 4 ? "2.3" : "2.4" ) << "\n";
    out << "#encoding utf-8\n";

    int count = 0;
    int pinCount = 0;
    int drawCount = 0;
    int aliasCount = 0;
    std::map<std::string, std::vector<std::string>> aliasesByBase;

    for( SEXPR::NODE* symbol : topLevelSymbols( aDocument.Root.get() ) )
    {
        std::string base = childAtomOrEmpty( symbol, "extends" );

        if( !base.empty() )
            aliasesByBase[base].push_back( sanitizeSymbolName( symbol->AtomAt( 1 ) ) );
    }

    for( SEXPR::NODE* symbol : topLevelSymbols( aDocument.Root.get() ) )
    {
        if( !childAtomOrEmpty( symbol, "extends" ).empty() )
            continue;

        std::string name = sanitizeSymbolName( symbol->AtomAt( 1 ) );
        std::string reference = propertyValue( symbol, "Reference" );

        if( reference.empty() )
            reference = libraryReferencePrefix( name );

        std::string showPinNumbers = symbolPinVisibilityFlag( symbol, "pin_numbers" );
        std::string showPinNames = symbolPinVisibilityFlag( symbol, "pin_names" );
        int unitCount = symbolLegacyUnitCount( symbol );

        out << "#\n";
        out << "# " << name << "\n";
        out << "#\n";
        out << "DEF " << name << " " << reference << " 0 40 "
            << showPinNumbers << " " << showPinNames << " " << unitCount << " F N\n";

        auto aliases = aliasesByBase.find( name );

        if( aliases != aliasesByBase.end() && !aliases->second.empty() )
        {
            out << "ALIAS";

            for( const std::string& alias : aliases->second )
            {
                out << " " << alias;
                ++aliasCount;
            }

            out << "\n";
        }

        out << "F0 " << legacyQuote( reference ) << " 0 0 50 H V C CNN\n";
        out << "F1 " << legacyQuote( name ) << " 0 -100 50 H V C CNN\n";
        writeLegacyLibraryStandardPropertyField( out, symbol, 2, "Footprint", true );
        writeLegacyLibraryStandardPropertyField( out, symbol, 3, "Datasheet", true );
        writeLegacyLibraryCustomPropertyFields( out, symbol );
        out << "DRAW\n";
        writeLegacyDrawItems( out, symbol, drawCount );
        writeLegacyPins( out, symbol, pinCount );
        out << "ENDDRAW\n";
        out << "ENDDEF\n";
        ++count;
    }

    out << "#\n";
    out << "#End Library\n";

    if( aWarnings )
    {
        if( count == 0 )
            aWarnings->push_back( "converted empty symbol library to legacy .lib" );
        else
            aWarnings->push_back( "converted symbol definitions to legacy .lib headers; drawing primitives and pins are lossy" );

        if( pinCount > 0 )
            aWarnings->push_back( "converted symbol pins to legacy X records" );

        if( drawCount > 0 )
            aWarnings->push_back( "converted symbol drawing primitives to legacy DRAW records" );

        if( aliasCount > 0 )
            aWarnings->push_back( "converted symbol aliases to legacy ALIAS records" );
    }

    return out.str();
}


std::string sexprProjectToLegacy( const DOCUMENT& aDocument, std::vector<std::string>* aWarnings )
{
    LEGACY_PROJECT_META meta;

    if( !aDocument.RawText.empty() )
    {
        if( aDocument.RawText.find( "\"project_settings\"" ) != std::string::npos )
        {
            const std::vector<std::string> settingKeys = {
                "update", "version", "last_client", "LibDir", "NetIExt", "CmpExt",
                "PageLayoutDescrFile", "PlotDirectoryName", "SubpartIdSeparator",
                "SubpartFirstId"
            };

            for( const std::string& key : settingKeys )
                meta.Settings[key] = jsonStringValue( aDocument.RawText, key );
        }

        meta.Libraries = jsonStringArray( aDocument.RawText, "legacy_symbol_libraries" );

        if( meta.Libraries.empty() )
            meta.Libraries = symbolLibraryNicknames( aDocument.Path );

        if( meta.Libraries.empty() )
            meta.Libraries = localSymbolLibraryStems( aDocument.Path );

        std::string legacyObject = jsonStringValue( aDocument.RawText, "legacy" );
        (void) legacyObject;
    }

    std::ostringstream out;
    out << "update=" << ( meta.Settings["update"].empty() ? "0" : meta.Settings["update"] ) << "\n";
    out << "version=" << ( meta.Settings["version"].empty() ? "1" : meta.Settings["version"] ) << "\n";
    out << "last_client="
        << ( meta.Settings["last_client"].empty() ? "kicad-backport" : meta.Settings["last_client"] ) << "\n";
    out << "LibDir=" << meta.Settings["LibDir"] << "\n";
    out << "NetIExt=" << ( meta.Settings["NetIExt"].empty() ? "net" : meta.Settings["NetIExt"] ) << "\n";
    out << "CmpExt=" << ( meta.Settings["CmpExt"].empty() ? ".cmp" : meta.Settings["CmpExt"] ) << "\n";
    out << "PageLayoutDescrFile=" << meta.Settings["PageLayoutDescrFile"] << "\n";
    out << "PlotDirectoryName=" << meta.Settings["PlotDirectoryName"] << "\n";
    out << "SubpartIdSeparator="
        << ( meta.Settings["SubpartIdSeparator"].empty() ? "0" : meta.Settings["SubpartIdSeparator"] ) << "\n";
    out << "SubpartFirstId="
        << ( meta.Settings["SubpartFirstId"].empty() ? "65" : meta.Settings["SubpartFirstId"] ) << "\n";

    for( size_t i = 0; i < meta.Libraries.size(); ++i )
        out << "LibName" << ( i + 1 ) << "=" << meta.Libraries[i] << "\n";

    if( aWarnings )
    {
        aWarnings->push_back( "converted project JSON to minimal legacy .pro settings" );

        if( !meta.Settings.empty() )
            aWarnings->push_back( "restored legacy project settings from JSON" );

        if( !meta.Libraries.empty() )
            aWarnings->push_back( "restored legacy project library names" );
    }

    (void) aDocument;
    return out.str();
}

} // namespace


bool IsLegacyKind( KIND aKind )
{
    return aKind == KIND::LEGACY_SCHEMATIC || aKind == KIND::LEGACY_SYMBOL_LIBRARY
           || aKind == KIND::LEGACY_SYMBOL_DOCUMENTATION || aKind == KIND::LEGACY_PROJECT;
}


KIND SexprKindForLegacyKind( KIND aKind )
{
    switch( aKind )
    {
    case KIND::LEGACY_SCHEMATIC: return KIND::SCHEMATIC;
    case KIND::LEGACY_SYMBOL_LIBRARY:
    case KIND::LEGACY_SYMBOL_DOCUMENTATION: return KIND::SYMBOL_LIBRARY;
    case KIND::LEGACY_PROJECT: return KIND::PROJECT;
    default: return KIND::UNKNOWN;
    }
}


KIND LegacyKindForSexprKind( KIND aKind )
{
    switch( aKind )
    {
    case KIND::SCHEMATIC: return KIND::LEGACY_SCHEMATIC;
    case KIND::SYMBOL_LIBRARY: return KIND::LEGACY_SYMBOL_LIBRARY;
    case KIND::PROJECT: return KIND::LEGACY_PROJECT;
    default: return KIND::UNKNOWN;
    }
}


int LegacyDocumentMajorVersion( const DOCUMENT& aDocument )
{
    switch( aDocument.Kind )
    {
    case KIND::LEGACY_SCHEMATIC:
        return parseSchematicHeaderVersion( aDocument.RawText ) >= 4 ? 5 : 4;
    case KIND::LEGACY_SYMBOL_LIBRARY:
        return parseLibraryHeaderVersion( aDocument.RawText ) >= "2.4" ? 5 : 4;
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
    case KIND::LEGACY_PROJECT:
        return 5;
    default:
        return 0;
    }
}


std::string LegacyTargetVersionForKind( KIND aKind, int aTargetMajor )
{
    switch( aKind )
    {
    case KIND::LEGACY_SCHEMATIC:
    case KIND::SCHEMATIC:
        return aTargetMajor <= 4 ? "legacy-sch-v2" : "legacy-sch-v4";
    case KIND::LEGACY_SYMBOL_LIBRARY:
    case KIND::SYMBOL_LIBRARY:
        return aTargetMajor <= 4 ? "legacy-lib-2.3" : "legacy-lib-2.4";
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
        return "legacy-dcm-2.0";
    case KIND::LEGACY_PROJECT:
    case KIND::PROJECT:
        return "legacy-pro";
    default:
        return std::string();
    }
}


DOCUMENT LoadLegacyDocument( const std::filesystem::path& aPath, std::string aText )
{
    DOCUMENT doc;
    doc.Path = aPath;
    doc.SourceBytes = aText.size();
    doc.RawText = std::move( aText );
    doc.Kind = DetectKind( aPath, std::string() );

    switch( doc.Kind )
    {
    case KIND::LEGACY_SCHEMATIC:
        doc.Version = LegacyTargetVersionForKind( doc.Kind, LegacyDocumentMajorVersion( doc ) );
        break;
    case KIND::LEGACY_SYMBOL_LIBRARY:
        doc.Version = LegacyTargetVersionForKind( doc.Kind, LegacyDocumentMajorVersion( doc ) );
        break;
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
        doc.Version = "legacy-dcm-2.0";
        break;
    case KIND::LEGACY_PROJECT:
        doc.Version = "legacy-pro";
        break;
    default:
        throw std::runtime_error( "not a KiCad legacy document" );
    }

    return doc;
}


std::string ConvertLegacyToSexprText( const DOCUMENT& aDocument,
                                      const std::string& aTargetVersion,
                                      KIND* aTargetKind,
                                      std::vector<std::string>* aWarnings )
{
    switch( aDocument.Kind )
    {
    case KIND::LEGACY_SCHEMATIC:
        if( aTargetKind )
            *aTargetKind = KIND::SCHEMATIC;
        return legacySchematicToSexpr( aDocument, aTargetVersion, aWarnings );
    case KIND::LEGACY_SYMBOL_LIBRARY:
        if( aTargetKind )
            *aTargetKind = KIND::SYMBOL_LIBRARY;
        return legacyLibraryToSexpr( aDocument, aTargetVersion, aWarnings );
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
        if( aTargetKind )
            *aTargetKind = KIND::SYMBOL_LIBRARY;
        return legacyDocumentationToSexpr( aDocument, aTargetVersion, aWarnings );
    case KIND::LEGACY_PROJECT:
        if( aTargetKind )
            *aTargetKind = KIND::PROJECT;
        return legacyProjectToJson( aDocument, aWarnings );
    default:
        throw std::runtime_error( "not a KiCad legacy document" );
    }
}


std::string ConvertSexprToLegacyText( const DOCUMENT& aDocument, int aTargetMajor,
                                      KIND* aTargetKind,
                                      std::vector<std::string>* aWarnings )
{
    switch( aDocument.Kind )
    {
    case KIND::SCHEMATIC:
        if( aTargetKind )
            *aTargetKind = KIND::LEGACY_SCHEMATIC;
        return sexprSchematicToLegacy( aDocument, aTargetMajor, aWarnings );
    case KIND::SYMBOL_LIBRARY:
        if( aTargetKind )
            *aTargetKind = KIND::LEGACY_SYMBOL_LIBRARY;
        return sexprSymbolLibraryToLegacy( aDocument, aTargetMajor, aWarnings );
    case KIND::PROJECT:
        if( aTargetKind )
            *aTargetKind = KIND::LEGACY_PROJECT;
        return sexprProjectToLegacy( aDocument, aWarnings );
    default:
        throw std::runtime_error( "legacy writer is not defined for this file type" );
    }
}


std::string RewriteLegacyTextForTarget( const DOCUMENT& aDocument, int aTargetMajor,
                                        std::vector<std::string>* aWarnings )
{
    std::vector<std::string> lines = linesOf( aDocument.RawText );
    bool changedHeader = false;

    switch( aDocument.Kind )
    {
    case KIND::LEGACY_SCHEMATIC:
    {
        std::string header = "EESchema Schematic File Version "
                             + std::string( aTargetMajor <= 4 ? "2" : "4" );

        for( std::string& line : lines )
        {
            if( StartsWith( line, "EESchema Schematic File Version" ) )
            {
                changedHeader = line != header;
                line = header;
                break;
            }
        }
        break;
    }

    case KIND::LEGACY_SYMBOL_LIBRARY:
    {
        std::string header = "EESchema-LIBRARY Version "
                             + std::string( aTargetMajor <= 4 ? "2.3" : "2.4" );

        for( std::string& line : lines )
        {
            if( StartsWith( line, "EESchema-LIBRARY Version" ) )
            {
                changedHeader = line != header;
                line = header;
                break;
            }
        }
        break;
    }

    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
    case KIND::LEGACY_PROJECT:
        break;

    default:
        throw std::runtime_error( "not a KiCad legacy document" );
    }

    std::ostringstream out;

    for( const std::string& line : lines )
        out << line << "\n";

    if( aWarnings )
    {
        if( changedHeader )
            aWarnings->push_back( "rewrote legacy file header for requested KiCad major version" );

        if( aDocument.Kind == KIND::LEGACY_SCHEMATIC || aDocument.Kind == KIND::LEGACY_SYMBOL_LIBRARY )
            aWarnings->push_back( "legacy V4/V5 rewrite preserves raw records but does not simplify version-specific features yet" );
    }

    return out.str();
}


std::string LegacyDocumentationSidecarText( const DOCUMENT& aDocument, int aTargetMajor,
                                            std::vector<std::string>* aWarnings )
{
    std::ostringstream out;
    out << "EESchema-DOCLIB  Version 2.0\n";

    int count = 0;

    if( aDocument.Kind == KIND::SYMBOL_LIBRARY && aDocument.Root )
    {
        for( SEXPR::NODE* symbol : topLevelSymbols( aDocument.Root.get() ) )
        {
            std::string name = sanitizeSymbolName( symbol->AtomAt( 1 ) );
            out << "$CMP " << name << "\n";
            out << "D " << propertyValue( symbol, "Description" ) << "\n";
            out << "K " << propertyValue( symbol, "ki_keywords" ) << "\n";
            out << "F " << propertyValue( symbol, "Datasheet" ) << "\n";
            out << "$ENDCMP\n";
            ++count;
        }
    }

    out << "#\n";
    out << "#End Doc Library\n";

    if( aWarnings )
    {
        if( count > 0 )
            aWarnings->push_back( "wrote legacy .dcm sidecar for symbol documentation properties" );
        else
            aWarnings->push_back( "wrote empty legacy .dcm sidecar" );
    }

    (void) aTargetMajor;
    return out.str();
}

} // namespace KICAD_BACKPORT
