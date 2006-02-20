// CNC:2002-07-03

#ifndef PKGLIB_REPOMD_H
#define PKBLIB_REPOMD_H

#ifdef __GNUG__
#pragma interface "apt-pkg/repomd.h"
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <apt-pkg/repository.h>


class repomdRepository : public pkgRepository
{
   protected:

   bool GotRelease;
   xmlDocPtr RepoMD;
   xmlNode *Root;

   public:

   virtual bool HasRelease() { return GotRelease; };
   virtual bool IsAuthenticated() const { return false; };
   virtual bool ParseRelease(string File);
   virtual bool FindChecksums(string URI,unsigned long &Size, string &MD5);

   repomdRepository(string URI,string Dist, const pkgSourceList::Vendor *Vendor,
		 string RootURI)
      : pkgRepository(URI, Dist, Vendor, RootURI) {};

};

#endif

// vim:sts=3:sw=3
