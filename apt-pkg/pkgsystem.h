// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: pkgsystem.h,v 1.4 2003/01/29 18:43:48 niemeyer Exp $
/* ######################################################################

   System - Abstraction for running on different systems.

   Instances of this class can be thought of as factories or meta-classes
   for a variety of more specialized classes. Together this class and
   it's speciallized offspring completely define the environment and how
   to access resources for a specific system. There are several sub
   areas that are all orthogonal - each system has a unique combination of
   these sub areas:
       - Versioning. Different systems have different ideas on versions.
         Within a system all sub classes must follow the same versioning
         rules.
       - Local tool locking to prevent multiple tools from accessing the
         same database.
       - Candidate Version selection policy - this is probably almost always
         managed using a standard APT class
       - Actual Package installation
         * Indication of what kind of binary formats are supported
       - Selection of local 'status' indexes that make up the pkgCache.

   It is important to note that the handling of index files is not a
   function of the system. Index files are handled through a seperate
   abstraction - the only requirement is that the index files have the
   same idea of versioning as the target system.

   Upon startup each supported system instantiates an instance of the
   pkgSystem class (using a global constructor) which will make itself
   available to the main APT init routine. That routine will select the
   proper system and make it the global default.

   ##################################################################### */
									/*}}}*/
#ifndef PKGLIB_PKGSYSTEM_H
#define PKGLIB_PKGSYSTEM_H

#include <apt-pkg/depcache.h>
#include <vector>

class pkgPackageManager;
class pkgVersioningSystem;
class Configuration;
class pkgIndexFile;

// CNC:2002-07-05
class pkgProblemResolver;

class pkgSystem
{
   public:

   // Global list of supported systems
   static pkgSystem **GlobalList;
   static unsigned long GlobalListLen;
   static pkgSystem *GetSystem(const char *Label);

   const char *Label;
   pkgVersioningSystem *VS;

   /* Prevent other programs from touching shared data not covered by
      other locks (cache or state locks) */
   virtual bool Lock() = 0;
   virtual bool UnLock(bool NoErrors = false) = 0;

   // CNC:2002-07-06
   virtual bool LockRead() {return true;}

   // CNC:2003-03-07 - Signal to system that the cache has been built.
   virtual void CacheBuilt() {}

   /* Various helper classes to interface with specific bits of this
      environment */
   virtual pkgPackageManager *CreatePM(pkgDepCache *Cache) const = 0;

   /* Load environment specific configuration and perform any other setup
      necessary */
   virtual bool Initialize(Configuration &/*Cnf*/) {return true;}

   /* Type is some kind of Globally Unique way of differentiating
      archive file types.. */
   virtual bool ArchiveSupported(const char *Type) = 0;

   // Return a list of system index files..
   virtual bool AddStatusFiles(std::vector<pkgIndexFile *> &List) = 0;
   // CNC:2003-11-21
   virtual bool AddSourceFiles(std::vector<pkgIndexFile *> &List) {return true;}
   virtual bool FindIndex(pkgCache::PkgFileIterator File,
			  pkgIndexFile *&Found) const = 0;

   /* Evauluate how 'right' we are for this system based on the filesystem
      etc.. */
   virtual signed Score(Configuration const &/*Cnf*/) {return 0;}

   // LORG:2006-02-06 Distro version from package system
   virtual string DistroVer() {return "";}

   // CNC:2002-07-03
   // Do environment specific pre-processing over the Index Files
   virtual bool PreProcess(pkgIndexFile **Start,pkgIndexFile **End,
                           OpProgress &Progress) {return true;}
   virtual bool ProcessCache(pkgDepCache &Cache, pkgProblemResolver &Fix)
	{return true;}
   virtual bool IgnoreDep(pkgVersioningSystem &VS, pkgCache::DepIterator &Dep)
	{return false;}

   // CNC:2003-11-24
   virtual unsigned long OptionsHash() const {return 0;}

   pkgSystem();
   virtual ~pkgSystem() {}
};

// The environment we are operating in.
extern pkgSystem *_system;

#endif
