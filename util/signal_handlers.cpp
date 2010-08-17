// signal_handlers.cpp

/**
*    Copyright (C) 2010 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../pch.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#if !defined(_WIN32) && !defined(NOEXECINFO)
#include <execinfo.h>
#endif

#include "log.h"
#include "signal_handlers.h"

namespace mongo {

/*
 * WARNING: PLEASE READ BEFORE CHANGING THIS MODULE
 *
 * All code in this module should be singal-friendly. Before adding any system
 * call or other dependency, please make sure the latter still holds.
 *
 */

static int rawWrite( int fd , char* c , int size ){
    int toWrite = size;
    int writePos = 0;
    int wrote;
    while ( toWrite > 0 ){
        wrote = write( fd , &c[writePos] , toWrite );
        if ( wrote < 1 ) break;
        toWrite -= wrote;
        writePos += wrote;        
    }
    return writePos;
}

static int formattedWrite( int fd , const char* format, ... ){
    const int MAX_ENTRY = 256;
    static char entryBuf[MAX_ENTRY];
              
    va_list ap;
    va_start( ap , format );
    int entrySize = vsnprintf( entryBuf , MAX_ENTRY-1 , format , ap );
    if ( entrySize < 0 ){
        return -1;
    }

    if ( rawWrite( fd , entryBuf , entrySize ) < 0 ){
        return -1;
    }

    return 0;
}

static void formattedBacktrace( int fd ){
    int numFrames;
    const int MAX_DEPTH = 20;
    void* stackFrame[MAX_DEPTH];

#if !defined(_WIN32) && !defined(NOEXECINFO)

    numFrames = backtrace( stackFrame , 20 );
    for (int i = 0; i < numFrames; i++ ){
        formattedWrite( fd , "Frame %d: %p\n" , i , stackFrame[i] );
    }

    // TODO get the backtrace_symbols

#else

    formattedWrite( fd, "backtracing not implemented for this platform yet\n" );

#endif

}

void printStackAndExit( int signalNum ){
    int fd = Logstream::getLogDesc();
    if ( fd < 0 ){
        fd = STDOUT_FILENO;
    }

    formattedWrite( fd , "Received signal %d\n" , signalNum );
    formattedWrite( fd , "Backtrace:\n" );
    formattedBacktrace( fd );
    formattedWrite( fd , "===\n" );

    ::exit( EXIT_ABRUPT );
}

} // namespace mongo
