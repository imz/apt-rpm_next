// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: pkgrecords.cc,v 1.8 2003/09/02 04:52:16 mdz Exp $
/* ######################################################################
   
   Package Records - Allows access to complete package description records
                     directly from the file.
     
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/pkgrecords.h"
#endif
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/indexfile.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
    
#include <apti18n.h>   
									/*}}}*/
using namespace std;

// Records::pkgRecords - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* This will create the necessary structures to access the status files */
pkgRecords::pkgRecords(pkgCache &Cache) : Cache(Cache), Files(0)
{
   Files = new Parser *[Cache.HeaderP->PackageFileCount];
   memset(Files,0,sizeof(*Files)*Cache.HeaderP->PackageFileCount);
   
   for (pkgCache::PkgFileIterator I = Cache.FileBegin(); 
	I.end() == false; I++)
   {
      const pkgIndexFile::Type *Type = pkgIndexFile::Type::GetType(I.IndexType());
      if (Type == 0)
      {
	 _error->Error(_("Index file type '%s' is not supported"),I.IndexType());
	 return;
      }

      Files[I->ID] = Type->CreatePkgParser(I);
      if (Files[I->ID] == 0)
	 return;
   }   
   
   // CNC:2002-11-28
   // We store that to make sure that the destructor won't segfault,
   // even if the Cache object was destructed before this instance.
   PackageFileCount = Cache.HeaderP->PackageFileCount;
}
									/*}}}*/
// Records::~pkgRecords - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgRecords::~pkgRecords()
{
   // CNC:2002-11-28
   // See comments above.
   for (unsigned I = 0; I != (unsigned)PackageFileCount; I++)
      delete Files[I];
   delete [] Files;
}
									/*}}}*/
// Records::Lookup - Get a parser for the package version file		/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgRecords::Parser &pkgRecords::Lookup(pkgCache::VerFileIterator const &Ver)
{
   Files[Ver.File()->ID]->Jump(Ver);
   return *Files[Ver.File()->ID];
}
									/*}}}*/
