# -*- mode: python; -*-
# build file for MongoDB
# this requires scons
# you can get from http://www.scons.org
# then just type scons

# some common tasks
#   build 64-bit mac and pushing to s3
#      scons --64 s3dist
#      scons --distname=0.8 s3dist
#      all s3 pushes require settings.py and simples3

import os
import sys
import imp
import types
import re
import shutil
import urllib
import urllib2
import buildscripts
import buildscripts.bb
from buildscripts import utils

buildscripts.bb.checkOk()

# --- options ----
AddOption('--prefix',
          dest='prefix',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='installation prefix')

AddOption('--distname',
          dest='distname',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='dist name (0.8.0)')

AddOption('--distmod',
          dest='distmod',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='additional piece for full dist name')


AddOption( "--64",
           dest="force64",
           type="string",
           nargs=0,
           action="store",
           help="whether to force 64 bit" )


AddOption( "--32",
           dest="force32",
           type="string",
           nargs=0,
           action="store",
           help="whether to force 32 bit" )


AddOption( "--mm",
           dest="mm",
           type="string",
           nargs=0,
           action="store",
           help="use main memory instead of memory mapped files" )


AddOption( "--release",
           dest="release",
           type="string",
           nargs=0,
           action="store",
           help="relase build")


AddOption( "--static",
           dest="static",
           type="string",
           nargs=0,
           action="store",
           help="fully static build")


AddOption('--usesm',
          dest='usesm',
          type="string",
          nargs=0,
          action="store",
          help="use spider monkey for javascript" )

AddOption('--usev8',
          dest='usev8',
          type="string",
          nargs=0,
          action="store",
          help="use v8 for javascript" )

AddOption('--asio',
          dest='asio',
          type="string",
          nargs=0,
          action="store",
          help="Use Asynchronous IO (NOT READY YET)" )

AddOption( "--d",
           dest="debugBuild",
           type="string",
           nargs=0,
           action="store",
           help="debug build no optimization, etc..." )

AddOption( "--dd",
           dest="debugBuildAndLogging",
           type="string",
           nargs=0,
           action="store",
           help="debug build no optimization, additional debug logging, etc..." )

AddOption( "--recstore",
           dest="recstore",
           type="string",
           nargs=0,
           action="store",
           help="use new recstore" )

AddOption( "--noshell",
           dest="noshell",
           type="string",
           nargs=0,
           action="store",
           help="don't build shell" )

AddOption( "--safeshell",
           dest="safeshell",
           type="string",
           nargs=0,
           action="store",
           help="don't let shell scripts run programs (still, don't run untrusted scripts)" )

AddOption( "--extrapath",
           dest="extrapath",
           type="string",
           nargs=1,
           action="store",
           help="comma separated list of add'l paths  (--extrapath /opt/foo/,/foo) static linking" )

AddOption( "--extrapathdyn",
           dest="extrapathdyn",
           type="string",
           nargs=1,
           action="store",
           help="comma separated list of add'l paths  (--extrapath /opt/foo/,/foo) dynamic linking" )


AddOption( "--extralib",
           dest="extralib",
           type="string",
           nargs=1,
           action="store",
           help="comma separated list of libraries  (--extralib js_static,readline" )

AddOption( "--staticlib",
           dest="staticlib",
           type="string",
           nargs=1,
           action="store",
           help="comma separated list of libs to link statically (--staticlib js_static,boost_program_options-mt,..." )

AddOption( "--staticlibpath",
           dest="staticlibpath",
           type="string",
           nargs=1,
           action="store",
           help="comma separated list of dirs to search for staticlib arguments" )

AddOption( "--cxx",
           dest="cxx",
           type="string",
           nargs=1,
           action="store",
           help="compiler to use" )


AddOption( "--boost-compiler",
           dest="boostCompiler",
           type="string",
           nargs=1,
           action="store",
           help="compiler used for boost (gcc41)" )

AddOption( "--boost-version",
           dest="boostVersion",
           type="string",
           nargs=1,
           action="store",
           help="boost version for linking(1_38)" )

AddOption( "--cpppath",
           dest="cpppath",
           type="string",
           nargs=1,
           action="store",
           help="Include path if you have headers in a nonstandard directory" )

AddOption( "--libpath",
           dest="libpath",
           type="string",
           nargs=1,
           action="store",
           help="Library path if you have libraries in a nonstandard directory" )

# 
# to use CPUPROFILE=/tmp/profile
# to view pprof -gv mongod /tmp/profile
#
AddOption( "--pg",
           dest="profile",
           type="string",
           nargs=0,
           action="store" )

AddOption( "--gdbserver",
           dest="gdbserver",
           type="string",
           nargs=0,
           action="store" )

AddOption("--nostrip",
          dest="nostrip",
          action="store_true",
          help="do not strip installed binaries")

AddOption("--sharedclient",
          dest="sharedclient",
          action="store",
          help="build a libmongoclient.so/.dll")

AddOption("--smokedbprefix",
          dest="smokedbprefix",
          action="store",
          help="prefix to dbpath et al. for smoke tests")

# --- environment setup ---

def removeIfInList( lst , thing ):
    if thing in lst:
        lst.remove( thing )

def printLocalInfo():
    import sys, SCons
    print( "scons version: " + SCons.__version__ )
    print( "python version: " + " ".join( [ `i` for i in sys.version_info ] ) )

printLocalInfo()

boostLibs = [ "thread" , "filesystem" , "program_options" ]

onlyServer = len( COMMAND_LINE_TARGETS ) == 0 or ( len( COMMAND_LINE_TARGETS ) == 1 and str( COMMAND_LINE_TARGETS[0] ) in [ "mongod" , "mongos" , "test" ] )
nix = False
linux = False
linux64  = False
darwin = False
windows = False
freebsd = False
solaris = False
force64 = not GetOption( "force64" ) is None
if not force64 and os.getcwd().endswith( "mongo-64" ):
    force64 = True
    print( "*** assuming you want a 64-bit build b/c of directory *** " )
msarch = None
if force64:
    msarch = "amd64"

force32 = not GetOption( "force32" ) is None
release = not GetOption( "release" ) is None
static = not GetOption( "static" ) is None

debugBuild = ( not GetOption( "debugBuild" ) is None ) or ( not GetOption( "debugBuildAndLogging" ) is None )
debugLogging = not GetOption( "debugBuildAndLogging" ) is None
noshell = not GetOption( "noshell" ) is None

usesm = not GetOption( "usesm" ) is None
usev8 = not GetOption( "usev8" ) is None

asio = not GetOption( "asio" ) is None

justClientLib = (COMMAND_LINE_TARGETS == ['mongoclient'])

env = Environment( MSVS_ARCH=msarch , tools = ["default", "gch"], toolpath = '.' )
if GetOption( "cxx" ) is not None:
    env["CC"] = GetOption( "cxx" )
    env["CXX"] = GetOption( "cxx" )
env["LIBPATH"] = []

if GetOption( "libpath" ) is not None:
    env["LIBPATH"] = [GetOption( "libpath" )]

if GetOption( "cpppath" ) is not None:
    env["CPPPATH"] = [GetOption( "cpppath" )]

if GetOption( "recstore" ) != None:
    env.Append( CPPDEFINES=[ "_RECSTORE" ] )
env.Append( CPPDEFINES=[ "_SCONS" , "MONGO_EXPOSE_MACROS" ] )
env.Append( CPPPATH=[ "." ] )


if GetOption( "safeshell" ) != None:
    env.Append( CPPDEFINES=[ "MONGO_SAFE_SHELL" ] )

boostCompiler = GetOption( "boostCompiler" )
if boostCompiler is None:
    boostCompiler = ""
else:
    boostCompiler = "-" + boostCompiler

boostVersion = GetOption( "boostVersion" )
if boostVersion is None:
    boostVersion = ""
else:
    boostVersion = "-" + boostVersion

if ( not ( usesm or usev8 or justClientLib) ):
    usesm = True

distBuild = len( COMMAND_LINE_TARGETS ) == 1 and ( str( COMMAND_LINE_TARGETS[0] ) == "s3dist" or str( COMMAND_LINE_TARGETS[0] ) == "dist" )

extraLibPlaces = []

def addExtraLibs( s ):
    for x in s.split(","):
        env.Append( CPPPATH=[ x + "/include" ] )
        env.Append( LIBPATH=[ x + "/lib" ] )
        env.Append( LIBPATH=[ x + "/lib64" ] )
        extraLibPlaces.append( x + "/lib" )

if GetOption( "extrapath" ) is not None:
    addExtraLibs( GetOption( "extrapath" ) )
    release = True

if GetOption( "extrapathdyn" ) is not None:
    addExtraLibs( GetOption( "extrapathdyn" ) )

if GetOption( "extralib" ) is not None:
    for x in GetOption( "extralib" ).split( "," ):
        env.Append( LIBS=[ x ] )

class InstallSetup:
    binaries = False
    clientSrc = False
    headers = False
    bannerDir = None
    headerRoot = "include"

    def __init__(self):
        self.default()
    
    def default(self):
        self.binaries = True
        self.clientSrc = False
        self.headers = False
        self.bannerDir = None
        self.headerRoot = "include"

    def justClient(self):
        self.binaries = False
        self.clientSrc = True
        self.headers = True
        self.bannerDir = "distsrc/client/"
        self.headerRoot = ""
        
installSetup = InstallSetup()
if distBuild:
    installSetup.bannerDir = "distsrc"


# ------    SOURCE FILE SETUP -----------

commonFiles = Split( "pch.cpp buildinfo.cpp db/common.cpp db/jsobj.cpp db/json.cpp db/lasterror.cpp db/nonce.cpp db/queryutil.cpp shell/mongo.cpp" )
commonFiles += [ "util/background.cpp" , "util/mmap.cpp" , "util/ramstore.cpp", "util/sock.cpp" ,  "util/util.cpp" , "util/message.cpp" , 
                 "util/assert_util.cpp" , "util/httpclient.cpp" , "util/md5main.cpp" , "util/base64.cpp", "util/concurrency/vars.cpp", "util/concurrency/task.cpp", "util/debug_util.cpp",
                 "util/thread_pool.cpp", "util/password.cpp" ]
commonFiles += Glob( "util/*.c" )
commonFiles += Split( "client/connpool.cpp client/dbclient.cpp client/dbclientcursor.cpp client/model.cpp client/syncclusterconnection.cpp s/shardconnection.cpp" )

#mmap stuff

if GetOption( "mm" ) != None:
    commonFiles += [ "util/mmap_mm.cpp" ]
elif os.sys.platform == "win32":
    commonFiles += [ "util/mmap_win.cpp" ]
else:
    commonFiles += [ "util/mmap_posix.cpp" ]

if os.path.exists( "util/processinfo_" + os.sys.platform + ".cpp" ):
    commonFiles += [ "util/processinfo_" + os.sys.platform + ".cpp" ]
else:
    commonFiles += [ "util/processinfo_none.cpp" ]

coreDbFiles = [ "db/commands.cpp" ]
coreServerFiles = [ "util/message_server_port.cpp" , "util/message_server_asio.cpp" , 
                    "client/parallel.cpp" ,  
                    "db/matcher.cpp" , "db/indexkey.cpp" , "db/dbcommands_generic.cpp" ]

serverOnlyFiles = Split( "db/query.cpp db/update.cpp db/introspect.cpp db/btree.cpp db/clientcursor.cpp db/tests.cpp db/repl.cpp db/repl/replset.cpp db/repl/consensus.cpp db/repl/rs_initiate.cpp db/repl/replset_commands.cpp db/repl/manager.cpp db/repl/health.cpp db/repl/heartbeat.cpp db/repl/rs_config.cpp db/oplog.cpp db/repl_block.cpp db/btreecursor.cpp db/cloner.cpp db/namespace.cpp db/matcher_covered.cpp db/dbeval.cpp db/dbwebserver.cpp db/dbhelpers.cpp db/instance.cpp db/client.cpp db/database.cpp db/pdfile.cpp db/cursor.cpp db/security_commands.cpp db/security.cpp util/miniwebserver.cpp db/storage.cpp db/queryoptimizer.cpp db/extsort.cpp db/mr.cpp s/d_util.cpp db/cmdline.cpp" )

serverOnlyFiles += [ "db/index.cpp" ] + Glob( "db/index_*.cpp" )

serverOnlyFiles += [ "db/dbcommands.cpp" , "db/dbcommands_admin.cpp" ]
coreServerFiles += Glob( "db/stats/*.cpp" )
serverOnlyFiles += [ "db/driverHelpers.cpp" ]

scriptingFiles = [ "scripting/engine.cpp" , "scripting/utils.cpp" ]

if usesm:
    scriptingFiles += [ "scripting/engine_spidermonkey.cpp" ]
elif usev8:
    scriptingFiles += [ Glob( "scripting/*v8*.cpp" ) ]
else:
    scriptingFiles += [ "scripting/engine_none.cpp" ]

coreServerFiles += scriptingFiles

coreShardFiles = []
shardServerFiles = coreShardFiles + Glob( "s/strategy*.cpp" ) + [ "s/commands_admin.cpp" , "s/commands_public.cpp" , "s/request.cpp" ,  "s/cursors.cpp" ,  "s/server.cpp" , "s/chunk.cpp" , "s/shard.cpp" , "s/shardkey.cpp" , "s/config.cpp" , "s/config_migrate.cpp" , "s/s_only.cpp" , "s/stats.cpp" , "s/balance.cpp" , "db/cmdline.cpp" ]
serverOnlyFiles += coreShardFiles + [ "s/d_logic.cpp" ]

serverOnlyFiles += [ "db/module.cpp" ] + Glob( "db/modules/*.cpp" )

modules = []
moduleNames = []

for x in os.listdir( "db/modules/" ):
    if x.find( "." ) >= 0:
        continue
    print( "adding module: " + x )
    moduleNames.append( x )
    modRoot = "db/modules/" + x + "/"
    serverOnlyFiles += Glob( modRoot + "src/*.cpp" )
    modBuildFile = modRoot + "build.py"
    if os.path.exists( modBuildFile ):
        modules += [ imp.load_module( "module_" + x , open( modBuildFile , "r" ) , modBuildFile , ( ".py" , "r" , imp.PY_SOURCE  ) ) ]

allClientFiles = commonFiles + coreDbFiles + [ "client/clientOnly.cpp" , "client/gridfs.cpp" , "s/d_util.cpp" ];

allCXXFiles = allClientFiles + coreShardFiles + shardServerFiles + serverOnlyFiles;

# ---- other build setup -----

platform = os.sys.platform
if "uname" in dir(os):
    processor = os.uname()[4]
else:
    processor = "i386"

if force32:
    processor = "i386"
if force64:
    processor = "x86_64"

DEFAULT_INSTALl_DIR = "/usr/local"
installDir = DEFAULT_INSTALl_DIR
nixLibPrefix = "lib"

distName = GetOption( "distname" )
dontReplacePackage = False

if distBuild:
    release = True

if GetOption( "prefix" ):
    installDir = GetOption( "prefix" )
    if installDir.find( "mongo-cxx-driver" ) >= 0:
        installSetup.justClient()


def findVersion( root , choices ):
    if not isinstance(root, list):
        root = [root]
    for r in root:
        for c in choices:
            if ( os.path.exists( r + c ) ):
                return r + c
    raise RuntimeError("can't find a version of [" + repr(root) + "] choices: " + repr(choices))

def choosePathExist( choices , default=None):
    for c in choices:
        if c != None and os.path.exists( c ):
            return c
    return default

def filterExists(paths):
    return filter(os.path.exists, paths)

if "darwin" == os.sys.platform:
    darwin = True
    platform = "osx" # prettier than darwin

    if env["CXX"] is None:
        if os.path.exists( "/usr/bin/g++-4.2" ):
            env["CXX"] = "g++-4.2"

    nix = True

    if force64:
        env.Append( CPPPATH=["/usr/64/include"] )
        env.Append( LIBPATH=["/usr/64/lib"] )
        if installDir == DEFAULT_INSTALl_DIR and not distBuild:
            installDir = "/usr/64/"
    else:
        env.Append( CPPPATH=filterExists(["/sw/include" , "/opt/local/include"]) )
        env.Append( LIBPATH=filterExists(["/sw/lib/", "/opt/local/lib"]) )

elif "linux2" == os.sys.platform:
    linux = True
    platform = "linux"

    if os.uname()[4] == "x86_64" and not force32:
        linux64 = True
        nixLibPrefix = "lib64"
        env.Append( LIBPATH=["/usr/lib64" , "/lib64" ] )
        env.Append( LIBS=["pthread"] )

        force64 = False

    if force32:
        env.Append( LIBPATH=["/usr/lib32"] )

    nix = True

    if static:
        env.Append( LINKFLAGS=" -static " )

elif "sunos5" == os.sys.platform:
     nix = True
     solaris = True
     env.Append( CPPDEFINES=[ "__sunos__" ] )
     env.Append( LIBS=["socket","resolv"] )

elif os.sys.platform.startswith( "freebsd" ):
    nix = True
    freebsd = True
    env.Append( CPPPATH=[ "/usr/local/include" ] )
    env.Append( LIBPATH=[ "/usr/local/lib" ] )
    env.Append( CPPDEFINES=[ "__freebsd__" ] )

elif "win32" == os.sys.platform:
    windows = True
    if force64:
        release = True

    for pathdir in env['ENV']['PATH'].split(os.pathsep):
	if os.path.exists(os.path.join(pathdir, 'cl.exe')):
            print( "found visual studio at " + pathdir )
	    break
    else:
	#use current environment
	env['ENV'] = dict(os.environ)

    def find_boost():
        for x in ('', ' (x86)'):	
            boostDir = "C:/Program Files" + x + "/boost/latest"
            if os.path.exists( boostDir ):
                return boostDir
            for bv in reversed( range(33,50) ):
	            for extra in ('', '_0', '_1'):
		            boostDir = "C:/Program Files" + x + "/Boost/boost_1_" + str(bv) + extra
		            if os.path.exists( boostDir ):
		                return boostDir
        if os.path.exists( "C:/boost" ):
	        return "C:/boost"
        if os.path.exists( "/boost" ):
	        return "/boost"
        return None

    boostDir = find_boost()
    if boostDir is None:
        print( "can't find boost" )
        Exit(1)

    serverOnlyFiles += [ "util/ntservice.cpp" ]

    boostLibs = []

    env.Append( CPPPATH=[ "js/src/" ] )
    env.Append(CPPPATH=["../js/src/"])
    env.Append(LIBPATH=["../js/src"])
    env.Append( CPPDEFINES=[ "OLDJS" ] )

    winSDKHome = findVersion( [ "C:/Program Files/Microsoft SDKs/Windows/", "C:/Program Files (x86)/Microsoft SDKs/Windows/" ] ,
                              [ "v6.0" , "v6.0a" , "v6.1", "v7.0A" ] )

    env.Append( CPPPATH=[ boostDir , "pcre-7.4" , winSDKHome + "/Include" ] )

    env.Append( CPPFLAGS=" /EHsc /W3 " )
    env.Append( CPPFLAGS=" /wd4355 /wd4800 " ) #some warnings we don't like
    env.Append( CPPDEFINES=["WIN32","_CONSOLE","_CRT_SECURE_NO_WARNINGS","HAVE_CONFIG_H","PCRE_STATIC","_UNICODE","UNICODE","SUPPORT_UCP","SUPPORT_UTF8,PSAPI_VERSION=1" ] )

    #env.Append( CPPFLAGS='  /Yu"pch.h" ' ) # this would be for pre-compiled headers, could play with it later

    if release:
        env.Append( CPPDEFINES=[ "NDEBUG" ] )
        env.Append( CPPFLAGS= " /O2 /FD /MT /Gy /Zi /TP /errorReport:prompt /Gm " )
        # TODO: this has caused some linking problems :
        env.Append( CPPFLAGS= " /GL " ) 
        env.Append( LINKFLAGS=" /LTCG " )
    else:
        env.Append( CPPDEFINES=[ "_DEBUG" ] )
        # /Od disable optimization
        # /ZI debug info w/edit & continue 
        # RTC1 /GZ (Enable Stack Frame Run-Time Error Checking)
        env.Append( CPPFLAGS=" /Od /Gm /RTC1 /MDd /ZI " )
        env.Append( CPPFLAGS=' /Fd"mongod.pdb" ' )
        env.Append( LINKFLAGS=" /incremental:yes /debug " )

    if force64 and os.path.exists( boostDir + "/lib/vs2010_64" ):
        env.Append( LIBPATH=[ boostDir + "/lib/vs2010_64" ] )
    elif not force64 and os.path.exists( boostDir + "/lib/vs2010_32" ):
        env.Append( LIBPATH=[ boostDir + "/lib/vs2010_32" ] )
    else:
        env.Append( LIBPATH=[ boostDir + "/Lib" ] )

    if force64:
        env.Append( LIBPATH=[ winSDKHome + "/Lib/x64" ] )
        env.Append( LINKFLAGS=" /NODEFAULTLIB:MSVCPRT  /NODEFAULTLIB:MSVCRT " )
    else:
        env.Append( LIBPATH=[ winSDKHome + "/Lib" ] )

    def pcreFilter(x):
        name = x.name
        if x.name.endswith( "dftables.c" ):
            return False
        if x.name.endswith( "pcredemo.c" ):
            return False
        if x.name.endswith( "pcretest.c" ):
            return False
        if x.name.endswith( "unittest.cc" ):
            return False
        if x.name.endswith( "pcregrep.c" ):
            return False
        return True

    pcreFiles = []
    pcreFiles += filter( pcreFilter , Glob( "pcre-7.4/*.c"  ) )
    pcreFiles += filter( pcreFilter , Glob( "pcre-7.4/*.cc" ) )
    commonFiles += pcreFiles
    allClientFiles += pcreFiles

    winLibString = "ws2_32.lib kernel32.lib advapi32.lib Psapi.lib"

    if force64:
        winLibString += " LIBCMT LIBCPMT "
    else:
        winLibString += " user32.lib gdi32.lib winspool.lib comdlg32.lib  shell32.lib ole32.lib oleaut32.lib "
        winLibString += " odbc32.lib odbccp32.lib uuid.lib "

    env.Append( LIBS=Split(winLibString) )

    if force64:
        env.Append( CPPDEFINES=["_AMD64_=1"] )
    else:
        env.Append( CPPDEFINES=["_X86_=1"] )

    env.Append( CPPPATH=["../winpcap/Include"] )
    env.Append( LIBPATH=["../winpcap/Lib"] )

else:
    print( "No special config for [" + os.sys.platform + "] which probably means it won't work" )

if nix:
    env.Append( CPPFLAGS="-fPIC -fno-strict-aliasing -ggdb -pthread -Wall -Wsign-compare -Wno-unknown-pragmas -Winvalid-pch" )
    env.Append( CXXFLAGS=" -Wnon-virtual-dtor " )
    env.Append( LINKFLAGS=" -fPIC -pthread -rdynamic" )
    env.Append( LIBS=[] )

    if debugBuild:
        env.Append( CPPFLAGS=" -O0 -fstack-protector " );
        env['ENV']['GLIBCXX_FORCE_NEW'] = 1; # play nice with valgrind
    else:
        env.Append( CPPFLAGS=" -O3" )

    if debugLogging:
        env.Append( CPPFLAGS=" -D_DEBUG" );

    if force64:
        env.Append( CFLAGS="-m64" )
        env.Append( CXXFLAGS="-m64" )
        env.Append( LINKFLAGS="-m64" )

    if force32:
        env.Append( CFLAGS="-m32" )
        env.Append( CXXFLAGS="-m32" )
        env.Append( LINKFLAGS="-m32" )

    if GetOption( "profile" ) is not None:
        env.Append( LIBS=[ "profiler" ] )

    if GetOption( "gdbserver" ) is not None:
        env.Append( CPPDEFINES=["USE_GDBSERVER"] )

    # pre-compiled headers
    if False and 'Gch' in dir( env ):
        print( "using precompiled headers" )
        env['Gch'] = env.Gch( [ "pch.h" ] )[0]
        #Depends( "pch.o" , "pch.h.gch" )
        #SideEffect( "dummyGCHSideEffect" , "pch.h.gch" )

if usev8:
    env.Append( CPPPATH=["../v8/include/"] )
    env.Append( LIBPATH=["../v8/"] )


if "uname" in dir(os):
    hacks = buildscripts.findHacks( os.uname() )
    if hacks is not None:
        hacks.insert( env , { "linux64" : linux64 } )

try:
    umask = os.umask(022)
except OSError:
    pass

# --- check system ---

def getGitBranch():
    if not os.path.exists( ".git" ):
        return None
    
    version = open( ".git/HEAD" ,'r' ).read().strip()
    if not version.startswith( "ref: " ):
        return version
    version = version.split( "/" )
    version = version[len(version)-1]
    return version

def getGitBranchString( prefix="" , postfix="" ):
    t = re.compile( '[/\\\]' ).split( os.getcwd() )
    if len(t) > 2 and t[len(t)-1] == "mongo":
        par = t[len(t)-2]
        m = re.compile( ".*_([vV]\d+\.\d+)$" ).match( par )
        if m is not None:
            return prefix + m.group(1).lower() + postfix
        if par.find("Nightly") > 0:
            return ""


    b = getGitBranch()
    if b == None or b == "master":
        return ""
    return prefix + b + postfix

def getGitVersion():
    if not os.path.exists( ".git" ):
        return "nogitversion"

    version = open( ".git/HEAD" ,'r' ).read().strip()
    if not version.startswith( "ref: " ):
        return version
    version = version[5:]
    f = ".git/" + version
    if not os.path.exists( f ):
        return version
    return open( f , 'r' ).read().strip()

def getSysInfo():
    if windows:
        return "windows " + str( sys.getwindowsversion() )
    else:
        return " ".join( os.uname() )

def add_exe(target):
    if windows:
        return target + ".exe"
    return target

def setupBuildInfoFile( outFile ):
    version = getGitVersion()
    if len(moduleNames) > 0:
        version = version + " modules: " + ','.join( moduleNames )
    sysInfo = getSysInfo()
    contents = '\n'.join([
        '#include "pch.h"',
        '#include <iostream>',
        '#include <boost/version.hpp>',
        'namespace mongo { const char * gitVersion(){ return "' + version + '"; } }',
        'namespace mongo { string sysInfo(){ return "' + sysInfo + ' BOOST_LIB_VERSION=" BOOST_LIB_VERSION ; } }',
        ])

    contents += '\n';

    if os.path.exists( outFile ) and open( outFile ).read().strip() == contents.strip():
        return

    contents += '\n';

    out = open( outFile , 'w' )
    out.write( contents )
    out.close()

setupBuildInfoFile( "buildinfo.cpp" )

def bigLibString( myenv ):
    s = str( myenv["LIBS"] )
    if 'SLIBS' in myenv._dict:
        s += str( myenv["SLIBS"] )
    return s


def doConfigure( myenv , needPcre=True , shell=False ):
    conf = Configure(myenv)
    myenv["LINKFLAGS_CLEAN"] = list( myenv["LINKFLAGS"] )
    myenv["LIBS_CLEAN"] = list( myenv["LIBS"] )

    if 'CheckCXX' in dir( conf ):
        if  not conf.CheckCXX():
            print( "c++ compiler not installed!" )
            Exit(1)

    if nix and not shell:
        if not conf.CheckLib( "stdc++" ):
            print( "can't find stdc++ library which is needed" );
            Exit(1)

    def myCheckLib( poss , failIfNotFound=False , staticOnly=False):

        if type( poss ) != types.ListType :
            poss = [poss]

        allPlaces = [];
        allPlaces += extraLibPlaces
        if nix and release:
            allPlaces += myenv["LIBPATH"]
            if not force64:
                allPlaces += [ "/usr/lib" , "/usr/local/lib" ]

            for p in poss:
                for loc in allPlaces:
                    fullPath = loc + "/lib" + p + ".a"
                    if os.path.exists( fullPath ):
                        myenv['_LIBFLAGS']='${_stripixes(LIBLINKPREFIX, LIBS, LIBLINKSUFFIX, LIBPREFIXES, LIBSUFFIXES, __env__)} $SLIBS'
                        myenv.Append( SLIBS=" " + fullPath + " " )
                        return True


        if release and not windows and failIfNotFound:
            print( "ERROR: can't find static version of: " + str( poss ) + " in: " + str( allPlaces ) )
            Exit(1)

        res = not staticOnly and conf.CheckLib( poss )
        if res:
            return True

        if failIfNotFound:
            print( "can't find or link against library " + str( poss ) + " in " + str( myenv["LIBPATH"] ) )
            print( "see config.log for more information" )
            Exit(1)

        return False

    if needPcre and not conf.CheckCXXHeader( 'pcrecpp.h' ):
        print( "can't find pcre" )
        Exit(1)

    if not conf.CheckCXXHeader( "boost/filesystem/operations.hpp" ):
        print( "can't find boost headers" )
        if shell:
            print( "\tshell might not compile" )
        else:
            Exit(1)

    if asio:
        if conf.CheckCXXHeader( "boost/asio.hpp" ):
            myenv.Append( CPPDEFINES=[ "USE_ASIO" ] )
        else:
            print( "WARNING: old version of boost - you should consider upgrading" )

    # this will add it iff it exists and works
    myCheckLib( [ "boost_system" + boostCompiler + "-mt" + boostVersion ,
                  "boost_system" + boostCompiler + boostVersion ] )

    for b in boostLibs:
        l = "boost_" + b
        myCheckLib( [ l + boostCompiler + "-mt" + boostVersion ,
                      l + boostCompiler + boostVersion ] ,
                    release or not shell)

    if not conf.CheckCXXHeader( "execinfo.h" ):
        myenv.Append( CPPDEFINES=[ "NOEXECINFO" ] )

    if nix and needPcre:
        myCheckLib( "pcrecpp" , True )
        myCheckLib( "pcre" , True )

    myenv["_HAVEPCAP"] = myCheckLib( ["pcap", "wpcap"] )
    removeIfInList( myenv["LIBS"] , "pcap" )
    removeIfInList( myenv["LIBS"] , "wpcap" )

    for m in modules:
        m.configure( conf , myenv )

    # this is outside of usesm block so don't have to rebuild for java
    if windows:
        myenv.Append( CPPDEFINES=[ "XP_WIN" ] )
    else:
        myenv.Append( CPPDEFINES=[ "XP_UNIX" ] )

    if solaris:
        conf.CheckLib( "nsl" )

    if usesm:

        myCheckLib( [ "mozjs" , "js", "js_static" ] , True )
        mozHeader = "js"
        if bigLibString(myenv).find( "mozjs" ) >= 0:
            mozHeader = "mozjs"

        if not conf.CheckHeader( mozHeader + "/jsapi.h" ):
            if conf.CheckHeader( "jsapi.h" ):
                myenv.Append( CPPDEFINES=[ "OLDJS" ] )
            else:
                print( "no spider monkey headers!" )
                Exit(1)

    if usev8:
        if debugBuild:
            myCheckLib( [ "v8_g" , "v8" ] , True )
        else:
            myCheckLib( "v8" , True )

    if shell:
        haveReadLine = False
        if darwin:
            myenv.Append( CPPDEFINES=[ "USE_READLINE" ] )
            if force64:
                myCheckLib( "readline" , True )
                myCheckLib( "ncurses" , True )
            else:
                myenv.Append( LINKFLAGS=" /usr/lib/libreadline.dylib " )
        elif myCheckLib( "readline" , release and nix , staticOnly=release ):
            myenv.Append( CPPDEFINES=[ "USE_READLINE" ] )
            myCheckLib( "ncurses" , staticOnly=release )
            myCheckLib( "tinfo" , staticOnly=release )
        else:
            print( "warning: no readline, shell will be a bit ugly" )

        if linux:
            myCheckLib( "rt" , True )

    # requires ports devel/libexecinfo to be installed
    if freebsd:
        myCheckLib( "execinfo", True )
        env.Append( LIBS=[ "execinfo" ] )

    # Handle staticlib,staticlibpath options.
    staticlibfiles = []
    if GetOption( "staticlib" ) is not None:
        # FIXME: probably this loop ought to do something clever
        # depending on whether we want to use 32bit or 64bit
        # libraries.  For now, we sort of rely on the user supplying a
        # sensible staticlibpath option. (myCheckLib implements an
        # analogous search, but it also does other things I don't
        # understand, so I'm not using it.)
        if GetOption ( "staticlibpath" ) is not None:
            dirs = GetOption ( "staticlibpath" ).split( "," )
        else:
            dirs = [ "/usr/lib64", "/usr/lib" ]

        for l in GetOption( "staticlib" ).split( "," ):
            removeIfInList(myenv["LIBS"], l)
            found = False
            for d in dirs:
                f=  "%s/lib%s.a" % ( d, l )
                if os.path.exists( f ):
                    staticlibfiles.append(f)
                    found = True
                    break
            if not found:
                raise "can't find a static %s" % l

    myenv.Append(LINKCOM=" $STATICFILES")
    myenv.Append(STATICFILES=staticlibfiles)

    return conf.Finish()

env = doConfigure( env )

# --- js concat ---

def concatjs(target, source, env):

    outFile = str( target[0] )

    fullSource = ""

    first = True

    for s in source:
        f = open( str(s) , 'r' )
        for l in f:
            l = l.split("//")[0].strip()
            if len ( l ) == 0:
                continue
            
            if l == "}":
                fullSource += "}"
                continue

            if first:
                first = False
            else:
                fullSource += "\n"

            fullSource += l

    fullSource += "\n"
    
    fullSource = re.compile( r'/\*\*.*?\*/' , re.M | re.S ).sub( "" , fullSource )

    out = open( outFile , 'w' )
    out.write( fullSource )

    return None

jsBuilder = Builder(action = concatjs,
                    suffix = '.jsall',
                    src_suffix = '.js')

env.Append( BUILDERS={'JSConcat' : jsBuilder})

# --- jsh ---

def jsToH(target, source, env):

    outFile = str( target[0] )
    if len( source ) != 1:
        raise Exception( "wrong" )

    h = "const char * jsconcatcode" + outFile.split( "mongo" )[-1].replace( "-" , "_").split( ".cpp")[0] + " = \n"

    for l in open( str(source[0]) , 'r' ):
        l = l.strip()
        l = l.split( "//" )[0]
        l = l.replace( '\\' , "\\\\" )
        l = l.replace( '"' , "\\\"" )


        h += '"' + l + "\\n\"\n "

    h += ";\n\n"

    out = open( outFile , 'w' )
    out.write( h )

    return None

jshBuilder = Builder(action = jsToH,
                    suffix = '.cpp',
                    src_suffix = '.jsall')

env.Append( BUILDERS={'JSHeader' : jshBuilder})


# --- targets ----

clientEnv = env.Clone();
clientEnv.Append( CPPPATH=["../"] )
clientEnv.Prepend( LIBS=[ "mongoclient"] )
clientEnv.Prepend( LIBPATH=["."] )
l = clientEnv[ "LIBS" ]
removeIfInList( l , "pcre" )
removeIfInList( l , "pcrecpp" )

testEnv = env.Clone()
testEnv.Append( CPPPATH=["../"] )
testEnv.Prepend( LIBS=[ "mongotestfiles" ] )
testEnv.Prepend( LIBPATH=["."] )


# ----- TARGETS ------

def checkErrorCodes():
    import buildscripts.errorcodes as x
    if x.checkErrorCodes() == False:
        print( "next id to use:" + str( x.getNextCode() ) )
        Exit(-1)

checkErrorCodes()

# main db target
mongod = env.Program( "mongod" , commonFiles + coreDbFiles + coreServerFiles + serverOnlyFiles + [ "db/db.cpp" ] )
Default( mongod )

# tools
allToolFiles = commonFiles + coreDbFiles + coreServerFiles + serverOnlyFiles + [ "client/gridfs.cpp", "tools/tool.cpp" ]
normalTools = [ "dump" , "restore" , "export" , "import" , "files" , "stat" ]
env.Alias( "tools" , [ add_exe( "mongo" + x ) for x in normalTools ] )
for x in normalTools:
    env.Program( "mongo" + x , allToolFiles + [ "tools/" + x + ".cpp" ] )

env.Program( "mongobridge" , allToolFiles + [ "tools/bridge.cpp" ] )

# mongos
mongos = env.Program( "mongos" , commonFiles + coreDbFiles + coreServerFiles + shardServerFiles )

# c++ library
clientLibName = str( env.Library( "mongoclient" , allClientFiles )[0] )
if GetOption( "sharedclient" ):
    sharedClientLibName = str( env.SharedLibrary( "mongoclient" , allClientFiles )[0] )
env.Library( "mongotestfiles" , commonFiles + coreDbFiles + coreServerFiles + serverOnlyFiles + ["client/gridfs.cpp"])
env.Library( "mongoshellfiles" , allClientFiles + coreServerFiles )

clientTests = []

# examples
clientTests += [ clientEnv.Program( "firstExample" , [ "client/examples/first.cpp" ] ) ]
clientTests += [ clientEnv.Program( "secondExample" , [ "client/examples/second.cpp" ] ) ]
clientTests += [ clientEnv.Program( "whereExample" , [ "client/examples/whereExample.cpp" ] ) ]
clientTests += [ clientEnv.Program( "authTest" , [ "client/examples/authTest.cpp" ] ) ]
clientTests += [ clientEnv.Program( "httpClientTest" , [ "client/examples/httpClientTest.cpp" ] ) ]

# testing
test = testEnv.Program( "test" , Glob( "dbtests/*.cpp" ) )
if windows:
    testEnv.Alias( "test" , "test.exe" )
perftest = testEnv.Program( "perftest", [ "dbtests/framework.cpp" , "dbtests/perf/perftest.cpp" ] )
clientTests += [ clientEnv.Program( "clientTest" , [ "client/examples/clientTest.cpp" ] ) ]

# --- sniffer ---
mongosniff_built = False
if darwin or clientEnv["_HAVEPCAP"]:
    mongosniff_built = True
    sniffEnv = clientEnv.Clone()
    if not windows:
        sniffEnv.Append( LIBS=[ "pcap" ] )
    else:
        sniffEnv.Append( LIBS=[ "wpcap" ] )
    sniffEnv.Program( "mongosniff" , "tools/sniffer.cpp" )

# --- shell ---

env.JSConcat( "shell/mongo.jsall"  , ["shell/utils.js","shell/db.js","shell/mongo.js","shell/mr.js","shell/query.js","shell/collection.js"] )
env.JSHeader( "shell/mongo.jsall" )

env.JSConcat( "shell/mongo-server.jsall"  , [ "shell/servers.js"] )
env.JSHeader( "shell/mongo-server.jsall" )


shellEnv = env.Clone();

if release and ( ( darwin and force64 ) or linux64 ):
    shellEnv["LINKFLAGS"] = env["LINKFLAGS_CLEAN"]
    shellEnv["LIBS"] = env["LIBS_CLEAN"]
    shellEnv["SLIBS"] = ""

if noshell:
    print( "not building shell" )
elif not onlyServer:
    weird = force64 and not windows and not solaris

    if weird:
        shellEnv["CFLAGS"].remove("-m64")
        shellEnv["CXXFLAGS"].remove("-m64")
        shellEnv["LINKFLAGS"].remove("-m64")
        shellEnv["CPPPATH"].remove( "/usr/64/include" )
        shellEnv["LIBPATH"].remove( "/usr/64/lib" )
        shellEnv.Append( CPPPATH=filterExists(["/sw/include" , "/opt/local/include"]) )
        shellEnv.Append( LIBPATH=filterExists(["/sw/lib/", "/opt/local/lib" , "/usr/lib"]) )

    l = shellEnv["LIBS"]

    removeIfInList( l , "pcre" )
    removeIfInList( l , "pcrecpp" )

    if windows:
        shellEnv.Append( LIBS=["winmm.lib"] )

    coreShellFiles = [ "shell/dbshell.cpp" , "shell/utils.cpp" , "shell/mongo-server.cpp" ]

    if weird:
        shell32BitFiles = coreShellFiles
        for f in allClientFiles:
            shell32BitFiles.append( "32bit/" + str( f ) )
        for f in scriptingFiles:
            shell32BitFiles.append( "32bit/" + str( f ) )
        shellEnv.VariantDir( "32bit" , "." )
        shellEnv.Append( CPPPATH=["32bit/"] )
    else:
        shellEnv.Prepend( LIBPATH=[ "." ] )

    shellEnv = doConfigure( shellEnv , needPcre=False , shell=True )

    if weird:
        mongo = shellEnv.Program( "mongo" , shell32BitFiles )
    else:
        shellEnv.Prepend( LIBS=[ "mongoshellfiles"] )
        mongo = shellEnv.Program( "mongo" , coreShellFiles )

    if weird:
        Depends( "32bit/shell/mongo.cpp" , "shell/mongo.cpp" )
        Depends( "32bit/shell/mongo-server.cpp" , "shell/mongo-server.cpp" )


#  ---- RUNNING TESTS ----

testEnv.Alias( "dummySmokeSideEffect", [], [] )

if GetOption( 'smokedbprefix') is not None:
    smokeDbPrefix = GetOption( 'smokedbprefix')
else:
    smokeDbPrefix = ''
    
def addSmoketest( name, deps, actions ):
    if type( actions ) == type( list() ):
        actions = [ testSetup ] + actions
    else:
        actions = [ testSetup, actions ]
    testEnv.Alias( name, deps, actions )
    testEnv.AlwaysBuild( name )
    # Prevent smoke tests from running in parallel
    testEnv.SideEffect( "dummySmokeSideEffect", name )

def ensureDir( name ):
    d = os.path.dirname( name )
    if not os.path.exists( d ):
        print( "Creating dir: " + name );
        os.makedirs( d )
        if not os.path.exists( d ):
            print( "Failed to create dir: " + name );
            Exit( 1 )

def ensureTestDirs():
    ensureDir( smokeDbPrefix + "/tmp/unittest/" )
    ensureDir( smokeDbPrefix + "/data/" )
    ensureDir( smokeDbPrefix + "/data/db/" )

def testSetup( env , target , source ):
    ensureTestDirs()

if len( COMMAND_LINE_TARGETS ) == 1 and str( COMMAND_LINE_TARGETS[0] ) == "test":
    ensureDir( smokeDbPrefix + "/tmp/unittest/" );

addSmoketest( "smoke", [ add_exe( "test" ) ] , [ test[ 0 ].abspath ] )
addSmoketest( "smokePerf", [ "perftest" ] , [ perftest[ 0 ].abspath ] )

clientExec = [ x[0].abspath for x in clientTests ]
def runClientTests( env, target, source ):
    global clientExec
    global mongodForTestsPort
    import subprocess
    for i in clientExec:
        if subprocess.call( [ i, "--port", mongodForTestsPort ] ) != 0:
            return True
    if subprocess.Popen( [ mongod[0].abspath, "msg", "ping", mongodForTestsPort ], stdout=subprocess.PIPE ).communicate()[ 0 ].count( "****ok" ) == 0:
        return True
    if subprocess.call( [ mongod[0].abspath, "msg", "ping", mongodForTestsPort ] ) != 0:
        return True
    return False
addSmoketest( "smokeClient" , clientExec, runClientTests )
addSmoketest( "mongosTest" , [ mongos[0].abspath ] , [ mongos[0].abspath + " --test" ] )

def jsSpec( suffix ):
    import os.path
    args = [ os.path.dirname( mongo[0].abspath ), "jstests" ] + suffix
    return apply( os.path.join, args )

def jsDirTestSpec( dir ):
    path = jsSpec( [ dir + '/*.js' ] )
    paths = [x.abspath for x in Glob( path ) ]
    return mongo[0].abspath + " --nodb " + ' '.join( paths )

def runShellTest( env, target, source ):
    global mongodForTestsPort
    import subprocess
    target = str( target[0] )
    if target == "smokeJs":
        spec = [ jsSpec( [ "_runner.js" ] ) ]
    elif target == "smokeQuota":
        g = Glob( jsSpec( [ "quota/*.js" ] ) )
        spec = [ x.abspath for x in g ]
    elif target == "smokeJsPerf":
        g = Glob( jsSpec( [ "perf/*.js" ] ) )
        spec = [ x.abspath for x in g ]
    elif target == "smokeJsSlow":
        spec = [x.abspath for x in Glob(jsSpec(["slow/*"]))]
    elif target == "smokeParallel":
        spec = [x.abspath for x in Glob(jsSpec(["parallel/*"]))]
    else:
        print( "invalid target for runShellTest()" )
        Exit( 1 )
    return subprocess.call( [ mongo[0].abspath, "--port", mongodForTestsPort ] + spec )

# These tests require the mongo shell
if not onlyServer and not noshell:
    addSmoketest( "smokeJs", [add_exe("mongo")], runShellTest )
    addSmoketest( "smokeClone", [ "mongo", "mongod" ], [ jsDirTestSpec( "clone" ) ] )
    addSmoketest( "smokeRepl", [ "mongo", "mongod", "mongobridge" ], [ jsDirTestSpec( "repl" ) ] )
    addSmoketest( "smokeDisk", [ add_exe( "mongo" ), add_exe( "mongod" ) ], [ jsDirTestSpec( "disk" ) ] )
    addSmoketest( "smokeAuth", [ add_exe( "mongo" ), add_exe( "mongod" ) ], [ jsDirTestSpec( "auth" ) ] )
    addSmoketest( "smokeParallel", [ add_exe( "mongo" ), add_exe( "mongod" ) ], runShellTest )
    addSmoketest( "smokeSharding", [ "mongo", "mongod", "mongos" ], [ jsDirTestSpec( "sharding" ) ] )
    addSmoketest( "smokeJsPerf", [ "mongo" ], runShellTest )
    addSmoketest("smokeJsSlow", [add_exe("mongo")], runShellTest)
    addSmoketest( "smokeQuota", [ "mongo" ], runShellTest )
    addSmoketest( "smokeTool", [ add_exe( "mongo" ) ], [ jsDirTestSpec( "tool" ) ] )

mongodForTests = None
mongodForTestsPort = "27017"

def startMongodWithArgs(*args):
    global mongodForTests
    global mongodForTestsPort
    global mongod
    if mongodForTests:
        return
    mongodForTestsPort = "32000"
    import os
    ensureTestDirs()
    dirName = smokeDbPrefix + "/data/db/sconsTests/"
    ensureDir( dirName )
    from subprocess import Popen
    mongodForTests = Popen([mongod[0].abspath, "--port", mongodForTestsPort,
                            "--dbpath", dirName] + list(args))

    if not utils.didMongodStart( 32000 ):
        print( "Failed to start mongod" )
        mongodForTests = None
        Exit( 1 )

def startMongodForTests( env, target, source ):
    return startMongodWithArgs()

def startMongodSmallOplog(env, target, source):
    return startMongodWithArgs("--master", "--oplogSize", "10")

def stopMongodForTests():
    global mongodForTests
    if not mongodForTests:
        return
    if mongodForTests.poll() is not None:
        print( "Failed to start mongod" )
        mongodForTests = None
        Exit( 1 )
    try:
        # This function not available in Python 2.5
        mongodForTests.terminate()
    except AttributeError:
        if windows:
            import win32process
            win32process.TerminateProcess(mongodForTests._handle, -1)
        else:
            from os import kill
            kill( mongodForTests.pid, 15 )
    mongodForTests.wait()

testEnv.Alias( "startMongod", [add_exe("mongod")], [startMongodForTests] );
testEnv.AlwaysBuild( "startMongod" );
testEnv.SideEffect( "dummySmokeSideEffect", "startMongod" )

testEnv.Alias( "startMongodSmallOplog", [add_exe("mongod")], [startMongodSmallOplog] );
testEnv.AlwaysBuild( "startMongodSmallOplog" );
testEnv.SideEffect( "dummySmokeSideEffect", "startMongodSmallOplog" )

def addMongodReqTargets( env, target, source ):
    mongodReqTargets = [ "smokeClient", "smokeJs" ]
    for target in mongodReqTargets:
        testEnv.Depends( target, "startMongod" )
        testEnv.Depends( "smokeAll", target )

testEnv.Alias( "addMongodReqTargets", [], [addMongodReqTargets] )
testEnv.AlwaysBuild( "addMongodReqTargets" )

testEnv.Alias( "smokeAll", [ "smoke", "mongosTest", "smokeClone", "smokeRepl", "addMongodReqTargets", "smokeDisk", "smokeAuth", "smokeSharding", "smokeTool" ] )
testEnv.AlwaysBuild( "smokeAll" )

def addMongodReqNoJsTargets( env, target, source ):
    mongodReqTargets = [ "smokeClient" ]
    for target in mongodReqTargets:
        testEnv.Depends( target, "startMongod" )
        testEnv.Depends( "smokeAllNoJs", target )

testEnv.Alias( "addMongodReqNoJsTargets", [], [addMongodReqNoJsTargets] )
testEnv.AlwaysBuild( "addMongodReqNoJsTargets" )

testEnv.Alias( "smokeAllNoJs", [ "smoke", "mongosTest", "addMongodReqNoJsTargets" ] )
testEnv.AlwaysBuild( "smokeAllNoJs" )

import atexit
atexit.register( stopMongodForTests )

def recordPerformance( env, target, source ):
    from buildscripts import benchmark_tools
    global perftest
    import subprocess, re
    p = subprocess.Popen( [ perftest[0].abspath ], stdout=subprocess.PIPE )
    b = p.communicate()[ 0 ]
    print( "perftest results:" );
    print( b );
    if p.returncode != 0:
        return True
    entries = re.findall( "{.*?}", b )
    import sys
    for e in entries:
        matches = re.match( "{'(.*?)': (.*?)}", e )
        name = matches.group( 1 )
        val = float( matches.group( 2 ) )
        sub = { "benchmark": { "project": "http://github.com/mongodb/mongo", "description": "" }, "trial": {} }
        sub[ "benchmark" ][ "name" ] = name
        sub[ "benchmark" ][ "tags" ] = [ "c++", re.match( "(.*)__", name ).group( 1 ) ]
        sub[ "trial" ][ "server_hash" ] = getGitVersion()
        sub[ "trial" ][ "client_hash" ] = ""
        sub[ "trial" ][ "result" ] = val
        try:
            print(benchmark_tools.post_data(sub))
        except:
            print( "exception posting perf results" )
            print( sys.exc_info() )
    return False

addSmoketest( "recordPerf", [ "perftest" ] , [ recordPerformance ] )

def run_shell_tests(env, target, source):
    from buildscripts import test_shell
    test_shell.mongo_path = windows and "mongo.exe" or "mongo"
    test_shell.run_tests()

env.Alias("test_shell", [], [run_shell_tests])
env.AlwaysBuild("test_shell")

#  ----  INSTALL -------

def getSystemInstallName():
    n = platform + "-" + processor
    if static:
        n += "-static"
    if nix and os.uname()[2].startswith( "8." ):
        n += "-tiger"

    try:
        import settings
        if "distmod" in dir( settings ):
            n = n + "-" + str( settings.distmod )
    except:
        pass

        
    dn = GetOption( "distmod" )
    if dn and len(dn) > 0:
        n = n + "-" + dn

    return n

def getCodeVersion():
    fullSource = open( "pch.cpp" , "r" ).read()
    allMatches = re.findall( r"versionString.. = \"(.*?)\"" , fullSource );
    if len(allMatches) != 1:
        print( "can't find version # in code" )
        return None
    return allMatches[0]

def getDistName( sofar ):
    global distName
    global dontReplacePackage

    if distName is not None:
        return distName

    if str( COMMAND_LINE_TARGETS[0] ) == "s3dist":
        version = getCodeVersion()
        if not version.endswith( "+" ) and not version.endswith("-"):
            print( "got real code version, doing release build for: " + version )
            dontReplacePackage = True
            distName = version
            return version


    return getGitBranchString( "" , "-" ) + today.strftime( "%Y-%m-%d" )


if distBuild:
    from datetime import date
    today = date.today()
    installDir = "mongodb-" + getSystemInstallName() + "-"
    installDir += getDistName( installDir )
    print "going to make dist: " + installDir

# binaries

def checkGlibc(target,source,env):
    import subprocess
    stringProcess = subprocess.Popen( [ "strings" , str( target[0] ) ] , stdout=subprocess.PIPE )
    stringResult = stringProcess.communicate()[0]
    if stringResult.count( "GLIBC_2.4" ) > 0:
        print( "************* " + str( target[0] ) + " has GLIBC_2.4 dependencies!" )
        Exit(-3)

allBinaries = []

def installBinary( e , name ):
    if not installSetup.binaries:
        return

    global allBinaries

    if windows:
        e.Alias( name , name + ".exe" )
        name += ".exe"

    inst = e.Install( installDir + "/bin" , name )

    fullInstallName = installDir + "/bin/" + name

    allBinaries += [ name ]
    if (solaris or linux) and (not GetOption("nostrip")):
        e.AddPostAction( inst, e.Action( 'strip ' + fullInstallName ) )

    if linux and len( COMMAND_LINE_TARGETS ) == 1 and str( COMMAND_LINE_TARGETS[0] ) == "s3dist":
        e.AddPostAction( inst , checkGlibc )

    if nix:
        e.AddPostAction( inst , e.Action( 'chmod 755 ' + fullInstallName ) )

for x in normalTools:
    installBinary( env , "mongo" + x )

if mongosniff_built:
    installBinary(env, "mongosniff")

installBinary( env , "mongod" )
installBinary( env , "mongos" )

if not noshell:
    installBinary( env , "mongo" )

env.Alias( "all" , allBinaries )
env.Alias( "core" , [ add_exe( "mongo" ) , add_exe( "mongod" ) , add_exe( "mongos" ) ] )

# NOTE: In some cases scons gets confused between installation targets and build
# dependencies.  Here, we use InstallAs instead of Install to prevent such confusion
# on a case-by-case basis.

#headers
if installSetup.headers:
    for id in [ "", "util/", "util/mongoutils/", "util/concurrency/", "db/" , "db/stats/" , "db/repl/" , "client/" , "bson/", "bson/util/" , "s/" , "scripting/" ]:
        env.Install( installDir + "/" + installSetup.headerRoot + "/mongo/" + id , Glob( id + "*.h" ) )
        env.Install( installDir + "/" + installSetup.headerRoot + "/mongo/" + id , Glob( id + "*.hpp" ) )

if installSetup.clientSrc:
    for x in allClientFiles:
        x = str(x)
        env.Install( installDir + "/mongo/" + x.rpartition( "/" )[0] , x )

#lib
if installSetup.binaries:
    env.Install( installDir + "/" + nixLibPrefix, clientLibName )

#textfiles
if installSetup.bannerDir:
    for x in os.listdir( installSetup.bannerDir ):
        full = installSetup.bannerDir + "/" + x
        if os.path.isdir( full ):
            continue
        if x.find( "~" ) >= 0:
            continue
        env.Install( installDir , full )

#final alias
env.Alias( "install" , installDir )

# aliases
if windows:
    env.Alias( "mongoclient" , "mongoclient.lib" )
else:
    env.Alias( "mongoclient" , "libmongoclient.a" )


#  ---- CONVENIENCE ----

def tabs( env, target, source ):
    from subprocess import Popen, PIPE
    from re import search, match
    diff = Popen( [ "git", "diff", "-U0", "origin", "master" ], stdout=PIPE ).communicate()[ 0 ]
    sourceFile = False
    for line in diff.split( "\n" ):
        if match( "diff --git", line ):
            sourceFile = not not search( "\.(h|hpp|c|cpp)\s*$", line )
        if sourceFile and match( "\+ *\t", line ):
            return True
    return False
env.Alias( "checkSource", [], [ tabs ] )
env.AlwaysBuild( "checkSource" )

def gitPush( env, target, source ):
    import subprocess
    return subprocess.call( [ "git", "push" ] )
env.Alias( "push", [ ".", "smoke", "checkSource" ], gitPush )
env.AlwaysBuild( "push" )


# ---- deploying ---

def s3push( localName , remoteName=None , remotePrefix=None , fixName=True , platformDir=True ):

    if remotePrefix is None:
        if distName is None:
            remotePrefix = getGitBranchString( "-" ) + "-latest"
        else:
            remotePrefix = "-" + distName

    sys.path.append( "." )
    sys.path.append( ".." )
    sys.path.append( "../../" )

    import simples3
    import settings

    s = simples3.S3Bucket( settings.bucket , settings.id , settings.key )

    if remoteName is None:
        remoteName = localName

    if fixName:
        (root,dot,suffix) = localName.rpartition( "." )
        name = remoteName + "-" + getSystemInstallName()
        name += remotePrefix
        if dot == "." :
            name += "." + suffix
        name = name.lower()
    else:
        name = remoteName

    if platformDir:
        name = platform + "/" + name

    print( "uploading " + localName + " to http://s3.amazonaws.com/" + s.name + "/" + name )
    if dontReplacePackage:
        for ( key , modify , etag , size ) in s.listdir( prefix=name ):
            print( "error: already a file with that name, not uploading" )
            Exit(2)
    s.put( name  , open( localName , "rb" ).read() , acl="public-read" );
    print( "  done uploading!" )

def s3shellpush( env , target , source ):
    s3push( "mongo" , "mongo-shell" )

env.Alias( "s3shell" , [ "mongo" ] , [ s3shellpush ] )
env.AlwaysBuild( "s3shell" )

def s3dist( env , target , source ):
    s3push( distFile , "mongodb" )

env.Append( TARFLAGS=" -z " )

if installDir[-1] != "/":
    if windows:
        distFile = installDir + ".zip"
        env.Zip( distFile , installDir )
    else:
        distFile = installDir + ".tgz"
        env.Tar( distFile , installDir )

    env.Alias( "dist" , distFile )
    env.Alias( "s3dist" , [ "install"  , distFile ] , [ s3dist ] )
    env.AlwaysBuild( "s3dist" )

def clean_old_dist_builds(env, target, source):
    prefix = "mongodb-%s-%s" % (platform, processor)
    filenames = sorted(os.listdir("."))
    filenames = [x for x in filenames if x.startswith(prefix)]
    to_keep = [x for x in filenames if x.endswith(".tgz") or x.endswith(".zip")][-2:]
    for filename in [x for x in filenames if x not in to_keep]:
        print "removing %s" % filename
        try:
            shutil.rmtree(filename)
        except:
            os.remove(filename)

env.Alias("dist_clean", [], [clean_old_dist_builds])
env.AlwaysBuild("dist_clean")

# --- an uninstall target ---
if len(COMMAND_LINE_TARGETS) > 0 and 'uninstall' in COMMAND_LINE_TARGETS:
    SetOption("clean", 1)
    # By inspection, changing COMMAND_LINE_TARGETS here doesn't do
    # what we want, but changing BUILD_TARGETS does.
    BUILD_TARGETS.remove("uninstall")
    BUILD_TARGETS.append("install")
