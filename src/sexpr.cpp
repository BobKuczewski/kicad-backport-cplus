#include "kicad_backport/sexpr.h"

#include <cctype>
#include <stdexcept>


namespace SEXPR
{
namespace
{

struct TOKEN
{
    std::string Text;
    bool        Quoted = false;
};


bool isSpace( char aChar )
{
    return std::isspace( static_cast<unsigned char>( aChar ) ) != 0;
}


std::vector<TOKEN> tokenize( const std::string& aText )
{
    std::vector<TOKEN> tokens;

    for( size_t i = 0; i < aText.size(); )
    {
        // Accept UTF-8 BOM files produced by some Windows editors.
        if( i == 0 && aText.size() >= 3
            && static_cast<unsigned char>( aText[0] ) == 0xEF
            && static_cast<unsigned char>( aText[1] ) == 0xBB
            && static_cast<unsigned char>( aText[2] ) == 0xBF )
        {
            i = 3;
            continue;
        }

        if( isSpace( aText[i] ) )
        {
            ++i;
            continue;
        }

        if( aText[i] == '(' || aText[i] == ')' )
        {
            tokens.push_back( TOKEN{ std::string( 1, aText[i] ), false } );
            ++i;
            continue;
        }

        if( aText[i] == '"' )
        {
            ++i;
            std::string value;
            bool closed = false;

            while( i < aText.size() )
            {
                char ch = aText[i++];

                if( ch == '"' )
                {
                    closed = true;
                    break;
                }

                if( ch == '\\' )
                {
                    if( i >= aText.size() )
                        throw std::runtime_error( "unterminated escape in quoted string" );

                    char escaped = aText[i++];

                    switch( escaped )
                    {
                    case '\\': value.push_back( '\\' ); break;
                    case '"':  value.push_back( '"' );  break;
                    case 'n':  value.push_back( '\n' ); break;
                    case 't':  value.push_back( '\t' ); break;
                    default:   value.push_back( escaped ); break;
                    }

                    continue;
                }

                value.push_back( ch );
            }

            if( !closed )
                throw std::runtime_error( "unterminated quoted string" );

            tokens.push_back( TOKEN{ value, true } );
            continue;
        }

        size_t start = i;

        while( i < aText.size() && aText[i] != '(' && aText[i] != ')' && !isSpace( aText[i] ) )
            ++i;

        tokens.push_back( TOKEN{ aText.substr( start, i - start ), false } );
    }

    return tokens;
}


std::unique_ptr<NODE> parseList( const std::vector<TOKEN>& aTokens, size_t& aPos )
{
    // Recursive descent is enough because KiCad files are already fully parenthesized.
    std::unique_ptr<NODE> node = NODE::MakeList();

    while( aPos < aTokens.size() )
    {
        const TOKEN& token = aTokens[aPos++];

        if( token.Text == "(" )
        {
            node->Children.push_back( parseList( aTokens, aPos ) );
        }
        else if( token.Text == ")" )
        {
            return node;
        }
        else
        {
            node->Children.push_back( NODE::MakeAtom( token.Text, token.Quoted ) );
        }
    }

    throw std::runtime_error( "unexpected end of input: unclosed '('" );
}


bool needsQuotes( const std::string& aAtom )
{
    if( aAtom.empty() )
        return true;

    for( char ch : aAtom )
    {
        if( isSpace( ch ) || ch == '(' || ch == ')' || ch == '"' )
            return true;
    }

    return false;
}


std::string escapeAtom( const std::string& aAtom )
{
    std::string out;

    for( char ch : aAtom )
    {
        switch( ch )
        {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n";  break;
        case '\t': out += "\\t";  break;
        default:   out.push_back( ch ); break;
        }
    }

    return out;
}


std::string formatAtom( const NODE* aNode )
{
    if( aNode->Quoted || needsQuotes( aNode->Atom ) )
        return "\"" + escapeAtom( aNode->Atom ) + "\"";

    return aNode->Atom;
}


bool shouldInline( const NODE* aNode )
{
    // Keep short atom-only lists on one line to match KiCad's normal formatting.
    if( !aNode || aNode->Children.empty() )
        return true;

    size_t totalLen = 2;

    for( size_t i = 0; i < aNode->Children.size(); ++i )
    {
        const std::unique_ptr<NODE>& child = aNode->Children[i];

        if( !child || !child->IsAtom() )
            return false;

        if( i > 0 )
            ++totalLen;

        totalLen += formatAtom( child.get() ).size();
    }

    return totalLen <= 88;
}


void writeNode( std::string& aOut, const NODE* aNode, int aIndent )
{
    if( !aNode )
        return;

    if( aNode->IsAtom() )
    {
        aOut += formatAtom( aNode );
        return;
    }

    aOut.push_back( '(' );

    if( aNode->Children.empty() )
    {
        aOut.push_back( ')' );
        return;
    }

    if( shouldInline( aNode ) )
    {
        for( size_t i = 0; i < aNode->Children.size(); ++i )
        {
            if( i > 0 )
                aOut.push_back( ' ' );

            writeNode( aOut, aNode->Children[i].get(), aIndent );
        }

        aOut.push_back( ')' );
        return;
    }

    writeNode( aOut, aNode->Children[0].get(), aIndent + 2 );

    for( size_t i = 1; i < aNode->Children.size(); ++i )
    {
        aOut.push_back( '\n' );
        aOut.append( static_cast<size_t>( aIndent + 2 ), ' ' );
        writeNode( aOut, aNode->Children[i].get(), aIndent + 2 );
    }

    aOut.push_back( ')' );
}

} // namespace


std::unique_ptr<NODE> NODE::MakeAtom( const std::string& aValue, bool aQuoted )
{
    std::unique_ptr<NODE> node( new NODE );
    node->Atom = aValue;
    node->Quoted = aQuoted;
    return node;
}


std::unique_ptr<NODE> NODE::MakeList()
{
    return std::unique_ptr<NODE>( new NODE );
}


std::string NODE::Head() const
{
    if( Children.empty() || !Children[0] || !Children[0]->IsAtom() )
        return std::string();

    return Children[0]->Atom;
}


std::string NODE::AtomAt( size_t aIndex ) const
{
    if( aIndex >= Children.size() || !Children[aIndex] || !Children[aIndex]->IsAtom() )
        return std::string();

    return Children[aIndex]->Atom;
}


bool NODE::SetAtomAt( size_t aIndex, const std::string& aValue, bool aQuoted )
{
    if( aIndex >= Children.size() || !Children[aIndex] || !Children[aIndex]->IsAtom() )
        return false;

    Children[aIndex]->Atom = aValue;
    Children[aIndex]->Quoted = aQuoted;
    return true;
}


NODE* NODE::ChildList( const std::string& aHead )
{
    for( std::unique_ptr<NODE>& child : Children )
    {
        if( child && !child->IsAtom() && child->Head() == aHead )
            return child.get();
    }

    return nullptr;
}


std::vector<NODE*> NODE::ChildLists( const std::string& aHead )
{
    std::vector<NODE*> out;

    for( std::unique_ptr<NODE>& child : Children )
    {
        if( child && !child->IsAtom() && child->Head() == aHead )
            out.push_back( child.get() );
    }

    return out;
}


std::unique_ptr<NODE> Parse( const std::string& aText )
{
    std::vector<TOKEN> tokens = tokenize( aText );

    if( tokens.empty() || tokens[0].Text != "(" )
        throw std::runtime_error( "expected '('" );

    size_t pos = 1;
    std::unique_ptr<NODE> root = parseList( tokens, pos );

    if( pos != tokens.size() )
        throw std::runtime_error( "trailing tokens after root expression" );

    return root;
}


std::string Format( const NODE* aRoot )
{
    std::string out;
    writeNode( out, aRoot, 0 );
    out.push_back( '\n' );
    return out;
}

} // namespace SEXPR
