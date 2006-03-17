// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: rpmsrcrecords.cc,v 1.9 2003/01/29 15:19:02 niemeyer Exp $
/* ######################################################################

   SRPM Records - Parser implementation for RPM style source indexes

   #####################################################################
 */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/rpmsrcrecords.h"
#endif

#include <config.h>

#ifdef HAVE_RPM

#include <assert.h>

#include <apt-pkg/rpmsrcrecords.h>
#include <apt-pkg/error.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/rpmhandler.h>
#include <apt-pkg/pkgcache.h>

#include <apti18n.h>

#if RPM_VERSION >= 0x040100
#include <rpm/rpmds.h>
#endif

using namespace std;

// SrcRecordParser::rpmSrcRecordParser - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmSrcRecordParser::rpmSrcRecordParser(string File,pkgIndexFile const *Index)
    : Parser(Index), Buffer(0), BufSize(0), BufUsed(0)
{
   struct stat Buf;
   if (stat(File.c_str(),&Buf) == 0 && S_ISDIR(Buf.st_mode))
      Handler = new RPMDirHandler(File);
   else if (flExtension(File) == "rpm")
      Handler = new RPMSingleFileHandler(File);
   else if (flExtension(File) == "xml")
      Handler = new RPMRepomdHandler(File);
   else
      Handler = new RPMFileHandler(File);
}
									/*}}}*/
// SrcRecordParser::~rpmSrcRecordParser - Destructor			/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmSrcRecordParser::~rpmSrcRecordParser()
{
   delete Handler;
   free(Buffer);
}
									/*}}}*/
// SrcRecordParser::Binaries - Return the binaries field		/*{{{*/
// ---------------------------------------------------------------------
/* This member parses the binaries field into a pair of class arrays and
   returns a list of strings representing all of the components of the
   binaries field. The returned array need not be freed and will be
   reused by the next Binaries function call. */
const char **rpmSrcRecordParser::Binaries()
{
   return NULL;

// WTF is this ?!? If we're looking for sources why would be interested
// in binaries? Maybe there's an inner Zen to this all but
// apt-cache showsrc seems to work without just fine so disabled for now...
#if 0
   int i = 0;
   char **bins;
   int type, count;
   assert(HeaderP != NULL);
   int rc = headerGetEntry(HeaderP, CRPMTAG_BINARY,
			   &type, (void**)&bins, &count);
   if (rc != 1)
       return NULL;
   for (i = 0; (unsigned)i < sizeof(StaticBinList)/sizeof(char*) && i < count;
        i++)
      StaticBinList[i] = bins[i];
   StaticBinList[i] = 0;
   return StaticBinList;
#endif
}
									/*}}}*/
// SrcRecordParser::Files - Return a list of files for this source	/*{{{*/
// ---------------------------------------------------------------------
/* This parses the list of files and returns it, each file is required to have
   a complete source package */
bool rpmSrcRecordParser::Files(vector<pkgSrcRecords::File> &List)
{
   List.clear();

   pkgSrcRecords::File F;

   F.MD5Hash = Handler->MD5Sum();
   F.Size = Handler->FileSize();
   F.Path = flCombine(Handler->Directory(), Handler->FileName());
   F.Type = "srpm";

   List.push_back(F);

   return true;
}
									/*}}}*/

bool rpmSrcRecordParser::Restart()
{
   Handler->Rewind();
   return true;
}

bool rpmSrcRecordParser::Step()
{
   return Handler->Skip();
}

bool rpmSrcRecordParser::Jump(unsigned long Off)
{
   return Handler->Jump(Off);
}

string rpmSrcRecordParser::Package() const
{
   return Handler->Name();
}

string rpmSrcRecordParser::Version() const
{
   string e, v, r, verstr;
   e = Handler->Epoch();
   v = Handler->Version();
   r = Handler->Release();

   if (e.empty() == false)
      verstr = e + ":" + v + "-" + r;
   else
      verstr = v + "-" + r;
   return verstr;
}


// RecordParser::Maintainer - Return the maintainer email		/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmSrcRecordParser::Maintainer() const
{
   return Handler->Packager();
}

string rpmSrcRecordParser::Section() const
{
   return Handler->Group();
}

unsigned long rpmSrcRecordParser::Offset()
{
    return Handler->Offset();
}

void rpmSrcRecordParser::BufCat(const char *text)
{
   if (text != NULL)
      BufCat(text, text+strlen(text));
}

void rpmSrcRecordParser::BufCat(const char *begin, const char *end)
{
   unsigned len = end - begin;

   if (BufUsed+len+1 >= BufSize)
   {
      BufSize += 512;
      char *tmp = (char*)realloc(Buffer, BufSize);
      if (tmp == NULL)
      {
	 _error->Errno("realloc", _("Could not allocate buffer for record text"));
	 return;
      }
      Buffer = tmp;
   }

   strncpy(Buffer+BufUsed, begin, len);
   BufUsed += len;
}

void rpmSrcRecordParser::BufCatTag(const char *tag, const char *value)
{
   BufCat(tag);
   BufCat(value);
}

void rpmSrcRecordParser::BufCatDep(const char *pkg, const char *version, int flags)
{
   char buf[16];
   char *ptr = (char*)buf;

   BufCat(pkg);
   if (*version)
   {
      int c = 0;
      *ptr++ = ' ';
      *ptr++ = '(';
      if (flags & RPMSENSE_LESS)
      {
	 *ptr++ = '<';
	 c = '<';
      }
      if (flags & RPMSENSE_GREATER)
      {
	 *ptr++ = '>';
	 c = '>';
      }
      if (flags & RPMSENSE_EQUAL)
      {
	 *ptr++ = '=';
      }/* else {
	 if (c)
	   fputc(c, f);
      }*/
      *ptr++ = ' ';
      *ptr = '\0';

      BufCat(buf);
      BufCat(version);
      BufCat(")");
   }
}

void rpmSrcRecordParser::BufCatDescr(const char *descr)
{
   const char *begin = descr;

   while (*descr)
   {
      if (*descr=='\n')
      {
	 BufCat(" ");
	 BufCat(begin, descr+1);
	 begin = descr+1;
      }
      descr++;
   }
   BufCat(" ");
   BufCat(begin, descr);
   BufCat("\n");
}

// SrcRecordParser::AsStr - The record in raw text
// -----------------------------------------------
string rpmSrcRecordParser::AsStr()
{
   int type, type2, type3, count;
   char *str;
   char **strv;
   char **strv2;
   int_32 *numv;
   char buf[32];

   BufUsed = 0;

   BufCatTag("Package: ", Handler->Name().c_str());

   BufCatTag("\nSection: ", Handler->Group().c_str());

   snprintf(buf, sizeof(buf), "%d", Handler->InstalledSize());
   BufCatTag("\nInstalled Size: ", buf);

   BufCatTag("\nPackager: ", Handler->Packager().c_str());
   //BufCatTag("\nVendor: ", Handler->Vendor().c_str());

   BufCat("\nVersion: ");
   // XXX FIXME: handle the epoch madness somewhere central instead of
   // figuring it out on every damn occasion separately

   string e, v, r, verstr;
   e = Handler->Epoch();
   v = Handler->Version();
   r = Handler->Release();

   if (e.empty() == false)
      verstr = e + ":" + v + "-" + r;
   else
      verstr = v + "-" + r;

   BufCat(verstr.c_str());

//   headerGetEntry(HeaderP, RPMTAG_DISTRIBUTION, &type, (void **)&str, &count);//   fprintf(f, "Distribution: %s\n", str);

// XXX FIXME: handle dependencies in handler as well
// XXX missing the BIIIIIIIIG if 0 section wrt dependencies
// .. here
// ..

   BufCatTag("\nArchitecture: ", Handler->Arch().c_str());

   snprintf(buf, sizeof(buf), "%d", Handler->FileSize());
   BufCatTag("\nSize: ", buf);

   BufCatTag("\nMD5Sum: ", Handler->MD5Sum().c_str());

   BufCatTag("\nFilename: ", Handler->FileName().c_str());

   BufCatTag("\nSummary: ", Handler->Summary().c_str());
   BufCat("\nDescription: ");
   BufCat("\n");
   BufCatDescr(Handler->Description().c_str());
   BufCat("\n");

   return string(Buffer, BufUsed);
}


// SrcRecordParser::BuildDepends - Return the Build-Depends information	/*{{{*/
// ---------------------------------------------------------------------
bool rpmSrcRecordParser::BuildDepends(vector<pkgSrcRecords::Parser::BuildDepRec> &BuildDeps,
				      bool ArchOnly)
{
   // FIXME: This method is leaking memory from headerGetEntry().
   int RpmTypeTag[] = {RPMTAG_REQUIRENAME,
		       RPMTAG_REQUIREVERSION,
		       RPMTAG_REQUIREFLAGS,
		       RPMTAG_CONFLICTNAME,
		       RPMTAG_CONFLICTVERSION,
		       RPMTAG_CONFLICTFLAGS};
   int BuildType[] = {pkgSrcRecords::Parser::BuildDepend,
		      pkgSrcRecords::Parser::BuildConflict};
   BuildDepRec rec;

   BuildDeps.clear();

// XXXX FIXME .. get the deps from Handler
#if 0
   for (unsigned char Type = 0; Type != 2; Type++)
   {
      char **namel = NULL;
      char **verl = NULL;
      int *flagl = NULL;
      int res, type, count;

      res = headerGetEntry(HeaderP, RpmTypeTag[0+Type*3], &type,
			 (void **)&namel, &count);
      if (res != 1)
	 return true;
      res = headerGetEntry(HeaderP, RpmTypeTag[1+Type*3], &type,
			 (void **)&verl, &count);
      res = headerGetEntry(HeaderP, RpmTypeTag[2+Type*3], &type,
			 (void **)&flagl, &count);

      for (int i = 0; i < count; i++)
      {
#if RPM_VERSION >= 0x040404
         if (namel[i][0] == 'g' && strncmp(namel[i], "getconf", 7) == 0)
         {
            rpmds getconfProv = NULL;
            rpmds ds = rpmdsSingle(RPMTAG_PROVIDENAME,
                                   namel[i], verl?verl[i]:NULL, flagl[i]);
            rpmdsGetconf(&getconfProv, NULL);
            int res = rpmdsSearch(getconfProv, ds) >= 0;
            rpmdsFree(ds);
            rpmdsFree(getconfProv);
            if (res) continue;
         }
#endif
	 if (strncmp(namel[i], "rpmlib", 6) == 0)
	 {
#if RPM_VERSION >= 0x040404
	    rpmds rpmlibProv = NULL;
	    rpmds ds = rpmdsSingle(RPMTAG_PROVIDENAME,
				   namel[i], verl?verl[i]:NULL, flagl[i]);
	    rpmdsRpmlib(&rpmlibProv, NULL);
	    int res = rpmdsSearch(rpmlibProv, ds) >= 0;
	    rpmdsFree(ds);
	    rpmdsFree(rpmlibProv);
#elif RPM_VERSION >= 0x040100
	    rpmds ds = rpmdsSingle(RPMTAG_PROVIDENAME,
				   namel[i], verl?verl[i]:NULL, flagl[i]);
	    int res = rpmCheckRpmlibProvides(ds);
	    rpmdsFree(ds);
#else
	    int res = rpmCheckRpmlibProvides(namel[i], verl?verl[i]:NULL,
					     flagl[i]);
#endif
	    if (res) continue;
	 }

	 if (verl)
	 {
	    if (!*verl[i])
	       rec.Op = pkgCache::Dep::NoOp;
	    else
	    {
	       if (flagl[i] & RPMSENSE_LESS)
	       {
		  if (flagl[i] & RPMSENSE_EQUAL)
		      rec.Op = pkgCache::Dep::LessEq;
		  else
		      rec.Op = pkgCache::Dep::Less;
	       }
	       else if (flagl[i] & RPMSENSE_GREATER)
	       {
		  if (flagl[i] & RPMSENSE_EQUAL)
		      rec.Op = pkgCache::Dep::GreaterEq;
		  else
		      rec.Op = pkgCache::Dep::Greater;
	       }
	       else if (flagl[i] & RPMSENSE_EQUAL)
		  rec.Op = pkgCache::Dep::Equals;
	    }

	    rec.Version = verl[i];
	 }
	 else
	 {
	    rec.Op = pkgCache::Dep::NoOp;
	    rec.Version = "";
	 }

	 rec.Type = BuildType[Type];
	 rec.Package = namel[i];
	 BuildDeps.push_back(rec);
      }
   }
#endif
   return true;
}
									/*}}}*/
#endif /* HAVE_RPM */

// vim:sts=3:sw=3
