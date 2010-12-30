// @file file_allocator.h

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "../pch.h"

namespace mongo {
    
    /* 
     * Handles allocation of contiguous files on disk.  Allocation may be
     * requested asynchronously or synchronously.
     */
    class FileAllocator {
        /* 
         * The public functions may not be called concurrently.  The allocation
         * functions may be called multiple times per file, but only the first
         * size specified per file will be used.
        */
    public:
        FileAllocator();

        void start();
        
        /**
         * May be called if file exists. If file exists, or its allocation has
         *  been requested, size is updated to match existing file size.
         */
        void requestAllocation( const string &name, long &size );


        /**
         * Returns when file has been allocated.  If file exists, size is
         * updated to match existing file size.
         */
        void allocateAsap( const string &name, unsigned long long &size );

        void waitUntilFinished() const;
        
        static void ensureLength(int fd , long size);
        
    private:
#if !defined(_WIN32)
        void checkFailure();
        
        // caller must hold pendingMutex_ lock.  Returns size if allocated or 
        // allocation requested, -1 otherwise.
        long prevSize( const string &name ) const;
         
        // caller must hold pendingMutex_ lock.
        bool inProgress( const string &name ) const;
        
        /** called from the worked thread */
        static void run( FileAllocator * fa );

        mutable mongo::mutex pendingMutex_;
        mutable boost::condition pendingUpdated_;
        list< string > pending_;
        mutable map< string, long > pendingSize_;
        bool failed_;
        
#endif    
    };
    
    FileAllocator &theFileAllocator();
} // namespace mongo
