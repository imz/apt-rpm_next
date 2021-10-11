// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: rpmsystem.cc,v 1.9 2002/11/25 18:25:28 niemeyer Exp $
/* ######################################################################

   System - Abstraction for running on different systems.

   RPM version of the system stuff
   
   ##################################################################### 
 */
									/*}}}*/
// Include Files							/*{{{*/
#include <config.h>

#ifdef HAVE_RPM

#include "rpmsystem.h"
#include "rpmversion.h"
#include "rpmindexfile.h"
#include "rpmpm.h"
#include "rpmhandler.h"
#include "rpmpackagedata.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>

#include <apti18n.h>
    
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <rpm/rpmlib.h>
#include <assert.h>
#include <time.h>
									/*}}}*/
extern int _rpmds_nopromote;

rpmSystem rpmSys;

// System::rpmSystem - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmSystem::rpmSystem()
{
   LockCount = 0;
   RpmDB = NULL;
   StatusFile = NULL;
   Label = "rpm interface";
   VS = &rpmVS;
   rpmReadConfigFiles(NULL, NULL);
}
									/*}}}*/
rpmSystem::~rpmSystem()
{
   delete StatusFile;
   delete RpmDB;
   rpmFreeRpmrc();
}

RPMDBHandler *rpmSystem::GetDBHandler()
{
   if (RpmDB == NULL)
      RpmDB = new RPMDBHandler();
   return RpmDB;
}

bool rpmSystem::LockRead()
{
   GetDBHandler();
   LockCount++;
   if (_error->PendingError() == true)
      return false;
   return true;
}

//
// System::Lock - Get the lock						/*{{{*/
// ---------------------------------------------------------------------
/* this will open the rpm database through rpmlib, which will lock the db */
bool rpmSystem::Lock()
{
   if (RpmDB != NULL && RpmDB->HasWriteLock() == false)
   {
      delete RpmDB;
      RpmDB = NULL;
   }
   if (RpmDB == NULL)
      RpmDB = new RPMDBHandler(true);
   if (_error->PendingError() == true)
      return false;
   LockCount++;
   return true;
}
									/*}}}*/
// System::UnLock - Drop a lock						/*{{{*/
// ---------------------------------------------------------------------
/* Close the rpmdb, effectively dropping it's lock */
bool rpmSystem::UnLock(bool NoErrors)
{
   if (LockCount == 0 && NoErrors == true)
      return false;
   if (LockCount < 1)
      return _error->Error("Not locked");
   if (--LockCount == 0)
   {
      delete RpmDB;
      RpmDB = NULL;
   }
   return true;
}
									/*}}}*/
// System::CreatePM - Create the underlying package manager		/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgPackageManager *rpmSystem::CreatePM(pkgDepCache *Cache) const
{
   if (_config->Find("RPM::PM", "internal") == "internal")
      return new pkgRPMLibPM(Cache);
   else
      return new pkgRPMExtPM(Cache);
}
									/*}}}*/
// System::Initialize - Setup the configuration space..			/*{{{*/
// ---------------------------------------------------------------------
/* These are the rpm specific configuration variables.. */
bool rpmSystem::Initialize(Configuration &Cnf)
{
   Cnf.CndSet("Dir::Bin::rpm","/bin/rpm");
   Cnf.CndSet("Dir::Etc::rpmpriorities", "rpmpriorities");
   Cnf.CndSet("Dir::Etc::translatelist", "translate.list");
   Cnf.CndSet("Dir::Etc::translateparts", "translate.list.d");
   Cnf.CndSet("Dir::State::prefetch", "prefetch");
   Cnf.CndSet("Acquire::CDROM::Mount", "/media/cdrom");
   Cnf.CndSet("Acquire::CDROM::Copy-All", "true");

   const char *RPMOptions[] =
   {
      "RPM::Options",
      "RPM::Install-Options",
      "RPM::Erase-Options",
      NULL,
   };
   int NoPromote = 1;
   const char **Opt = RPMOptions;
   const Configuration::Item *Top;
   while (*Opt && NoPromote)
   {
      Top = _config->Tree(*Opt);
      if (Top != 0)
      {
	 for (Top = Top->Child; Top != 0; Top = Top->Next)
	    if (Top->Value == "--promoteepoch") {
	       NoPromote = 0;
	       break;
	    }
      }
      Opt++;
   }
   _rpmds_nopromote = NoPromote;
   HideZeroEpoch = (NoPromote == 1);

   return true;
}
									/*}}}*/
// System::ArchiveSupported - Is a file format supported		/*{{{*/
// ---------------------------------------------------------------------
/* The standard name for a rpm is 'rpm'.. There are no seperate versions
   of .rpm to worry about.. */
bool rpmSystem::ArchiveSupported(const char *Type)
{
   if (strcmp(Type,"rpm") == 0)
      return true;
   return false;
}
									/*}}}*/
// System::Score - Determine how Re**at'ish this sys is..	        /*{{{*/
// ---------------------------------------------------------------------
/* Check some symptoms that this is a Re**at like system */
signed rpmSystem::Score(Configuration const &Cnf)
{
   signed Score = 0;

   if (FileExists(RPMDBHandler::DataPath(false)))
      Score += 10;
   if (FileExists(Cnf.FindFile("Dir::Bin::rpm","/bin/rpm")) == true)
      Score += 10;

   return Score;
}

string rpmSystem::DistroVer()
{
   string DistroVerPkg = _config->Find("APT::DistroVerPkg", "");
   if (DistroVerPkg.empty() || LockRead() == false)
      return "";

   string DistroVersion = "";
   if (RpmDB->JumpByName(DistroVerPkg, true) == true) {
      DistroVersion = RpmDB->Version();
   } else {
      _error->Error(_("Unable to determine version for package %s"),
		      DistroVerPkg.c_str());
   }
   UnLock(true);

   return DistroVersion;
}

									/*}}}*/
// System::AddStatusFiles - Register the status files			/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmSystem::AddStatusFiles(vector<pkgIndexFile *> &List)
{
   if (StatusFile == NULL)
      StatusFile = new rpmDatabaseIndex();
   List.push_back(StatusFile);
   return true;
}
									/*}}}*/
// System::AddSourceFiles - Register aditional source files		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmSystem::AddSourceFiles(vector<pkgIndexFile *> &List)
{
   const Configuration::Item *Top;
   Top = _config->Tree("APT::Arguments");
   if (Top != 0)
   {
      for (Top = Top->Child; Top != 0; Top = Top->Next) {
	 const string &S = Top->Value;
	 if (FileExists(S) && flExtension(S) == "rpm")
	 {
	    if (S.length() > 8 && string(S, S.length()-8) == ".src.rpm")
	       List.push_back(new rpmSingleSrcIndex(S));
	    else
	       List.push_back(new rpmSinglePkgIndex(S));
	 }
      }
   }
   return true;
}
									/*}}}*/
// System::FindIndex - Get an index file for status files		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmSystem::FindIndex(pkgCache::PkgFileIterator File,
			  pkgIndexFile *&Found) const
{
   if (StatusFile == 0)
      return false;
   if (StatusFile->FindInCache(*File.Cache()) == File)
   {
      Found = StatusFile;
      return true;
   }
   
   return false;
}
									/*}}}*/

// System::ProcessCache - Do specific changes in the cache  		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmSystem::ProcessCache(pkgDepCache &Cache,pkgProblemResolver &Fix)
{
   RPMPackageData *rpmdata = RPMPackageData::Singleton();
   for (pkgCache::PkgIterator I = Cache.PkgBegin(); I.end() == false; I++)
   {
      // Ignore virtual packages
      if (I->VersionList == 0)
	 continue;
	 
      // Do package holding
      if (I->CurrentVer != 0)
      {
	 if (rpmdata->HoldPackage(I.Name()))
	 {
	    Cache.MarkKeep(I);
	    Fix.Protect(I);
	 }
      }
   }
   return true;
}
									/*}}}*/

// System::IgnoreDep - Check if this dependency should be ignored       /*{{{*/
// ---------------------------------------------------------------------
/* For strong hearts only */
bool rpmSystem::IgnoreDep(pkgVersioningSystem &VS,pkgCache::DepIterator &Dep)
{
   RPMPackageData *rpmdata = RPMPackageData::Singleton();
   return rpmdata->IgnoreDep(VS,Dep);
}
									/*}}}*/

// System::CacheBuilt - free caches used during cache build		/*{{{*/
// ---------------------------------------------------------------------
/* */
void rpmSystem::CacheBuilt()
{
   RPMPackageData *rpmdata = RPMPackageData::Singleton();
   rpmdata->CacheBuilt();
}
									/*}}}*/

// System::OptionsHash - Identify options which change the cache	/*{{{*/
// ---------------------------------------------------------------------
/* */
static void HashString(unsigned long &Hash, const char *Str)
{
   for (const char *I = Str; *I != 0; I++)
      Hash = 5*Hash + *I;
}
static void HashEnv(unsigned long &Hash, const char *Name)
{
   const char *Value = getenv(Name);
   if (Value)
      HashString(Hash, Value);
}
static void HashOption(unsigned long &Hash, const char *Name)
{
   const Configuration::Item *Top = _config->Tree(Name);
   if (Top != 0)
      HashString(Hash, Top->Value.c_str());
}
static void HashOptionTree(unsigned long &Hash, const char *Name)
{
   const Configuration::Item *Top = _config->Tree(Name);
   if (Top != 0)
      for (Top = Top->Child; Top != 0; Top = Top->Next)
	 HashString(Hash, Top->Value.c_str());
}
static void HashOptionFile(unsigned long &Hash, const char *Name)
{
   string FileName = _config->FindFile(Name);
   struct stat st;
   stat(FileName.c_str(), &st);
   Hash += st.st_mtime;
}

unsigned long rpmSystem::OptionsHash() const
{
   unsigned long Hash = 0;
   HashOption(Hash, "RPM::Architecture");
   HashOptionTree(Hash, "RPM::Allow-Duplicated");
   HashOptionTree(Hash, "RPM::Ignore");
   HashOptionFile(Hash, "Dir::Etc::rpmpriorities");
   // FIXME: the whole RPM::Multilib::<basearch> tree should be hashed,
   // but HashOptionTree doesn't recurse so it's useless for this at the moment
   // HashOptionTree(Hash, "RPM::Multilib");
   HashEnv(Hash, "LANG");
   HashEnv(Hash, "LC_ALL");
   HashEnv(Hash, "LC_MESSAGES");
   return Hash;
}
									/*}}}*/

#endif /* HAVE_RPM */

// vim:sts=3:sw=3
