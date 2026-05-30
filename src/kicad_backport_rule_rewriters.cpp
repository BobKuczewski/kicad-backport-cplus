#include "internal/kicad_backport_rule_rewriters.h"
#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>


namespace KICAD_BACKPORT::RULE_REWRITERS
{

bool parseKiCadBool( std::string_view aValue, bool aDefault );
std::unique_ptr<SEXPR::NODE> propertyNode( const std::string& aName, const std::string& aValue );
std::unique_ptr<SEXPR::NODE> pcbPropertyToLegacyFPText( std::unique_ptr<SEXPR::NODE> aProperty,
                                                        const std::string& aKind );

namespace
{

bool containsString( const std::set<std::string>& aSet, std::string_view aValue )
{
    for( const std::string& item : aSet )
    {
        if( item == aValue )
            return true;
    }

    return false;
}


size_t findRuleForHead( const std::vector<std::pair<std::string, size_t>>& aHeads,
                        std::string_view aHead )
{
    for( const auto& item : aHeads )
    {
        if( item.first == aHead )
            return item.second;
    }

    return static_cast<size_t>( -1 );
}


bool containsHead( const std::vector<std::pair<std::string, size_t>>& aHeads,
                   std::string_view aHead )
{
    return findRuleForHead( aHeads, aHead ) != static_cast<size_t>( -1 );
}


char asciiLower( char aChar )
{
    if( aChar >= 'A' && aChar <= 'Z' )
        return static_cast<char>( aChar - 'A' + 'a' );

    return aChar;
}


bool equalsNoCase( std::string_view aLeft, std::string_view aRight )
{
    if( aLeft.size() != aRight.size() )
        return false;

    for( size_t i = 0; i < aLeft.size(); ++i )
    {
        if( asciiLower( aLeft[i] ) != asciiLower( aRight[i] ) )
            return false;
    }

    return true;
}


double toDouble( std::string_view aValue, double aDefault = 0.0 )
{
    std::string text( aValue );
    char* end = nullptr;
    double value = std::strtod( text.c_str(), &end );

    if( end == text.c_str() || ( end && *end != '\0' ) )
        return aDefault;

    return value;
}


std::string formatDouble( double aValue )
{
    if( std::abs( aValue ) < 0.0000000005 )
        aValue = 0.0;

    std::ostringstream out;
    out << std::fixed << std::setprecision( 9 ) << aValue;
    std::string text = out.str();

    while( text.size() > 1 && text.back() == '0' )
        text.pop_back();

    if( !text.empty() && text.back() == '.' )
        text.pop_back();

    return text.empty() ? "0" : text;
}


std::unique_ptr<SEXPR::NODE> listNode( std::string aHead )
{
    std::unique_ptr<SEXPR::NODE> node = SEXPR::NODE::MakeList();
    node->Children.push_back( SEXPR::NODE::MakeAtom( std::move( aHead ) ) );
    return node;
}


std::unique_ptr<SEXPR::NODE> cloneAtomNode( const SEXPR::NODE* aNode )
{
    if( !aNode )
        return SEXPR::NODE::MakeAtom( "" );

    return SEXPR::NODE::MakeAtom( std::string( aNode->Atom ), aNode->Quoted );
}


int toInt( std::string_view aValue, int aDefault = 0 )
{
    int value = aDefault;
    auto result = std::from_chars( aValue.data(), aValue.data() + aValue.size(), value );

    if( result.ec != std::errc() || result.ptr != aValue.data() + aValue.size() )
        return aDefault;

    return value;
}


std::unique_ptr<SEXPR::NODE> copyDimensionLayer( SEXPR::NODE* aSource,
                                                 SEXPR::NODE* aText )
{
    const SEXPR::NODE* layer = aSource ? aSource->ChildList( "layer" ) : nullptr;

    if( !layer && aText )
        layer = aText->ChildList( "layer" );

    std::unique_ptr<SEXPR::NODE> out = listNode( "layer" );

    if( layer && layer->Children.size() > 1 && layer->Children[1] )
        out->Children.push_back( cloneAtomNode( layer->Children[1].get() ) );
    else
        out->Children.push_back( SEXPR::NODE::MakeAtom( "Dwgs.User", true ) );

    return out;
}


double childDouble( SEXPR::NODE* aNode, const std::string& aHead, double aDefault )
{
    const SEXPR::NODE* child = aNode ? aNode->ChildList( aHead ) : nullptr;
    return child ? toDouble( child->AtomAtView( 1 ), aDefault ) : aDefault;
}


bool dimensionPoints( SEXPR::NODE* aNode, std::array<std::array<double, 2>, 2>& aPoints )
{
    const SEXPR::NODE* pts = aNode ? aNode->ChildList( "pts" ) : nullptr;

    if( !pts )
        return false;

    size_t count = 0;

    for( const std::unique_ptr<SEXPR::NODE>& child : pts->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "xy" )
            continue;

        if( count < 2 )
        {
            aPoints[count][0] = toDouble( child->AtomAtView( 1 ) );
            aPoints[count][1] = toDouble( child->AtomAtView( 2 ) );
        }

        ++count;

        if( count >= 2 )
            return true;
    }

    return false;
}


std::unique_ptr<SEXPR::NODE> graphicLine( const std::array<double, 2>& aStart,
                                          const std::array<double, 2>& aEnd,
                                          std::unique_ptr<SEXPR::NODE> aLayer,
                                          double aWidth )
{
    std::unique_ptr<SEXPR::NODE> line = listNode( "gr_line" );

    std::unique_ptr<SEXPR::NODE> start = listNode( "start" );
    start->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aStart[0] ) ) );
    start->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aStart[1] ) ) );

    std::unique_ptr<SEXPR::NODE> end = listNode( "end" );
    end->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aEnd[0] ) ) );
    end->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aEnd[1] ) ) );

    std::unique_ptr<SEXPR::NODE> stroke = listNode( "stroke" );
    std::unique_ptr<SEXPR::NODE> width = listNode( "width" );
    width->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aWidth ) ) );
    std::unique_ptr<SEXPR::NODE> type = listNode( "type" );
    type->Children.push_back( SEXPR::NODE::MakeAtom( "default" ) );
    stroke->Children.push_back( std::move( width ) );
    stroke->Children.push_back( std::move( type ) );

    line->Children.push_back( std::move( start ) );
    line->Children.push_back( std::move( end ) );
    line->Children.push_back( std::move( stroke ) );
    line->Children.push_back( std::move( aLayer ) );
    return line;
}


std::array<double, 2> addScaled( const std::array<double, 2>& aPoint,
                                 const std::array<double, 2>& aVector,
                                 double aScale )
{
    return { aPoint[0] + aVector[0] * aScale, aPoint[1] + aVector[1] * aScale };
}


std::vector<std::unique_ptr<SEXPR::NODE>> arrowLines(
        const std::array<double, 2>& aOrigin,
        const std::array<double, 2>& aDirection,
        const std::array<double, 2>& aPerpendicular,
        SEXPR::NODE* aSource,
        SEXPR::NODE* aText,
        double aWidth,
        double aArrowLength )
{
    constexpr double pi = 3.14159265358979323846;
    double arrowAngle = 27.5 * pi / 180.0;
    double arrowAlong = aArrowLength * std::cos( arrowAngle );
    double arrowSide = aArrowLength * std::sin( arrowAngle );

    auto endpoint = [&]( double aSide )
    {
        return std::array<double, 2>{
            aOrigin[0] + aDirection[0] * arrowAlong + aPerpendicular[0] * aSide,
            aOrigin[1] + aDirection[1] * arrowAlong + aPerpendicular[1] * aSide
        };
    };

    std::vector<std::unique_ptr<SEXPR::NODE>> lines;
    lines.push_back( graphicLine( aOrigin, endpoint( arrowSide ),
                                  copyDimensionLayer( aSource, aText ), aWidth ) );
    lines.push_back( graphicLine( aOrigin, endpoint( -arrowSide ),
                                  copyDimensionLayer( aSource, aText ), aWidth ) );
    return lines;
}


std::unique_ptr<SEXPR::NODE> takeChildByHead( SEXPR::NODE* aNode, const std::string& aHead )
{
    if( !aNode || aNode->IsAtom() )
        return nullptr;

    for( size_t i = 0; i < aNode->Children.size(); ++i )
    {
        std::unique_ptr<SEXPR::NODE>& child = aNode->Children[i];

        if( child && !child->IsAtom() && child->HeadView() == aHead )
        {
            std::unique_ptr<SEXPR::NODE> out = std::move( child );
            aNode->Children.erase( aNode->Children.begin() + i );
            return out;
        }
    }

    return nullptr;
}


void appendMoved( std::vector<std::unique_ptr<SEXPR::NODE>>& aTarget,
                  std::vector<std::unique_ptr<SEXPR::NODE>> aSource )
{
    for( std::unique_ptr<SEXPR::NODE>& node : aSource )
        aTarget.push_back( std::move( node ) );
}


std::vector<std::unique_ptr<SEXPR::NODE>> graphicsFromModernDimension(
        std::unique_ptr<SEXPR::NODE> aDimension )
{
    std::vector<std::unique_ptr<SEXPR::NODE>> graphics;

    if( !aDimension )
        return graphics;

    std::array<std::array<double, 2>, 2> points{};

    if( !dimensionPoints( aDimension.get(), points ) )
        return graphics;

    SEXPR::NODE* textRaw = aDimension->ChildList( "gr_text" );

    if( !textRaw )
        return graphics;

    SEXPR::NODE* style = aDimension->ChildList( "style" );
    double thickness = childDouble( style, "thickness", 0.15 );
    double arrowLength = childDouble( style, "arrow_length", 1.27 );
    double extensionHeight = childDouble( style, "extension_height", 0.58642 );
    double height = childDouble( aDimension.get(), "height", 0.0 );
    std::string dimType = aDimension->ChildList( "type" )
            ? aDimension->ChildList( "type" )->AtomAt( 1 ) : "aligned";

    auto line = [&]( const std::array<double, 2>& aStart, const std::array<double, 2>& aEnd )
    {
        return graphicLine( aStart, aEnd, copyDimensionLayer( aDimension.get(), textRaw ),
                            thickness );
    };

    std::unique_ptr<SEXPR::NODE> text = takeChildByHead( aDimension.get(), "gr_text" );

    if( dimType == "radial" || dimType == "leader" )
    {
        graphics.push_back( std::move( text ) );
        graphics.push_back( line( points[0], points[1] ) );
        return graphics;
    }

    if( dimType == "orthogonal" )
    {
        int orientation = aDimension->ChildList( "orientation" )
                ? toInt( aDimension->ChildList( "orientation" )->AtomAtView( 1 ) ) : 0;
        double heightSign = height < 0.0 ? -1.0 : 1.0;
        std::array<double, 2> crossbar1{};
        std::array<double, 2> crossbar2{};
        std::array<double, 2> feature1End{};
        std::array<double, 2> feature2End{};
        std::array<double, 2> direction1{};
        std::array<double, 2> perpendicular{};

        if( orientation == 1 )
        {
            crossbar1 = { points[0][0] + height, points[0][1] };
            crossbar2 = { points[0][0] + height, points[1][1] };
            feature1End = { points[0][0] + height + extensionHeight * heightSign, points[0][1] };
            feature2End = { points[0][0] + height + extensionHeight * heightSign, points[1][1] };
            direction1 = { 0.0, points[1][1] < points[0][1] ? -1.0 : 1.0 };
            perpendicular = { 1.0, 0.0 };
        }
        else
        {
            crossbar1 = { points[0][0], points[0][1] + height };
            crossbar2 = { points[1][0], points[0][1] + height };
            feature1End = { points[0][0], points[0][1] + height + extensionHeight * heightSign };
            feature2End = { points[1][0], points[0][1] + height + extensionHeight * heightSign };
            direction1 = { points[1][0] > points[0][0] ? 1.0 : -1.0, 0.0 };
            perpendicular = { 0.0, 1.0 };
        }

        std::array<double, 2> direction2{ -direction1[0], -direction1[1] };
        graphics.push_back( std::move( text ) );
        graphics.push_back( line( points[0], feature1End ) );
        graphics.push_back( line( points[1], feature2End ) );
        graphics.push_back( line( crossbar1, crossbar2 ) );
        appendMoved( graphics, arrowLines( crossbar1, direction1, perpendicular, aDimension.get(),
                                           textRaw, thickness, arrowLength ) );
        appendMoved( graphics, arrowLines( crossbar2, direction2, perpendicular, aDimension.get(),
                                           textRaw, thickness, arrowLength ) );
        return graphics;
    }

    double dx = points[1][0] - points[0][0];
    double dy = points[1][1] - points[0][1];
    double length = std::hypot( dx, dy );

    graphics.push_back( std::move( text ) );

    if( length <= 0.0 )
        return graphics;

    std::array<double, 2> unit{ dx / length, dy / length };
    std::array<double, 2> perpendicular{ -unit[1], unit[0] };
    double heightSign = height < 0.0 ? -1.0 : 1.0;
    std::array<double, 2> feature1End = addScaled( points[0], perpendicular,
                                                   height + extensionHeight * heightSign );
    std::array<double, 2> feature2End = addScaled( points[1], perpendicular,
                                                   height + extensionHeight * heightSign );
    std::array<double, 2> crossbar1 = addScaled( points[0], perpendicular, height );
    std::array<double, 2> crossbar2 = addScaled( points[1], perpendicular, height );

    graphics.push_back( line( points[0], feature1End ) );
    graphics.push_back( line( points[1], feature2End ) );
    graphics.push_back( line( crossbar1, crossbar2 ) );
    appendMoved( graphics, arrowLines( crossbar1, unit, perpendicular, aDimension.get(), textRaw,
                                       thickness, arrowLength ) );
    appendMoved( graphics, arrowLines( crossbar2, { -unit[0], -unit[1] }, perpendicular,
                                       aDimension.get(), textRaw, thickness, arrowLength ) );
    return graphics;
}

} // namespace

// Removes any subtree whose head token is not accepted by the target parser.
int removeDescendantsByHead( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    bool directRemoval = false;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() && containsString( aHeads, child->HeadView() ) )
        {
            ++removed;
            directRemoval = true;
            continue;
        }

        removed += removeDescendantsByHead( child.get(), aHeads );
    }

    if( directRemoval )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
        kept.reserve( aRoot->Children.size() );

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && containsString( aHeads, child->HeadView() ) )
                continue;

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    return removed;
}


int removeDescendantsByRuleHeads( SEXPR::NODE* aRoot,
                                  const std::vector<std::pair<std::string, size_t>>& aHeadToRule,
                                  std::vector<int>& aCounts )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    bool directRemoval = false;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() )
        {
            size_t ruleIndex = findRuleForHead( aHeadToRule, child->HeadView() );

            if( ruleIndex != static_cast<size_t>( -1 ) )
            {
                ++removed;
                ++aCounts[ruleIndex];
                directRemoval = true;
                continue;
            }
        }

        removed += removeDescendantsByRuleHeads( child.get(), aHeadToRule, aCounts );
    }

    if( directRemoval )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
        kept.reserve( aRoot->Children.size() );

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && containsHead( aHeadToRule, child->HeadView() ) )
                continue;

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    return removed;
}


int removeChildrenFromParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                               const std::set<std::string>& aChildren )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        bool directRemoval = false;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && containsString( aChildren, child->HeadView() ) )
            {
                ++removed;
                directRemoval = true;
            }
        }

        if( directRemoval )
        {
            std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
            kept.reserve( aRoot->Children.size() );

            for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
            {
                if( child && !child->IsAtom() && containsString( aChildren, child->HeadView() ) )
                    continue;

                kept.push_back( std::move( child ) );
            }

            aRoot->Children = std::move( kept );
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        removed += removeChildrenFromParents( child.get(), aParents, aChildren );

    return removed;
}


std::vector<int> removeChildrenFromParentsBatch( SEXPR::NODE* aRoot,
                                                 const std::vector<CHILD_REMOVAL_RULE>& aRules )
{
    std::vector<int> counts( aRules.size(), 0 );

    if( !aRoot || aRoot->IsAtom() || aRules.empty() )
        return counts;

    auto visit = [&]( auto&& self, SEXPR::NODE* node ) -> void
    {
        if( !node || node->IsAtom() )
            return;

        std::vector<size_t> active;
        std::string_view head = node->HeadView();

        for( size_t i = 0; i < aRules.size(); ++i )
        {
            if( containsString( aRules[i].Parents, head ) )
                active.push_back( i );
        }

        bool directRemoval = false;

        if( !active.empty() )
        {
            for( std::unique_ptr<SEXPR::NODE>& child : node->Children )
            {
                if( !child || child->IsAtom() )
                    continue;

                std::string_view childHead = child->HeadView();

                for( size_t ruleIndex : active )
                {
                    if( containsString( aRules[ruleIndex].Children, childHead ) )
                    {
                        ++counts[ruleIndex];
                        directRemoval = true;
                        break;
                    }
                }
            }
        }

        if( directRemoval )
        {
            std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( node->Children.get_allocator() );
            kept.reserve( node->Children.size() );

            for( std::unique_ptr<SEXPR::NODE>& child : node->Children )
            {
                bool remove = false;

                if( child && !child->IsAtom() )
                {
                    std::string_view childHead = child->HeadView();

                    for( size_t ruleIndex : active )
                    {
                        if( containsString( aRules[ruleIndex].Children, childHead ) )
                        {
                            remove = true;
                            break;
                        }
                    }
                }

                if( !remove )
                    kept.push_back( std::move( child ) );
            }

            node->Children = std::move( kept );
        }

        for( std::unique_ptr<SEXPR::NODE>& child : node->Children )
            self( self, child.get() );
    };

    visit( visit, aRoot );
    return counts;
}


BOARD_FAST_COUNTS applyBoardFastVisitor( SEXPR::NODE* aRoot, const BOARD_FAST_OPTIONS& aOptions,
                                         const std::vector<CHILD_REMOVAL_RULE>& aChildRemovalRules )
{
    BOARD_FAST_COUNTS counts;
    counts.ChildRemovalRules.assign( aChildRemovalRules.size(), 0 );

    if( !aRoot || aRoot->IsAtom() )
        return counts;

    static const std::set<std::string> shapeHeads = {
        "gr_rect", "gr_circle", "gr_poly", "fp_rect", "fp_circle", "fp_poly"
    };

    static const std::set<std::string> tstampParents = {
        "footprint", "module", "pad", "via", "segment", "arc", "zone",
        "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly", "gr_curve", "gr_text",
        "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly", "fp_curve", "fp_text"
    };

    auto visit = [&]( auto&& self, SEXPR::NODE* node ) -> void
    {
        if( !node || node->IsAtom() )
            return;

        std::string_view head = node->HeadView();
        std::array<size_t, 32> activeChildRemovalRules;
        size_t activeChildRemovalRuleCount = 0;
        std::vector<size_t> overflowActiveChildRemovalRules;

        if( !aChildRemovalRules.empty() )
        {
            for( size_t i = 0; i < aChildRemovalRules.size(); ++i )
            {
                if( containsString( aChildRemovalRules[i].Parents, head ) )
                {
                    if( activeChildRemovalRuleCount < activeChildRemovalRules.size() )
                        activeChildRemovalRules[activeChildRemovalRuleCount++] = i;
                    else
                        overflowActiveChildRemovalRules.push_back( i );
                }
            }
        }

        if( aOptions.PlotParamBools && head == "pcbplotparams" )
        {
            for( std::unique_ptr<SEXPR::NODE>& child : node->Children )
            {
                if( !child || child->IsAtom() || child->Children.size() < 2 )
                    continue;

                std::string_view value = child->AtomAtView( 1 );

                if( equalsNoCase( value, "yes" ) && child->SetAtomAt( 1, "true", false ) )
                    ++counts.PlotParamBools;
                else if( equalsNoCase( value, "no" ) && child->SetAtomAt( 1, "false", false ) )
                    ++counts.PlotParamBools;
            }
        }

        if( aOptions.UserLayerTypes && head == "layers" )
        {
            for( std::unique_ptr<SEXPR::NODE>& layer : node->Children )
            {
                if( !layer || layer->IsAtom() || layer->Children.size() < 4 )
                    continue;

                std::string_view name = layer->AtomAtView( 1 );
                std::string_view type = layer->AtomAtView( 2 );

                if( name.size() >= 5 && name.substr( 0, 5 ) == "User."
                    && ( type == "front" || type == "back" || type == "auxiliary" )
                    && layer->SetAtomAt( 2, "user", false ) )
                {
                    ++counts.UserLayerTypes;
                }
            }
        }

        if( ( aOptions.ShapeFillNoToNone || aOptions.ShapeHatchFills )
            && containsString( shapeHeads, head ) )
        {
            SEXPR::NODE* fill = node->ChildList( "fill" );

            if( fill )
            {
                std::string_view value = fill->AtomAtView( 1 );

                if( aOptions.ShapeFillNoToNone && equalsNoCase( value, "no" )
                    && fill->SetAtomAt( 1, "none", false ) )
                {
                    ++counts.ShapeFillNoToNone;
                    value = fill->AtomAtView( 1 );
                }

                if( aOptions.ShapeHatchFills
                    && ( value == "hatch" || value == "reverse_hatch" || value == "cross_hatch" )
                    && fill->SetAtomAt( 1, "yes", false ) )
                {
                    ++counts.ShapeHatchFills;
                }
            }
        }

        if( aOptions.ZoneFilledAreasThickness && head == "zone"
            && node->ChildList( "filled_polygon" ) && !node->ChildList( "filled_areas_thickness" ) )
        {
            std::unique_ptr<SEXPR::NODE> thickness = SEXPR::NODE::MakeList();
            thickness->Children.push_back( SEXPR::NODE::MakeAtom( "filled_areas_thickness" ) );
            thickness->Children.push_back( SEXPR::NODE::MakeAtom( "no" ) );

            size_t insertAt = node->Children.size();

            for( size_t i = 1; i < node->Children.size(); ++i )
            {
                if( node->Children[i] && !node->Children[i]->IsAtom()
                    && node->Children[i]->HeadView() == "fill" )
                {
                    insertAt = i;
                    break;
                }
            }

            node->Children.insert( node->Children.begin() + insertAt, std::move( thickness ) );
            ++counts.ZoneFilledAreasThickness;
        }

        bool rebuilding = false;
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( node->Children.get_allocator() );

        auto beginRebuild = [&]( size_t aCurrentIndex )
        {
            if( rebuilding )
                return;

            kept.reserve( node->Children.size() );

            for( size_t j = 0; j < aCurrentIndex; ++j )
                kept.push_back( std::move( node->Children[j] ) );

            rebuilding = true;
        };

        auto renameUuidChildrenToTstamp = [&]( SEXPR::NODE* aNode )
        {
            if( !aOptions.RenameUuidToTstamp || !aNode || aNode->IsAtom() )
                return;

            for( std::unique_ptr<SEXPR::NODE>& child : aNode->Children )
            {
                if( child && !child->IsAtom() && child->HeadView() == "uuid"
                    && child->SetAtomAt( 0, "tstamp", false ) )
                {
                    ++counts.RenamedUuidToTstamp;
                }
            }
        };

        for( size_t i = 0; i < node->Children.size(); ++i )
        {
            std::unique_ptr<SEXPR::NODE>& child = node->Children[i];

            if( !child )
                continue;

            if( child->IsAtom() )
            {
                if( aOptions.ReplaceThievingMode && head == "mode" && child->Atom == "thieving" )
                {
                    child->Atom = "polygon";
                    ++counts.ReplacedThievingMode;
                }

                if( aOptions.AttrDnpAtoms && head == "attr" && i > 0 && child->Atom == "dnp" )
                {
                    ++counts.RemovedAttrDnpAtoms;
                    beginRebuild( i );
                    continue;
                }

                if( rebuilding )
                    kept.push_back( std::move( child ) );

                continue;
            }

            std::string_view childHead = child->HeadView();

            if( aOptions.RemoveRootGeneratorVersion && node == aRoot
                && childHead == "generator_version" )
            {
                ++counts.RemovedRootGeneratorVersion;
                beginRebuild( i );
                continue;
            }

            if( activeChildRemovalRuleCount > 0 || !overflowActiveChildRemovalRules.empty() )
            {
                bool removeChild = false;

                for( size_t activeIndex = 0; activeIndex < activeChildRemovalRuleCount; ++activeIndex )
                {
                    size_t ruleIndex = activeChildRemovalRules[activeIndex];

                    if( containsString( aChildRemovalRules[ruleIndex].Children, childHead ) )
                    {
                        ++counts.ChildRemovalRules[ruleIndex];
                        removeChild = true;
                        break;
                    }
                }

                if( !removeChild )
                {
                    for( size_t ruleIndex : overflowActiveChildRemovalRules )
                    {
                        if( containsString( aChildRemovalRules[ruleIndex].Children, childHead ) )
                        {
                            ++counts.ChildRemovalRules[ruleIndex];
                            removeChild = true;
                            break;
                        }
                    }
                }

                if( removeChild )
                {
                    beginRebuild( i );
                    continue;
                }
            }

            if( aOptions.RemoveTypedModels && childHead == "model"
                && child->ChildList( "type" ) )
            {
                ++counts.RemovedTypedModels;
                beginRebuild( i );
                continue;
            }

            if( childHead == "uuid" )
            {
                if( aOptions.RenameUuidToTstamp && containsString( tstampParents, head )
                    && child->SetAtomAt( 0, "tstamp", false ) )
                {
                    ++counts.RenamedUuidToTstamp;
                    childHead = child->HeadView();
                }
                else if( aOptions.RenameGroupGeneratedUuidToId
                         && ( head == "group" || head == "generated" )
                         && child->SetAtomAt( 0, "id", false ) )
                {
                    ++counts.RenamedGroupGeneratedUuidToId;
                    childHead = child->HeadView();
                }
            }

            if( aOptions.DowngradePCBFootprintFields
                && ( head == "footprint" || head == "module" ) )
            {
                if( childHead == "property" )
                {
                    self( self, child.get() );

                    if( child->AtomAtView( 1 ) == "Reference" )
                    {
                        beginRebuild( i );
                        std::unique_ptr<SEXPR::NODE> text =
                                pcbPropertyToLegacyFPText( std::move( child ), "reference" );
                        renameUuidChildrenToTstamp( text.get() );
                        kept.push_back( std::move( text ) );
                        ++counts.PCBFootprintFields;
                        continue;
                    }

                    if( child->AtomAtView( 1 ) == "Value" )
                    {
                        beginRebuild( i );
                        std::unique_ptr<SEXPR::NODE> text =
                                pcbPropertyToLegacyFPText( std::move( child ), "value" );
                        renameUuidChildrenToTstamp( text.get() );
                        kept.push_back( std::move( text ) );
                        ++counts.PCBFootprintFields;
                        continue;
                    }

                    if( child->AtomAtView( 1 ) == "Description"
                        && child->SetAtomAt( 1, "ki_description", true ) )
                    {
                        ++counts.PCBFootprintFields;
                    }

                    if( child->Children.size() > 3 )
                    {
                        child->Children.resize( 3 );
                        ++counts.PCBFootprintFields;
                    }

                    if( rebuilding )
                        kept.push_back( std::move( child ) );

                    continue;
                }

                if( childHead == "sheetname" )
                {
                    if( !child->AtomAtView( 1 ).empty() )
                    {
                        beginRebuild( i );
                        kept.push_back( propertyNode( "Sheetname", child->AtomAt( 1 ) ) );
                        ++counts.PCBFootprintFields;
                    }
                    else
                    {
                        beginRebuild( i );
                    }

                    continue;
                }

                if( childHead == "sheetfile" )
                {
                    if( !child->AtomAtView( 1 ).empty() )
                    {
                        beginRebuild( i );
                        kept.push_back( propertyNode( "Sheetfile", child->AtomAt( 1 ) ) );
                        ++counts.PCBFootprintFields;
                    }
                    else
                    {
                        beginRebuild( i );
                    }

                    continue;
                }
            }

            if( aOptions.RemoveUnlocked && childHead == "unlocked" )
            {
                ++counts.RemovedUnlocked;
                beginRebuild( i );
                continue;
            }

            if( aOptions.RemoveLocked && childHead == "locked" )
            {
                if( aOptions.BoardPresenceBoolFields && child->Children.size() > 1 )
                {
                    bool enabled = parseKiCadBool( child->AtomAtView( 1 ), true );
                    ++counts.BoardPresenceBoolFields;

                    if( enabled )
                        ++counts.RemovedLocked;
                }
                else
                {
                    ++counts.RemovedLocked;
                }

                beginRebuild( i );
                continue;
            }

            if( aOptions.BoardPresenceBoolFields
                && ( childHead == "locked" || childHead == "hide" ) && child->Children.size() > 1 )
            {
                bool enabled = parseKiCadBool( child->AtomAtView( 1 ), true );
                ++counts.BoardPresenceBoolFields;

                if( enabled )
                {
                    child->Children.resize( 1 );

                    if( rebuilding )
                        kept.push_back( std::move( child ) );
                }
                else
                {
                    beginRebuild( i );
                }

                continue;
            }

            if( aOptions.FreeViaPresence && childHead == "free" && child->Children.size() > 1 )
            {
                bool enabled = parseKiCadBool( child->AtomAtView( 1 ), true );
                ++counts.FreeViaPresence;

                if( enabled )
                {
                    child->Children.resize( 1 );

                    if( rebuilding )
                        kept.push_back( std::move( child ) );
                }
                else
                {
                    beginRebuild( i );
                }

                continue;
            }

            if( aOptions.DimensionBoolFields
                && ( childHead == "suppress_zeroes" || childHead == "keep_text_aligned" ) )
            {
                bool enabled = parseKiCadBool( child->AtomAtView( 1 ), true );
                ++counts.DimensionBoolFields;
                beginRebuild( i );

                if( enabled )
                    kept.push_back( SEXPR::NODE::MakeAtom( std::string( childHead ) ) );

                continue;
            }

            if( aOptions.FontStyleLists && head == "font"
                && ( childHead == "bold" || childHead == "italic" ) && child->Children.size() > 1 )
            {
                bool enabled = parseKiCadBool( child->AtomAtView( 1 ), true );
                ++counts.FontStyleLists;
                beginRebuild( i );

                if( enabled )
                    kept.push_back( SEXPR::NODE::MakeAtom( std::string( childHead ) ) );

                continue;
            }

            self( self, child.get() );

            if( rebuilding )
                kept.push_back( std::move( child ) );
        }

        if( rebuilding )
            node->Children = std::move( kept );
    };

    visit( visit, aRoot );
    return counts;
}


bool hasChild( SEXPR::NODE* aNode, const std::string& aHead )
{
    return aNode && aNode->ChildList( aHead ) != nullptr;
}


bool parseKiCadBool( std::string_view aValue, bool aDefault )
{
    if( equalsNoCase( aValue, "yes" ) || equalsNoCase( aValue, "true" ) || aValue == "1" )
        return true;

    if( equalsNoCase( aValue, "no" ) || equalsNoCase( aValue, "false" ) || aValue == "0" )
        return false;

    return aDefault;
}


bool isIntegerAtom( std::string_view aValue )
{
    if( aValue.empty() )
        return false;

    size_t pos = 0;

    if( aValue[0] == '-' || aValue[0] == '+' )
        pos = 1;

    if( pos == aValue.size() )
        return false;

    for( ; pos < aValue.size(); ++pos )
    {
        if( !std::isdigit( static_cast<unsigned char>( aValue[pos] ) ) )
            return false;
    }

    return true;
}


int atomToInt( std::string_view aValue, int aDefault = 0 )
{
    int value = aDefault;
    const char* first = aValue.data();
    const char* last = first + aValue.size();
    auto result = std::from_chars( first, last, value );

    if( result.ec != std::errc() || result.ptr != last )
        return aDefault;

    return value;
}


std::unique_ptr<SEXPR::NODE> propertyNode( const std::string& aName, const std::string& aValue )
{
    std::unique_ptr<SEXPR::NODE> node = SEXPR::NODE::MakeList();
    node->Children.push_back( SEXPR::NODE::MakeAtom( "property" ) );
    node->Children.push_back( SEXPR::NODE::MakeAtom( aName, true ) );
    node->Children.push_back( SEXPR::NODE::MakeAtom( aValue, true ) );
    return node;
}


bool isStandardSchProperty( const std::string& aName )
{
    static const std::set<std::string> standard = {
        "Reference", "Value", "Footprint", "Datasheet", "ki_keywords", "ki_fp_filters"
    };

    return standard.count( aName ) != 0;
}


// Older schematic formats require explicit ids on non-standard fields.
int ensureLegacyPropertyIds( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "symbol" || aRoot->HeadView() == "sheet" )
    {
        int nextId = 5;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->HeadView() != "property" )
                continue;

            if( isStandardSchProperty( child->AtomAt( 1 ) ) || child->ChildList( "id" ) )
                continue;

            std::unique_ptr<SEXPR::NODE> id = SEXPR::NODE::MakeList();
            id->Children.push_back( SEXPR::NODE::MakeAtom( "id" ) );
            id->Children.push_back( SEXPR::NODE::MakeAtom( std::to_string( nextId++ ) ) );

            size_t insertAt = std::min<size_t>( 3, child->Children.size() );
            child->Children.insert( child->Children.begin() + insertAt, std::move( id ) );
            ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += ensureLegacyPropertyIds( child.get() );

    return changed;
}


// KiCad 8/7 store property visibility inside the effects list.
int movePropertyHideToEffects( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "property" )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
        bool hidden = false;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && child->IsAtom() && child->Atom == "hide" )
            {
                hidden = true;
                ++changed;
                continue;
            }

            if( child && !child->IsAtom() && child->HeadView() == "hide" )
            {
                hidden = parseKiCadBool( child->AtomAt( 1 ), true );
                ++changed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );

        if( hidden )
        {
            SEXPR::NODE* effects = aRoot->ChildList( "effects" );

            if( effects && !effects->ChildList( "hide" ) )
            {
                effects->Children.push_back( SEXPR::NODE::MakeAtom( "hide" ) );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += movePropertyHideToEffects( child.get() );

    return changed;
}


int removeDirectChildrenByHead( SEXPR::NODE* aRoot, const std::string& aHead )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

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


int renameChildHeadInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                              const std::string& aFrom, const std::string& aTo )
{
    // Used for UUID-to-tstamp/id compatibility without touching unrelated UUIDs.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && child->HeadView() == aFrom && child->SetAtomAt( 0, aTo, false ) )
                ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += renameChildHeadInParents( child.get(), aParents, aFrom, aTo );

    return changed;
}


int removeAtomsFromHeadedLists( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                const std::set<std::string>& aAtoms )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

        for( size_t i = 0; i < aRoot->Children.size(); ++i )
        {
            std::unique_ptr<SEXPR::NODE>& child = aRoot->Children[i];

            if( i > 0 && child && child->IsAtom() && containsString( aAtoms, child->Atom ) )
            {
                ++removed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        removed += removeAtomsFromHeadedLists( child.get(), aParents, aAtoms );

    return removed;
}


int flattenChildListsToAtomsInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                       const std::set<std::string>& aChildren )
{
    // Older KiCad often stores flags as bare atoms instead of boolean child lists.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && containsString( aChildren, child->HeadView() ) )
            {
                child = SEXPR::NODE::MakeAtom( std::string( child->HeadView() ) );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += flattenChildListsToAtomsInParents( child.get(), aParents, aChildren );

    return changed;
}


bool tentingBool( SEXPR::NODE* aNode, bool* aSeen )
{
    if( !aNode )
    {
        *aSeen = false;
        return false;
    }

    *aSeen = true;
    std::string value = Lower( aNode->AtomAt( 1 ) );
    return value.empty() || value == "yes" || value == "true" || value == "1";
}


int downgradeTentingToLegacyAtoms( SEXPR::NODE* aRoot )
{
    // Preserve front/back tenting intent using the older atom list form.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "tenting" )
    {
        bool sawFront = false;
        bool sawBack = false;
        bool front = tentingBool( aRoot->ChildList( "front" ), &sawFront );
        bool back = tentingBool( aRoot->ChildList( "back" ), &sawBack );

        if( sawFront || sawBack )
        {
            std::pmr::vector<std::unique_ptr<SEXPR::NODE>> children( aRoot->Children.get_allocator() );
            children.push_back( SEXPR::NODE::MakeAtom( "tenting" ) );

            if( front )
                children.push_back( SEXPR::NODE::MakeAtom( "front" ) );

            if( back )
                children.push_back( SEXPR::NODE::MakeAtom( "back" ) );

            if( children.size() == 1 )
                children.push_back( SEXPR::NODE::MakeAtom( "none" ) );

            aRoot->Children = std::move( children );
            return 1;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradeTentingToLegacyAtoms( child.get() );

    return changed;
}


// New boolean list forms become presence atoms in older file formats.
int downgradeBooleanPresenceNodes( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;
    std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() && containsString( aHeads, child->HeadView() ) && child->Children.size() > 1 )
        {
            std::string value = Lower( child->AtomAt( 1 ) );

            if( value == "yes" || value == "true" || value == "1" )
            {
                child->Children.resize( 1 );
                ++changed;
            }
            else if( value == "no" || value == "false" || value == "0" )
            {
                ++changed;
                continue;
            }
        }

        changed += downgradeBooleanPresenceNodes( child.get(), aHeads );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


int downgradeFontStyleListsToAtoms( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "font" )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom()
                && ( child->HeadView() == "bold" || child->HeadView() == "italic" )
                && child->Children.size() > 1 )
            {
                std::string head( child->HeadView() );
                bool enabled = parseKiCadBool( child->AtomAt( 1 ), true );

                if( enabled )
                    kept.push_back( SEXPR::NODE::MakeAtom( head ) );

                ++changed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradeFontStyleListsToAtoms( child.get() );

    return changed;
}


int downgradeBoolListsToAtoms( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;
    std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() && containsString( aHeads, child->HeadView() ) )
        {
            bool enabled = parseKiCadBool( child->AtomAt( 1 ), true );

            if( enabled )
                kept.push_back( SEXPR::NODE::MakeAtom( std::string( child->HeadView() ) ) );

            ++changed;
            continue;
        }

        changed += downgradeBoolListsToAtoms( child.get(), aHeads );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


// KiCad 7 stores reference/value as fp_text instead of footprint properties.
std::unique_ptr<SEXPR::NODE> pcbPropertyToLegacyFPText( std::unique_ptr<SEXPR::NODE> aProperty,
                                                        const std::string& aKind )
{
    std::unique_ptr<SEXPR::NODE> text = SEXPR::NODE::MakeList();
    text->Children.push_back( SEXPR::NODE::MakeAtom( "fp_text" ) );
    text->Children.push_back( SEXPR::NODE::MakeAtom( aKind ) );
    text->Children.push_back( SEXPR::NODE::MakeAtom( aProperty->AtomAt( 2 ), true ) );

    for( size_t i = 3; i < aProperty->Children.size(); ++i )
    {
        std::unique_ptr<SEXPR::NODE>& child = aProperty->Children[i];

        if( !child )
            continue;

        if( !child->IsAtom() && child->HeadView() == "hide" )
        {
            if( parseKiCadBool( child->AtomAt( 1 ), true ) )
                text->Children.push_back( SEXPR::NODE::MakeAtom( "hide" ) );

            continue;
        }

        text->Children.push_back( std::move( child ) );
    }

    return text;
}


int downgradePCBFootprintFields( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "footprint" || aRoot->HeadView() == "module" )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child )
                continue;

            if( !child->IsAtom() )
            {
                if( child->HeadView() == "property" )
                {
                    if( child->AtomAt( 1 ) == "Reference" )
                    {
                        kept.push_back( pcbPropertyToLegacyFPText( std::move( child ), "reference" ) );
                        ++changed;
                        continue;
                    }

                    if( child->AtomAt( 1 ) == "Value" )
                    {
                        kept.push_back( pcbPropertyToLegacyFPText( std::move( child ), "value" ) );
                        ++changed;
                        continue;
                    }

                    if( child->AtomAt( 1 ) == "Description" && child->SetAtomAt( 1, "ki_description", true ) )
                        ++changed;

                    if( child->Children.size() > 3 )
                    {
                        child->Children.resize( 3 );
                        ++changed;
                    }
                }
                else if( child->HeadView() == "sheetname" )
                {
                    if( !child->AtomAt( 1 ).empty() )
                    {
                        kept.push_back( propertyNode( "Sheetname", child->AtomAt( 1 ) ) );
                        ++changed;
                    }

                    continue;
                }
                else if( child->HeadView() == "sheetfile" )
                {
                    if( !child->AtomAt( 1 ).empty() )
                    {
                        kept.push_back( propertyNode( "Sheetfile", child->AtomAt( 1 ) ) );
                        ++changed;
                    }

                    continue;
                }
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBFootprintFields( child.get() );

    return changed;
}


int downgradeUserLayerTypes( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "layers" )
    {
        for( std::unique_ptr<SEXPR::NODE>& layer : aRoot->Children )
        {
            if( !layer || layer->IsAtom() || layer->Children.size() < 4 )
                continue;

            std::string name = layer->AtomAt( 1 );
            std::string type = layer->AtomAt( 2 );

            if( StartsWith( name, "User." )
                && ( type == "front" || type == "back" || type == "auxiliary" ) )
            {
                layer->SetAtomAt( 2, "user", false );
                ++changed;
            }
        }

        return changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradeUserLayerTypes( child.get() );

    return changed;
}


int downgradePCBPlotParamsBoolsToTrueFalse( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "pcbplotparams" )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->Children.size() < 2 )
                continue;

            std::string value = Lower( child->AtomAt( 1 ) );

            if( value == "yes" && child->SetAtomAt( 1, "true", false ) )
                ++changed;
            else if( value == "no" && child->SetAtomAt( 1, "false", false ) )
                ++changed;
        }

        return changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBPlotParamsBoolsToTrueFalse( child.get() );

    return changed;
}


int downgradePCBShapeFillNoToNone( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    static const std::set<std::string> shapeHeads = {
        "gr_rect", "gr_circle", "gr_poly", "fp_rect", "fp_circle", "fp_poly"
    };

    int changed = 0;

    if( containsString( shapeHeads, aRoot->HeadView() ) )
    {
        SEXPR::NODE* fill = aRoot->ChildList( "fill" );

        if( fill && Lower( fill->AtomAt( 1 ) ) == "no" && fill->SetAtomAt( 1, "none", false ) )
            ++changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBShapeFillNoToNone( child.get() );

    return changed;
}


int downgradePCBShapeHatchFills( SEXPR::NODE* aRoot )
{
    // KiCad versions without hatch fill support still display/manufacture solid fills.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    static const std::set<std::string> shapeHeads = {
        "gr_rect", "gr_circle", "gr_poly", "fp_rect", "fp_circle", "fp_poly"
    };

    int changed = 0;

    if( containsString( shapeHeads, aRoot->HeadView() ) )
    {
        SEXPR::NODE* fill = aRoot->ChildList( "fill" );

        if( fill )
        {
            std::string value = fill->AtomAt( 1 );

            if( value == "hatch" || value == "reverse_hatch" || value == "cross_hatch" )
            {
                fill->SetAtomAt( 1, "yes", false );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBShapeHatchFills( child.get() );

    return changed;
}


int ensureZoneFilledAreasThickness( SEXPR::NODE* aRoot )
{
    // KiCad 6+ uses this flag to distinguish polygon fill caches from old stroked fills.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "zone" && aRoot->ChildList( "filled_polygon" )
        && !aRoot->ChildList( "filled_areas_thickness" ) )
    {
        std::unique_ptr<SEXPR::NODE> thickness = SEXPR::NODE::MakeList();
        thickness->Children.push_back( SEXPR::NODE::MakeAtom( "filled_areas_thickness" ) );
        thickness->Children.push_back( SEXPR::NODE::MakeAtom( "no" ) );

        size_t insertAt = aRoot->Children.size();

        for( size_t i = 1; i < aRoot->Children.size(); ++i )
        {
            if( aRoot->Children[i] && !aRoot->Children[i]->IsAtom()
                && aRoot->Children[i]->HeadView() == "fill" )
            {
                insertAt = i;
                break;
            }
        }

        aRoot->Children.insert( aRoot->Children.begin() + insertAt, std::move( thickness ) );
        ++changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += ensureZoneFilledAreasThickness( child.get() );

    return changed;
}


// Dimensions cannot be represented in KiCad 7 PCB syntax, so keep visible text.
std::unique_ptr<SEXPR::NODE> legacyTextFromDimension( std::unique_ptr<SEXPR::NODE> aDimension,
                                                       bool aInsideFootprint )
{
    SEXPR::NODE* source = aDimension ? aDimension->ChildList( "gr_text" ) : nullptr;

    if( !source )
        return nullptr;

    std::unique_ptr<SEXPR::NODE> text = SEXPR::NODE::MakeList();

    if( aInsideFootprint )
    {
        text->Children.push_back( SEXPR::NODE::MakeAtom( "fp_text" ) );
        text->Children.push_back( SEXPR::NODE::MakeAtom( "user" ) );
        text->Children.push_back( SEXPR::NODE::MakeAtom( source->AtomAt( 1 ), true ) );
    }
    else
    {
        text->Children.push_back( SEXPR::NODE::MakeAtom( "gr_text" ) );
        text->Children.push_back( SEXPR::NODE::MakeAtom( source->AtomAt( 1 ), true ) );
    }

    for( size_t i = 2; i < source->Children.size(); ++i )
        text->Children.push_back( std::move( source->Children[i] ) );

    return text;
}


int downgradeDimensionsToTextImpl( SEXPR::NODE* aRoot, bool aInsideFootprint )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    bool insideFootprint = aInsideFootprint || aRoot->HeadView() == "footprint" || aRoot->HeadView() == "module";
    int changed = 0;
    std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "dimension" )
        {
            std::unique_ptr<SEXPR::NODE> text = legacyTextFromDimension( std::move( child ),
                                                                         insideFootprint );

            if( text )
                kept.push_back( std::move( text ) );

            ++changed;
            continue;
        }

        changed += downgradeDimensionsToTextImpl( child.get(), insideFootprint );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


int downgradeDimensionsToText( SEXPR::NODE* aRoot )
{
    return downgradeDimensionsToTextImpl( aRoot, false );
}


int downgradeDimensionsToGraphicsImpl( SEXPR::NODE* aRoot,
                                       std::vector<std::unique_ptr<SEXPR::NODE>>& aGraphics )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;
    std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
    kept.reserve( aRoot->Children.size() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == "dimension" )
        {
            appendMoved( aGraphics, graphicsFromModernDimension( std::move( child ) ) );
            ++changed;
            continue;
        }

        changed += downgradeDimensionsToGraphicsImpl( child.get(), aGraphics );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


int downgradeDimensionsToGraphics( SEXPR::NODE* aRoot )
{
    std::vector<std::unique_ptr<SEXPR::NODE>> graphics;
    int changed = downgradeDimensionsToGraphicsImpl( aRoot, graphics );

    if( aRoot && !aRoot->IsAtom() && aRoot->HeadView() == "kicad_pcb" )
    {
        for( std::unique_ptr<SEXPR::NODE>& graphic : graphics )
            aRoot->Children.push_back( std::move( graphic ) );
    }

    return changed;
}


int removeNodesContainingChild( SEXPR::NODE* aRoot, const std::string& aParentHead,
                                const std::string& aChildHead )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == aParentHead && hasChild( child.get(), aChildHead ) )
        {
            ++removed;
            continue;
        }

        removed += removeNodesContainingChild( child.get(), aParentHead, aChildHead );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return removed;
}


int replaceAtomValuesInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                const std::string& aFrom, const std::string& aTo )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && child->IsAtom() && std::string_view( child->Atom ) == aFrom )
            {
                child->Atom = aTo;
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += replaceAtomValuesInParents( child.get(), aParents, aFrom, aTo );

    return changed;
}


std::string zoneNetName( SEXPR::NODE* aZone )
{
    if( !aZone )
        return std::string();

    SEXPR::NODE* net = aZone->ChildList( "net" );

    if( !net || isIntegerAtom( net->AtomAt( 1 ) ) )
        return std::string();

    return net->AtomAt( 1 );
}


void collectBoardNetRefs( SEXPR::NODE* aRoot, std::unordered_map<std::string, int>& aCodes, int& aNext )
{
    if( !aRoot || aRoot->IsAtom() )
        return;

    if( aRoot->HeadView() == "net" && !isIntegerAtom( aRoot->AtomAtView( 1 ) ) )
    {
        std::string name = aRoot->AtomAt( 1 );

        if( !aCodes.count( name ) )
            aCodes[name] = aNext++;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        collectBoardNetRefs( child.get(), aCodes, aNext );
}


std::unordered_map<std::string, int> collectBoardNetCodes( SEXPR::NODE* aRoot )
{
    std::unordered_map<std::string, int> codes;
    codes.reserve( 128 );
    codes[""] = 0;
    int next = 1;

    if( aRoot )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->HeadView() != "net" )
                continue;

            if( isIntegerAtom( child->AtomAtView( 1 ) ) )
            {
                int code = atomToInt( child->AtomAtView( 1 ) );
                std::string name = child->AtomAt( 2 );

                if( !codes.count( name ) )
                    codes[name] = code;

                if( code >= next )
                    next = code + 1;
            }
            else
            {
                std::string name = child->AtomAt( 1 );

                if( !codes.count( name ) )
                    codes[name] = next++;
            }
        }
    }

    collectBoardNetRefs( aRoot, codes, next );
    return codes;
}


int rewriteBoardNetNamesToCodes( SEXPR::NODE* aRoot, const std::string& aParentHead,
                                 const std::unordered_map<std::string, int>& aCodes )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "net" && !isIntegerAtom( aRoot->AtomAtView( 1 ) ) )
    {
        std::string name = aRoot->AtomAt( 1 );
        int code = 0;
        auto it = aCodes.find( name );

        if( it != aCodes.end() )
            code = it->second;

        std::unique_ptr<SEXPR::NODE> head = std::move( aRoot->Children[0] );
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> children( aRoot->Children.get_allocator() );
        children.push_back( std::move( head ) );
        children.push_back( SEXPR::NODE::MakeAtom( std::to_string( code ) ) );

        if( aParentHead == "kicad_pcb" || aParentHead == "pad" )
        {
            for( size_t i = 1; i < aRoot->Children.size(); ++i )
                children.push_back( std::move( aRoot->Children[i] ) );
        }

        aRoot->Children = std::move( children );
        ++changed;
    }

    if( aRoot->HeadView() == "zone" )
    {
        std::string name = zoneNetName( aRoot );

        if( !name.empty() && !aRoot->ChildList( "net_name" ) )
        {
            std::unique_ptr<SEXPR::NODE> netName = SEXPR::NODE::MakeList();
            netName->Children.push_back( SEXPR::NODE::MakeAtom( "net_name" ) );
            netName->Children.push_back( SEXPR::NODE::MakeAtom( name, true ) );
            aRoot->Children.push_back( std::move( netName ) );
            ++changed;
        }
    }

    std::string parent = aRoot->Head();

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += rewriteBoardNetNamesToCodes( child.get(), parent, aCodes );

    return changed;
}


int firstBoardItemIndex( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    static const std::set<std::string> itemHeads = {
        "arc", "dimension", "footprint", "gr_arc", "gr_circle", "gr_curve", "gr_line",
        "gr_poly", "gr_rect", "gr_text", "group", "image", "segment", "via", "zone"
    };

    for( size_t i = 0; i < aRoot->Children.size(); ++i )
    {
        SEXPR::NODE* child = aRoot->Children[i].get();

        if( child && !child->IsAtom() && containsString( itemHeads, child->HeadView() ) )
            return static_cast<int>( i );
    }

    return static_cast<int>( aRoot->Children.size() );
}


int ensureRootBoardNetCodeTable( SEXPR::NODE* aRoot, const std::unordered_map<std::string, int>& aCodes )
{
    // Legacy boards require root-level numeric net declarations before items reference them.
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "kicad_pcb" )
        return 0;

    std::set<int> existingCodes;
    int lastRootNet = -1;

    for( size_t i = 0; i < aRoot->Children.size(); ++i )
    {
        SEXPR::NODE* child = aRoot->Children[i].get();

        if( !child || child->IsAtom() || child->HeadView() != "net" )
            continue;

        if( isIntegerAtom( child->AtomAtView( 1 ) ) )
            existingCodes.insert( atomToInt( child->AtomAtView( 1 ) ) );

        lastRootNet = static_cast<int>( i );
    }

    std::vector<std::pair<std::string, int>> entries;

    for( const auto& item : aCodes )
    {
        if( !existingCodes.count( item.second ) )
            entries.push_back( item );
    }

    std::sort( entries.begin(), entries.end(),
            []( const auto& a, const auto& b )
            {
                if( a.second == b.second )
                    return a.first < b.first;

                return a.second < b.second;
            } );

    if( entries.empty() )
        return 0;

    size_t insertAt = lastRootNet >= 0 ? static_cast<size_t>( lastRootNet + 1 )
                                      : static_cast<size_t>( firstBoardItemIndex( aRoot ) );

    std::vector<std::unique_ptr<SEXPR::NODE>> nodes;

    for( const auto& entry : entries )
    {
        std::unique_ptr<SEXPR::NODE> net = SEXPR::NODE::MakeList();
        net->Children.push_back( SEXPR::NODE::MakeAtom( "net" ) );
        net->Children.push_back( SEXPR::NODE::MakeAtom( std::to_string( entry.second ) ) );
        net->Children.push_back( SEXPR::NODE::MakeAtom( entry.first, true ) );
        nodes.push_back( std::move( net ) );
    }

    aRoot->Children.insert( aRoot->Children.begin() + insertAt,
                            std::make_move_iterator( nodes.begin() ),
                            std::make_move_iterator( nodes.end() ) );

    return static_cast<int>( entries.size() );
}


// Rebuilds legacy numeric net ids while keeping net names where required.
int downgradeBoardNetNamesToCodes( SEXPR::NODE* aRoot )
{
    std::unordered_map<std::string, int> codes = collectBoardNetCodes( aRoot );
    int changed = rewriteBoardNetNamesToCodes( aRoot, std::string(), codes );
    changed += ensureRootBoardNetCodeTable( aRoot, codes );
    return changed;
}


std::vector<std::string> removeIntroduced( SEXPR::NODE* aRoot, int aTarget,
                                           const std::vector<FEATURE_RULE>& aRules )
{
    // Feature gates are deliberately conservative: remove only when the target parser cannot read it.
    std::vector<std::string> warnings;
    std::vector<std::pair<std::string, size_t>> headToRule;
    std::vector<int> counts( aRules.size(), 0 );

    for( size_t i = 0; i < aRules.size(); ++i )
    {
        const FEATURE_RULE& rule = aRules[i];

        if( aTarget >= rule.MinVersion )
            continue;

        for( const std::string& head : rule.Heads )
            headToRule.emplace_back( head, i );
    }

    if( headToRule.empty() )
        return warnings;

    removeDescendantsByRuleHeads( aRoot, headToRule, counts );

    for( size_t i = 0; i < aRules.size(); ++i )
    {
        const FEATURE_RULE& rule = aRules[i];

        if( aTarget >= rule.MinVersion )
            continue;

        if( counts[i] > 0 )
        {
            std::ostringstream msg;
            msg << "removed " << counts[i] << " node(s) introduced in " << rule.MinVersion
                << ": " << rule.Reason;
            warnings.push_back( msg.str() );
        }
    }

    return warnings;
}


} // namespace KICAD_BACKPORT::RULE_REWRITERS
