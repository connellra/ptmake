#include "file.h"
#include "find.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>

extern const char *makefileNames[];
extern const unsigned int makefileNameCount;

std::string find_makefile( void )
{
    for( unsigned int i = 0; i < makefileNameCount; i ++ )
    {
        if( fileExists( makefileNames[ i ] ) ) {
            return makefileNames[ i ];
        }
    }

    throw std::runtime_error( "No makefile found" );
}
