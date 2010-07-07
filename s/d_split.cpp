// d_split.cpp

/**
*    Copyright (C) 2008 10gen Inc.
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

#include "pch.h"
#include <map>
#include <string>

#include "../db/btree.h"
#include "../db/commands.h"
#include "../db/dbmessage.h"
#include "../db/jsobj.h"
#include "../db/query.h"
#include "../db/queryoptimizer.h"

namespace mongo {

    // TODO: Fold these checks into each command.
    static IndexDetails *cmdIndexDetailsForRange( const char *ns, string &errmsg, BSONObj &min, BSONObj &max, BSONObj &keyPattern ) {
        if ( ns[ 0 ] == '\0' || min.isEmpty() || max.isEmpty() ) {
            errmsg = "invalid command syntax (note: min and max are required)";
            return 0;
        }
        return indexDetailsForRange( ns, errmsg, min, max, keyPattern );
    }


    class CmdMedianKey : public Command {
    public:
        CmdMedianKey() : Command( "medianKey" ) {}
        virtual bool slaveOk() const { return true; }
        virtual LockType locktype() const { return READ; } 
        virtual void help( stringstream &help ) const {
            help << 
                "Internal command.\n"
                "example: { medianKey:\"blog.posts\", keyPattern:{x:1}, min:{x:10}, max:{x:55} }\n"
                "NOTE: This command may take a while to run";
        }
        bool run(const string& dbname, BSONObj& jsobj, string& errmsg, BSONObjBuilder& result, bool fromRepl ){
            const char *ns = jsobj.getStringField( "medianKey" );
            BSONObj min = jsobj.getObjectField( "min" );
            BSONObj max = jsobj.getObjectField( "max" );
            BSONObj keyPattern = jsobj.getObjectField( "keyPattern" );
            
            Client::Context ctx( ns );

            IndexDetails *id = cmdIndexDetailsForRange( ns, errmsg, min, max, keyPattern );
            if ( id == 0 )
                return false;

            Timer timer;
            int num = 0;
            NamespaceDetails *d = nsdetails(ns);
            int idxNo = d->idxNo(*id);
            for( BtreeCursor c( d, idxNo, *id, min, max, false, 1, false ); c.ok(); c.advance(), ++num );
            num /= 2;
            BtreeCursor c( d, idxNo, *id, min, max, false, 1, false );
            for( ; num; c.advance(), --num );

            ostringstream os;
            os << "Finding median for index: " << keyPattern << " between " << min << " and " << max;
            logIfSlow( timer , os.str() );

            if ( !c.ok() ) {
                errmsg = "no index entries in the specified range";
                return false;
            }

            result.append( "median", c.prettyKey( c.currKey() ) );
            return true;
        }
    } cmdMedianKey;

     class SplitVector : public Command {
     public:
        SplitVector() : Command( "splitVector" , false ){}
        virtual bool slaveOk() const { return false; }
        virtual LockType locktype() const { return READ; }
        virtual void help( stringstream &help ) const {
            help <<
                "Internal command.\n"
                "example: { splitVector : \"myLargeCollection\" , keyPattern : {x:1} , maxChunkSize : 200 }\n"
                "maxChunkSize unit in MBs\n"
                "NOTE: This command may take a while to run";
        }
        bool run(const string& dbname, BSONObj& jsobj, string& errmsg, BSONObjBuilder& result, bool fromRepl ){
            const char* ns = jsobj.getStringField( "splitVector" );
            BSONObj keyPattern = jsobj.getObjectField( "keyPattern" );

            long long maxChunkSize = 0;
            BSONElement maxSizeElem = jsobj[ "maxChunkSize" ];
            if ( ! maxSizeElem.eoo() ){
                maxChunkSize = maxSizeElem.numberLong() * 1<<20;
            } else {
                errmsg = "need to specify the desired max chunk size";
                return false;
            }
            
            Client::Context ctx( ns );

            const char* field = keyPattern.firstElement().fieldName();
            BSONObjBuilder minBuilder;
            minBuilder.appendMinKey( field );
            BSONObj min = minBuilder.obj();
            BSONObjBuilder maxBuilder;
            maxBuilder.appendMaxKey( field );
            BSONObj max = maxBuilder.obj();
            IndexDetails *idx = cmdIndexDetailsForRange( ns , errmsg , min , max , keyPattern );
            if ( idx == NULL ){
                errmsg = "couldn't find index over splitting key";
                return false;
            }

            NamespaceDetails *d = nsdetails( ns );
            BtreeCursor c( d , d->idxNo(*idx) , *idx , min , max , false , 1 );

            // We'll use the average object size and number of object to find approximately how many keys
            // each chunk should have. We'll split a little smaller than the specificied by 'maxSize'
            // assuming a recently sharded collectio is still going to grow.

            const long long dataSize = d->datasize;
            const long long recCount = d->nrecords;
            long long keyCount = 0;
            if (( dataSize > 0 ) && ( recCount > 0 )){
                const long long avgRecSize = dataSize / recCount;
                keyCount = 0.9 * maxChunkSize / avgRecSize;
            }

            // We traverse the index and add the keyCount-th key to the result vector. If that key
            // appeared in the vector before, we omit it. The assumption here is that all the 
            // instances of a key value live in the same chunk.

            Timer timer;
            long long currCount = 0;
            vector<BSONObj> splitKeys;
            BSONObj currKey;
            while ( c.ok() ){ 
                currCount++;
                if ( currCount > keyCount ){
                    if ( ! currKey.isEmpty() && (currKey.woCompare( c.currKey() ) == 0 ) ) 
                         continue;

                    currKey = c.currKey();
                    splitKeys.push_back( c.prettyKey( currKey ) );
                    currCount = 0;
                }
                c.advance();
            }

            ostringstream os;
            os << "Finding the split vector for " <<  ns << " over "<< keyPattern;
            logIfSlow( timer , os.str() );

            // Warning: we are sending back an array of keys but are currently limited to 
            // 4MB work of 'result' size. This should be okay for now.

            result.append( "splitKeys" , splitKeys );
            return true;

        }
    } cmdSplitVector;

}  // namespace mongo
