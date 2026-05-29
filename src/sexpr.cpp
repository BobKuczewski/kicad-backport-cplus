#include "kicad_backport/sexpr.h"

#include <cctype>
#include <cstdint>
#include <stdexcept>


namespace SEXPR
{
namespace
{

thread_local std::pmr::memory_resource* g_parseResource = nullptr;

struct NODE_ALLOCATION_HEADER
{
    std::pmr::memory_resource* Resource;
};


size_t alignUp( size_t aValue, size_t aAlignment )
{
    return ( aValue + aAlignment - 1 ) & ~( aAlignment - 1 );
}


constexpr uintptr_t GLOBAL_NODE_ALLOCATION = 1;


class PARSE_RESOURCE_SCOPE
{
public:
    explicit PARSE_RESOURCE_SCOPE( std::pmr::memory_resource* aResource ) :
            m_previous( g_parseResource )
    {
        g_parseResource = aResource;
    }

    ~PARSE_RESOURCE_SCOPE()
    {
        g_parseResource = m_previous;
    }

private:
    std::pmr::memory_resource* m_previous;
};


bool isSpace( char aChar )
{
    return std::isspace( static_cast<unsigned char>( aChar ) ) != 0;
}


class PARSER
{
public:
    explicit PARSER( const std::string& aText ) :
            m_text( aText )
    {
    }

    std::unique_ptr<NODE> ParseRoot()
    {
        skipBom();
        skipSpace();

        if( eof() || peek() != '(' )
            throw std::runtime_error( "expected '('" );

        ++m_pos;
        std::unique_ptr<NODE> root = parseList();
        skipSpace();

        if( !eof() )
            throw std::runtime_error( "trailing tokens after root expression" );

        return root;
    }

private:
    bool eof() const
    {
        return m_pos >= m_text.size();
    }

    char peek() const
    {
        return m_text[m_pos];
    }

    void skipBom()
    {
        if( m_pos == 0 && m_text.size() >= 3
            && static_cast<unsigned char>( m_text[0] ) == 0xEF
            && static_cast<unsigned char>( m_text[1] ) == 0xBB
            && static_cast<unsigned char>( m_text[2] ) == 0xBF )
        {
            m_pos = 3;
        }
    }

    void skipSpace()
    {
        while( !eof() && isSpace( peek() ) )
            ++m_pos;
    }

    std::unique_ptr<NODE> parseList()
    {
        std::unique_ptr<NODE> node = NODE::MakeList();
        node->Children.reserve( 8 );

        while( true )
        {
            skipSpace();

            if( eof() )
                throw std::runtime_error( "unexpected end of input: unclosed '('" );

            char ch = peek();

            if( ch == ')' )
            {
                ++m_pos;
                return node;
            }

            if( ch == '(' )
            {
                ++m_pos;
                node->Children.push_back( parseList() );
                continue;
            }

            if( ch == '"' )
            {
                node->Children.push_back( parseQuotedAtom() );
                continue;
            }

            node->Children.push_back( parseAtom() );
        }
    }

    std::unique_ptr<NODE> parseQuotedAtom()
    {
        ++m_pos;
        std::string value;
        value.reserve( 32 );

        while( !eof() )
        {
            char ch = m_text[m_pos++];

            if( ch == '"' )
                return NODE::MakeAtom( std::move( value ), true );

            if( ch == '\\' )
            {
                if( eof() )
                    throw std::runtime_error( "unterminated escape in quoted string" );

                char escaped = m_text[m_pos++];

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

        throw std::runtime_error( "unterminated quoted string" );
    }

    std::unique_ptr<NODE> parseAtom()
    {
        size_t start = m_pos;

        while( !eof() && peek() != '(' && peek() != ')' && !isSpace( peek() ) )
            ++m_pos;

        return NODE::MakeAtom( m_text.substr( start, m_pos - start ), false );
    }

    const std::string& m_text;
    size_t             m_pos = 0;
};


bool needsQuotes( std::string_view aAtom )
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


std::string escapeAtom( std::string_view aAtom )
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

    return std::string( aNode->Atom );
}


size_t escapedAtomLength( std::string_view aAtom )
{
    size_t len = 0;

    for( char ch : aAtom )
    {
        switch( ch )
        {
        case '\\':
        case '"':
        case '\n':
        case '\t':
            len += 2;
            break;
        default:
            ++len;
            break;
        }
    }

    return len;
}


size_t formattedAtomLength( const NODE* aNode )
{
    size_t len = escapedAtomLength( aNode->Atom );

    if( aNode->Quoted || needsQuotes( aNode->Atom ) )
        len += 2;

    return len;
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

        totalLen += formattedAtomLength( child.get() );
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


void* NODE::operator new( size_t aSize )
{
    const size_t headerSize = alignUp( sizeof( NODE_ALLOCATION_HEADER ), alignof( NODE ) );
    const size_t totalSize = headerSize + aSize;

    if( g_parseResource )
    {
        char* raw = static_cast<char*>( g_parseResource->allocate( totalSize, alignof( NODE ) ) );
        auto* header = reinterpret_cast<NODE_ALLOCATION_HEADER*>( raw );
        header->Resource = g_parseResource;
        return raw + headerSize;
    }

    char* raw = static_cast<char*>( ::operator new( totalSize ) );
    auto* header = reinterpret_cast<NODE_ALLOCATION_HEADER*>( raw );
    header->Resource = reinterpret_cast<std::pmr::memory_resource*>( GLOBAL_NODE_ALLOCATION );
    return raw + headerSize;
}


void NODE::operator delete( void* aPtr ) noexcept
{
    if( !aPtr )
        return;

    const size_t headerSize = alignUp( sizeof( NODE_ALLOCATION_HEADER ), alignof( NODE ) );
    char* raw = static_cast<char*>( aPtr ) - headerSize;
    auto* header = reinterpret_cast<NODE_ALLOCATION_HEADER*>( raw );

    if( reinterpret_cast<uintptr_t>( header->Resource ) == GLOBAL_NODE_ALLOCATION )
        ::operator delete( raw );
}


void NODE::operator delete( void* aPtr, size_t ) noexcept
{
    NODE::operator delete( aPtr );
}


NODE::NODE() :
        Atom( g_parseResource ? g_parseResource : std::pmr::get_default_resource() ),
        Children( g_parseResource ? g_parseResource : std::pmr::get_default_resource() )
{
}


std::unique_ptr<NODE> NODE::MakeAtom( const std::string& aValue, bool aQuoted )
{
    std::unique_ptr<NODE> node( new NODE );
    node->Atom = aValue;
    node->Quoted = aQuoted;
    return node;
}


std::unique_ptr<NODE> NODE::MakeAtom( std::string&& aValue, bool aQuoted )
{
    std::unique_ptr<NODE> node( new NODE );
    node->Atom = std::move( aValue );
    node->Quoted = aQuoted;
    return node;
}


std::unique_ptr<NODE> NODE::MakeList()
{
    return std::unique_ptr<NODE>( new NODE );
}


std::string NODE::Head() const
{
    return std::string( HeadView() );
}


std::string_view NODE::HeadView() const
{
    if( Children.empty() || !Children[0] || !Children[0]->IsAtom() )
        return {};

    return Children[0]->Atom;
}


std::string NODE::AtomAt( size_t aIndex ) const
{
    return std::string( AtomAtView( aIndex ) );
}


std::string_view NODE::AtomAtView( size_t aIndex ) const
{
    if( aIndex >= Children.size() || !Children[aIndex] || !Children[aIndex]->IsAtom() )
        return {};

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
        if( child && !child->IsAtom() && child->HeadView() == aHead )
            return child.get();
    }

    return nullptr;
}


std::vector<NODE*> NODE::ChildLists( const std::string& aHead )
{
    std::vector<NODE*> out;

    for( std::unique_ptr<NODE>& child : Children )
    {
        if( child && !child->IsAtom() && child->HeadView() == aHead )
            out.push_back( child.get() );
    }

    return out;
}


std::unique_ptr<NODE> Parse( const std::string& aText )
{
    auto arena = std::make_shared<std::pmr::monotonic_buffer_resource>();
    PARSE_RESOURCE_SCOPE scope( arena.get() );

    PARSER parser( aText );
    std::unique_ptr<NODE> root = parser.ParseRoot();
    root->ArenaOwner = arena;
    return root;
}


std::string Format( const NODE* aRoot )
{
    return Format( aRoot, 0 );
}


std::string Format( const NODE* aRoot, size_t aReserveBytes )
{
    std::string out;
    if( aReserveBytes > 0 )
        out.reserve( aReserveBytes );

    writeNode( out, aRoot, 0 );
    out.push_back( '\n' );
    return out;
}

} // namespace SEXPR
