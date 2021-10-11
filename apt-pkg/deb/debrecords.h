// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: debrecords.h,v 1.1 2002/07/23 17:54:51 niemeyer Exp $
/* ######################################################################
   
   Debian Package Records - Parser for debian package records
   
   This provides display-type parsing for the Packages file. This is 
   different than the the list parser which provides cache generation
   services. There should be no overlap between these two.
   
   ##################################################################### */
									/*}}}*/
#ifndef PKGLIB_DEBRECORDS_H
#define PKGLIB_DEBRECORDS_H

#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/tagfile.h>

class debRecordParser : public pkgRecords::Parser
{
   FileFd File;
   pkgTagFile Tags;
   pkgTagSection Section;
   
   protected:
   
   virtual bool Jump(pkgCache::VerFileIterator const &Ver);
   
   public:

   // These refer to the archive file for the Version
   virtual string FileName();
   virtual string MD5Hash();
   virtual string SHA1Hash();
   virtual string SourcePkg();
   
   // These are some general stats about the package
   virtual string Maintainer();
   virtual string ShortDesc();
   virtual string LongDesc();
   virtual string Name();

   virtual void GetRec(const char *&Start,const char *&Stop);
   
   debRecordParser(string FileName,pkgCache &Cache);
};

#endif
