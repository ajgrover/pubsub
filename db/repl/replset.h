// /db/repl/replset.h

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

#pragma once

#include "../../util/concurrency/list.h"
#include "../../util/concurrency/value.h"
#include "../../util/hostandport.h"
#include "rstime.h"
#include "rs_config.h"

namespace mongo {

    extern bool replSet; // true if using repl sets
    extern class ReplSet *theReplSet; // null until initialized

    extern Tee *rsLog;

    /* information about the entire repl set, such as the various servers in the set, and their state */
    /* note: We currently do not free mem when the set goes away - it is assumed the replset is a 
             singleton and long lived.
    */
    class ReplSet {
    public:
        static enum StartupStatus { PRESTART=0, LOADINGCONFIG=1, BADCONFIG=2, EMPTYCONFIG=3, EMPTYUNREACHABLE=4, STARTED=5, SOON=6 } startupStatus;
        static string startupStatusMsg;

        enum State {
            STARTUP,
            PRIMARY,
            SECONDARY,
            RECOVERING,
            FATAL,
            STARTUP2,
            UNKNOWN /* remote node not yet reached */
        };

    private: 
        State _myState;

    public:

        void fatal();
        bool isMaster(const char *client);
        void fillIsMaster(BSONObjBuilder&);
        bool ok() const { return _myState != FATAL; }
        State state() const { return _myState; }        
        string name() const { return _name; } /* @return replica set's logical name */

        /* cfgString format is 
           replsetname/host1,host2:port,...
           where :port is optional.

           throws exception if a problem initializing.
        */
        ReplSet(string cfgString);

        /* call after constructing to start - returns fairly quickly after launching its threads */
        void go() { _myState = STARTUP2; startHealthThreads(); }

        // for replSetGetStatus command
        void summarizeStatus(BSONObjBuilder&) const;
        void summarizeAsHtml(stringstream&) const;
        const ReplSetConfig& config() { return *_cfg; }

    private:
        string _name;
        const vector<HostAndPort> *_seeds;
        ReplSetConfig *_cfg;

        /** load our configuration from admin.replset.  try seed machines too. 
            throws exception if a problem.
        */
        void loadConfig();
        void finishLoadingConfig(vector<ReplSetConfig>& v);
        void initFromConfig(ReplSetConfig& c);//, bool save);

        struct Consensus {
            ReplSet &rs;
            Consensus(ReplSet *t) : rs(*t) { }
            int totalVotes() const;
            bool aMajoritySeemsToBeUp() const;
            bool electSelf();
        } elect;

    public:
        struct Member : public List1<Member>::Base {
            Member(HostAndPort h, int ord, const ReplSetConfig::MemberCfg *c);
            const HostAndPort _h;
            const unsigned _id; // ordinal
            string fullName() const { return _h.toString(); }
            double health() const { return _health; }
            time_t upSince() const { return _upSince; }
            time_t lastHeartbeat() const { return _lastHeartbeat; }
            const ReplSetConfig::MemberCfg& config() const { return *_config; }
            bool up() const { return health() > 0; }
            void summarizeAsHtml(stringstream& s) const;
            ReplSet::State state() const { return _state; }
            DiagStr _lastHeartbeatErrMsg;
        private:
            friend class FeedbackThread; // feedbackthread is the primary writer to these objects
            const ReplSetConfig::MemberCfg *_config; /* todo: when this changes??? */

            bool _dead;
            double _health;
            time_t _lastHeartbeat;
            time_t _upSince;
            ReplSet::State _state;
        };

        list<HostAndPort> memberHostnames() const;
        const Member* currentPrimary() const { return _currentPrimary; }
        const ReplSetConfig::MemberCfg& myConfig() const { return _self->config(); }

    private:
        const Member *_currentPrimary;

        static string stateAsStr(State state);
        static string stateAsHtml(State state);

        Member *_self;
        /* all members of the set EXCEPT self. */
        List1<Member> _members;
        Member* head() const { return _members.head(); }

        void startHealthThreads();
        friend class FeedbackThread;

    public:
        class Manager : boost::noncopyable {
            ReplSet *_rs;
            int _primary;
            const Member* findOtherPrimary();
            void noteARemoteIsPrimary(const Member *);
        public:
            Manager(ReplSet *rs);
            void checkNewState();
        } _mgr;

    };

    inline void ReplSet::fatal() 
    { _myState = FATAL; log() << "replSet error fatal error, stopping replication" << rsLog; }

    inline ReplSet::Member::Member(HostAndPort h, int ord, const ReplSetConfig::MemberCfg *c) : 
        _h(h), _id(ord), _config(c)
    { 
        _dead = false;
        _lastHeartbeat = 0;
        _upSince = 0;
        _health = -1.0;
        _state = UNKNOWN;
    }

    inline bool ReplSet::isMaster(const char *client) {         
        /* todo replset */
        return false;
    }

}
