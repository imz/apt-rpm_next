// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: rpmpm.h,v 1.4 2003/01/29 13:52:32 niemeyer Exp $
/* ######################################################################

   rpm Package Manager - Provide an interface to rpm

   #####################################################################
 */
									/*}}}*/
#ifndef PKGLIB_rpmPM_H
#define PKGLIB_rpmPM_H

#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
									/*}}}*/
typedef Header rpmHeader;

#include <apt-pkg/packagemanager.h>
#include <vector>

using std::string;
using std::vector;

class pkgRPMPM : public pkgPackageManager
{
   protected:

   struct Item
   {
      enum Ops {Install, Configure, Remove, Purge} Op;
      enum RPMOps {RPMInstall, RPMUpgrade, RPMErase};
      string File;
      PkgIterator Pkg;
      Item(Ops Op,PkgIterator Pkg,string File = "")
	 : Op(Op), File(File), Pkg(Pkg) {}
      Item() {}

   };
   vector<Item> List;

   // Helpers
   bool RunScripts(const char *Cnf);
   bool RunScriptsWithPkgs(const char *Cnf);

   // The Actuall installation implementation
   virtual bool Install(PkgIterator Pkg,string File);
   virtual bool Configure(PkgIterator Pkg);
   virtual bool Remove(PkgIterator Pkg,bool Purge = false);

   virtual bool Process(vector<const char*> &install,
			vector<const char*> &upgrade,
			vector<const char*> &uninstall) {return false;}

   virtual bool Go();
   virtual void Reset();

   public:

   pkgRPMPM(pkgDepCache *Cache);
   virtual ~pkgRPMPM();
};

class pkgRPMExtPM : public pkgRPMPM
{
   protected:
   bool ExecRPM(Item::RPMOps op, vector<const char*> &files);
   virtual bool Process(vector<const char*> &install,
			vector<const char*> &upgrade,
			vector<const char*> &uninstall);

   public:
   pkgRPMExtPM(pkgDepCache *Cache);
   virtual ~pkgRPMExtPM();
};

class pkgRPMLibPM : public pkgRPMPM
{
   protected:
   rpmts TS;

   bool ParseRpmOpts(const char *Cnf, int *tsFlags, int *probFilter);
   bool AddToTransaction(Item::RPMOps op, vector<const char*> &files);
   virtual bool Process(vector<const char*> &install,
			vector<const char*> &upgrade,
			vector<const char*> &uninstall);

   public:

   pkgRPMLibPM(pkgDepCache *Cache);
   virtual ~pkgRPMLibPM();
};

#endif

// vim:sts=3:sw=3
