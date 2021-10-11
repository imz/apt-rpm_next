// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: rpmlistparser.h,v 1.2 2002/07/26 17:39:28 niemeyer Exp $
/* ######################################################################
   
   RPM Package List Parser - This implements the abstract parser 
   interface for RPM package files
   
   ##################################################################### 
 */
									/*}}}*/
// Header section: pkglib
#ifndef PKGLIB_RPMLISTPARSER_H
#define PKGLIB_RPMLISTPARSER_H

#include <apt-pkg/aptconf.h>
#include <apt-pkg/pkgcachegen.h>
#include "rpmhandler.h"
#include "rpmmisc.h"

#ifdef APT_WITH_REPOMD
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif

#include <map>
#include <vector>
#include <regex.h>

using std::map;
using std::vector;
using std::string;

class RPMHandler;
class RPMPackageData;

class rpmListParser : public pkgCacheGenerator::ListParser
{
   RPMHandler *Handler;
   RPMPackageData *RpmData;

   string CurrentName;
   const pkgCache::VerIterator *VI;
   
#ifdef APT_WITH_GNU_HASH_MAP
   typedef hash_map<const char*,bool,
   		    hash<const char*>,cstr_eq_pred> SeenPackagesType;
#else
   typedef map<const char*,bool,cstr_lt_pred> SeenPackagesType;
#endif
   SeenPackagesType *SeenPackages;

   bool Duplicated;
   
   bool ParseStatus(pkgCache::PkgIterator Pkg,pkgCache::VerIterator Ver);
   bool ParseDepends(pkgCache::VerIterator Ver, unsigned int Type);
   bool ParseProvides(pkgCache::VerIterator Ver);
   
 public:
   
   // These all operate against the current header
   virtual string Package();
   virtual string Version();
   virtual string Architecture();
   virtual bool NewVersion(pkgCache::VerIterator Ver);
   virtual unsigned short VersionHash();
   virtual bool UsePackage(pkgCache::PkgIterator Pkg,
			   pkgCache::VerIterator Ver);
   virtual off_t Offset()
	{return Handler->Offset();}
   virtual off_t Size();

   virtual bool OrderedOffset()
   	{return Handler->OrderedOffset();}

   virtual bool IsDatabase()
   	{return Handler->IsDatabase();}

   virtual bool CollectFileProvides(pkgCache &Cache,
				    pkgCache::VerIterator Ver); 
   virtual bool Step();
   
   bool LoadReleaseInfo(pkgCache::PkgFileIterator FileI,FileFd &File);

   void VirtualizePackage(string Name);
   void CompatArchPackage(string Name);
   
   rpmListParser(RPMHandler *Handler);
   ~rpmListParser();
};

#ifdef APT_WITH_REPOMD
class rpmRepomdParser : public rpmListParser
{
   protected:

   string Primary;
   string Filelist;
   string Other;

   xmlNode *FindNode(xmlNode *n, const string Name);

   public:
 
   bool LoadReleaseInfo(pkgCache::PkgFileIterator FileI,const string File,
		   	const string Dist);

   rpmRepomdParser(RPMHandler *Handler) : rpmListParser(Handler) {}
};
#endif /* APT_WITH_REPOMD */

#endif
