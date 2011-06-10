// dbcommands_generic.cpp

/**
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

/**
 * commands suited for any mongo server
 */

#include "pch.h"
#include "query.h"
#include "pdfile.h"
#include "jsobj.h"
#include "../bson/util/builder.h"
#include <time.h>
#include "introspect.h"
#include "btree.h"
#include "../util/lruishmap.h"
#include "../util/md5.hpp"
#include "../util/processinfo.h"
#include "json.h"
#include "repl.h"
#include "repl_block.h"
#include "replutil.h"
#include "repl/rs.h"
#include "commands.h"
#include "db.h"
#include "instance.h"
#include "lasterror.h"
#include "security.h"
#include "../scripting/engine.h"
#include "stats/counters.h"
#include "background.h"
#include "../util/version.h"
#include "../util/ramlog.h"

namespace mongo {

    class CmdBuildInfo : public Command {
    public:
        CmdBuildInfo() : Command( "buildInfo", true, "buildinfo" ) {}
        virtual bool slaveOk() const { return true; }
        virtual bool adminOnly() const { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual void help( stringstream &help ) const {
            help << "get version #, etc.\n";
            help << "{ buildinfo:1 }";
        }
        bool run(const string& dbname, BSONObj& jsobj, string& errmsg, BSONObjBuilder& result, bool fromRepl ) {
            result << "version" << versionString << "gitVersion" << gitVersion() << "sysInfo" << sysInfo();
            result << "versionArray" << versionArray;
            result << "bits" << ( sizeof( int* ) == 4 ? 32 : 64 );
            result.appendBool( "debug" , debug );
            result.appendNumber("maxBsonObjectSize", BSONObjMaxUserSize);
            return true;
        }
    } cmdBuildInfo;

    /** experimental. either remove or add support in repl sets also.  in a repl set, getting this setting from the
        repl set config could make sense.
        */
    unsigned replApplyBatchSize = 1;

    class CmdGet : public Command {
    public:
        CmdGet() : Command( "getParameter" ) { }
        virtual bool slaveOk() const { return true; }
        virtual bool adminOnly() const { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual void help( stringstream &help ) const {
            help << "get administrative option(s)\nexample:\n";
            help << "{ getParameter:1, notablescan:1 }\n";
            help << "supported so far:\n";
            help << "  quiet\n";
            help << "  notablescan\n";
            help << "  logLevel\n";
            help << "  syncdelay\n";
            help << "{ getParameter:'*' } to get everything\n";
        }
        bool run(const string& dbname, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl ) {
            bool all = *cmdObj.firstElement().valuestrsafe() == '*';

            int before = result.len();

            if( all || cmdObj.hasElement("quiet") ) {
                result.append("quiet", cmdLine.quiet );
            }
            if( all || cmdObj.hasElement("notablescan") ) {
                result.append("notablescan", cmdLine.noTableScan);
            }
            if( all || cmdObj.hasElement("logLevel") ) {
                result.append("logLevel", logLevel);
            }
            if( all || cmdObj.hasElement("syncdelay") ) {
                result.append("syncdelay", cmdLine.syncdelay);
            }
            if( all || cmdObj.hasElement("replApplyBatchSize") ) {
                result.append("replApplyBatchSize", replApplyBatchSize);
            }

            if ( before == result.len() ) {
                errmsg = "no option found to get";
                return false;
            }
            return true;
        }
    } cmdGet;

    // dev - experimental. so only in set command for now.  may go away or change
    namespace dur { 
        int groupCommitIntervalMs = 100;
    }

    class CmdSet : public Command {
    public:
        CmdSet() : Command( "setParameter" ) { }
        virtual bool slaveOk() const { return true; }
        virtual bool adminOnly() const { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual void help( stringstream &help ) const {
            help << "set administrative option(s)\n";
            help << "{ setParameter:1, <param>:<value> }\n";
            help << "supported so far:\n";
            help << "  notablescan\n";
            help << "  logLevel\n";
            help << "  quiet\n";
            help << "  syncdelay\n";
        }
        bool run(const string& dbname, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl ) {
            int s = 0;
            if( cmdObj.hasElement("groupCommitIntervalMs") ) { 
                if( !cmdLine.dur ) { 
                    errmsg = "journaling is off";
                    return false;
                }
                int x = (int) cmdObj["groupCommitIntervalMs"].Number();
                assert( x > 0 && x < 500 );
                dur::groupCommitIntervalMs = x;
                log() << "groupCommitIntervalMs " << x << endl;
                s++;
            }
            if( cmdObj.hasElement("notablescan") ) {
                assert( !cmdLine.isMongos() );
                if( s == 0 )
                    result.append("was", cmdLine.noTableScan);
                cmdLine.noTableScan = cmdObj["notablescan"].Bool();
                s++;
            }
            if( cmdObj.hasElement("quiet") ) {
                if( s == 0 )
                    result.append("was", cmdLine.quiet );
                cmdLine.quiet = cmdObj["quiet"].Bool();
                s++;
            }
            if( cmdObj.hasElement("syncdelay") ) {
                assert( !cmdLine.isMongos() );
                if( s == 0 )
                    result.append("was", cmdLine.syncdelay );
                cmdLine.syncdelay = cmdObj["syncdelay"].Number();
                s++;
            }
            if( cmdObj.hasElement( "logLevel" ) ) {
                if( s == 0 )
                    result.append("was", logLevel );
                logLevel = cmdObj["logLevel"].numberInt();
                s++;
            }
            if( cmdObj.hasElement( "replApplyBatchSize" ) ) {
                if( s == 0 )
                    result.append("was", replApplyBatchSize );
                BSONElement e = cmdObj["replApplyBatchSize"];
                ParameterValidator * v = ParameterValidator::get( e.fieldName() );
                assert( v );
                if ( ! v->isValid( e , errmsg ) )
                    return false;
                replApplyBatchSize = e.numberInt();
                s++;
            }

            if( s == 0 ) {
                errmsg = "no option found to set, use help:true to see options ";
                return false;
            }

            return true;
        }
    } cmdSet;

    class PingCommand : public Command {
    public:
        PingCommand() : Command( "ping" ) {}
        virtual bool slaveOk() const { return true; }
        virtual void help( stringstream &help ) const { help << "a way to check that the server is alive. responds immediately even if server is in a db lock."; }
        virtual LockType locktype() const { return NONE; }
        virtual bool requiresAuth() { return false; }
        virtual bool run(const string& badns, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool) {
            // IMPORTANT: Don't put anything in here that might lock db - including authentication
            return true;
        }
    } pingCmd;

    class FeaturesCmd : public Command {
    public:
        FeaturesCmd() : Command( "features", true ) {}
        void help(stringstream& h) const { h << "return build level feature settings"; }
        virtual bool slaveOk() const { return true; }
        virtual bool readOnly() { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual bool run(const string& ns, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl) {
            if ( globalScriptEngine ) {
                BSONObjBuilder bb( result.subobjStart( "js" ) );
                result.append( "utf8" , globalScriptEngine->utf8Ok() );
                bb.done();
            }
            if ( cmdObj["oidReset"].trueValue() ) {
                result.append( "oidMachineOld" , OID::getMachineId() );
                OID::regenMachineId();
            }
            result.append( "oidMachine" , OID::getMachineId() );
            return true;
        }

    } featuresCmd;

    class LogRotateCmd : public Command {
    public:
        LogRotateCmd() : Command( "logRotate" ) {}
        virtual LockType locktype() const { return NONE; }
        virtual bool slaveOk() const { return true; }
        virtual bool adminOnly() const { return true; }
        virtual bool run(const string& ns, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl) {
            rotateLogs();
            return 1;
        }

    } logRotateCmd;

    class ListCommandsCmd : public Command {
    public:
        virtual void help( stringstream &help ) const { help << "get a list of all db commands"; }
        ListCommandsCmd() : Command( "listCommands", false ) {}
        virtual LockType locktype() const { return NONE; }
        virtual bool slaveOk() const { return true; }
        virtual bool adminOnly() const { return false; }
        virtual bool run(const string& ns, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl) {
            BSONObjBuilder b( result.subobjStart( "commands" ) );
            for ( map<string,Command*>::iterator i=_commands->begin(); i!=_commands->end(); ++i ) {
                Command * c = i->second;

                // don't show oldnames
                if (i->first != c->name)
                    continue;

                BSONObjBuilder temp( b.subobjStart( c->name ) );

                {
                    stringstream help;
                    c->help( help );
                    temp.append( "help" , help.str() );
                }
                temp.append( "lockType" , c->locktype() );
                temp.append( "slaveOk" , c->slaveOk() );
                temp.append( "adminOnly" , c->adminOnly() );
                temp.done();
            }
            b.done();

            return 1;
        }

    } listCommandsCmd;

    class CmdShutdown : public Command {
    public:
        virtual bool requiresAuth() { return true; }
        virtual bool adminOnly() const { return true; }
        virtual bool localHostOnlyIfNoAuth(const BSONObj& cmdObj) { return true; }
        virtual bool logTheOp() {
            return false;
        }
        virtual bool slaveOk() const {
            return true;
        }
        virtual LockType locktype() const { return NONE; }
        virtual void help( stringstream& help ) const {
            help << "shutdown the database.  must be ran against admin db and "
                 << "either (1) ran from localhost or (2) authenticated. If "
                 << "this is a primary in a replica set and there is no member "
                 << "within 10 seconds of its optime, it will not shutdown "
                 << "without force : true.  You can also specify timeoutSecs : "
                 << "N to wait N seconds for other members to catch up.";
        }
        CmdShutdown() : Command("shutdown") {}
        bool run(const string& dbname, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl) {
            bool force = cmdObj.hasField("force") && cmdObj["force"].trueValue();

            if (!force && theReplSet && theReplSet->isPrimary()) {
                int timeout = curTimeMicros64()/1000000;
                int now = timeout;
                if (cmdObj.hasField("timeoutSecs")) {
                    timeout += cmdObj["timeoutSecs"].numberInt();
                }

                long long int lastOp = 0, closest = 0, diff = 0;
                bool okay = false;
                while (now <= timeout) {
                    lastOp = (long long int)theReplSet->lastOpTimeWritten.getSecs();
                    closest = (long long int)theReplSet->lastOtherOpTime().getSecs();

                    diff = lastOp - closest;

                    if (diff >= 0 && diff <= 10) {
                        okay = true;
                        break;
                    }

                    sleepsecs(1);
                    now++;
                }

                if (!okay) {
                    errmsg = "no secondaries within 10 seconds of my optime";
                    result.append("closest", closest);
                    result.append("difference", diff);
                    return false;
                }
            }

            Client * c = currentClient.get();
            if ( c ) {
                c->shutdown();
            }

            log() << "terminating, shutdown command received" << endl;

            dbexit( EXIT_CLEAN , "shutdown called" , true ); // this never returns
            assert(0);
            return true;
        }
    } cmdShutdown;

    /* for testing purposes only */
    class CmdForceError : public Command {
    public:
        virtual void help( stringstream& help ) const {
            help << "for testing purposes only.  forces a user assertion exception";
        }
        virtual bool logTheOp() {
            return false;
        }
        virtual bool slaveOk() const {
            return true;
        }
        virtual LockType locktype() const { return NONE; }
        CmdForceError() : Command("forceerror") {}
        bool run(const string& dbnamne, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl) {
            uassert( 10038 , "forced error", false);
            return true;
        }
    } cmdForceError;

    class AvailableQueryOptions : public Command {
    public:
        AvailableQueryOptions() : Command( "availableQueryOptions" , false , "availablequeryoptions" ) {}
        virtual bool slaveOk() const { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual bool requiresAuth() { return false; }
        virtual bool run(const string& dbname , BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool) {
            result << "options" << QueryOption_AllSupported;
            return true;
        }
    } availableQueryOptionsCmd;


    class GetLogCmd : public Command {
    public:
        GetLogCmd() : Command( "getLog" ){}

        virtual bool slaveOk() const { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual bool requiresAuth() { return false; }
        virtual bool adminOnly() const { return true; }

        virtual void help( stringstream& help ) const {
            help << "{ getLog : '*' }  OR { getLog : 'global' }";
        }

        virtual bool run(const string& dbname , BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool) {
            string p = cmdObj.firstElement().String();
            if ( p == "*" ) {
                vector<string> names;
                RamLog::getNames( names );

                BSONArrayBuilder arr;
                for ( unsigned i=0; i<names.size(); i++ ) {
                    arr.append( names[i] );
                }
                
                result.appendArray( "names" , arr.arr() );
            }
            else {
                RamLog* rl = RamLog::get( p );
                if ( ! rl ) {
                    errmsg = str::stream() << "no RamLog named: " << p;
                    return false;
                }
                
                vector<const char*> lines;
                rl->get( lines );
                
                BSONArrayBuilder arr( result.subarrayStart( "log" ) );
                for ( unsigned i=0; i<lines.size(); i++ )
                    arr.append( lines[i] );
                arr.done();
            }
            return true;
        }

    } getLogCmd;

}
