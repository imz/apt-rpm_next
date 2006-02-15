// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   RPM Package Records - Parser for RPM package records

   #####################################################################
 */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/rpmrecords.h"
#endif

#include <config.h>

#ifdef HAVE_RPM

#include <assert.h>

#include <apt-pkg/rpmrecords.h>
#include <apt-pkg/error.h>
#include <apt-pkg/rpmhandler.h>
#include <apt-pkg/rpmsystem.h>

#include <apti18n.h>

using namespace std;

// RecordParser::rpmRecordParser - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmRecordParser::rpmRecordParser(string File, pkgCache &Cache)
   : Handler(0), HeaderP(0), Buffer(0), BufSize(0), BufUsed(0)
{
   if (File == RPMDBHandler::DataPath(false)) {
      IsDatabase = true;
      Handler = rpmSys.GetDBHandler();
   } else {
      IsDatabase = false;
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
}
									/*}}}*/
// RecordParser::~rpmRecordParser - Destructor				/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmRecordParser::~rpmRecordParser()
{
   // Can't use Handler->IsDatabase here, since the RPMDBHandler
   // could already have been destroyed.
   if (IsDatabase == false)
      delete Handler;
   free(Buffer);
}
									/*}}}*/
// RecordParser::Jump - Jump to a specific record			/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmRecordParser::Jump(pkgCache::VerFileIterator const &Ver)
{
   Handler->Jump(Ver->Offset);
   HeaderP = Handler->GetHeader();
   return (HeaderP != NULL);
}
									/*}}}*/
// RecordParser::FileName - Return the archive filename on the site	/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::FileName()
{
   string Dir = Handler->Directory();
   if (Dir.empty() == true)
      return Handler->FileName();
   return flCombine(Dir, Handler->FileName());
}
									/*}}}*/
// RecordParser::Name - Return the package name				/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::Name()
{
   return Handler->Name();
}
									/*}}}*/
// RecordParser::MD5Hash - Return the archive hash			/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::MD5Hash()
{
   return Handler->MD5Sum();
}
									/*}}}*/
// RecordParser::Maintainer - Return the maintainer email		/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::Maintainer()
{
   return Handler->Packager();
}
									/*}}}*/
// RecordParser::ShortDesc - Return a 1 line description		/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::ShortDesc()
{
   return Handler->Summary();
}
									/*}}}*/
// RecordParser::LongDesc - Return a longer description			/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::LongDesc()
{
   return Handler->Description();
// XXX FIXME: is all this stuff *really* necessary?
#if 0
   char *str, *ret, *x, *y;
   int_32 count, type;
   int len;
   str = Handler->Description(); // typecast problems...

   // Count size plus number of newlines
   for (x = str, len = 0; *x; x++, len++)
      if (*x == '\n')
	 len++;

   ret = (char*)malloc(len+1);
   if (ret == NULL)
      return "out of mem";

   // Copy string, inserting a space after each newline
   for (x = str, y = ret; *x; x++, y++)
   {
      *y = *x;
      if (*x == '\n')
	 *++y = ' ';
   }
   *y = 0;

   // Remove spaces and newlines from end of string
   for (y--; y > ret && (*y == ' ' || *y == '\n'); y--)
      *y = 0;

   string Ret = string(ret);
   free(ret);

   return Ret;
#endif
}
									/*}}}*/
// RecordParser::SourcePkg - Return the source package name if any	/*{{{*/
// ---------------------------------------------------------------------
/* */
string rpmRecordParser::SourcePkg()
{
   // This must be the *package* name, not the *file* name. We have no
   // current way to extract it safely from the file name.
   return "";
}
									/*}}}*/

void rpmRecordParser::BufCat(const char *text)
{
   if (text != NULL)
      BufCat(text, text+strlen(text));
}

void rpmRecordParser::BufCat(const char *begin, const char *end)
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

void rpmRecordParser::BufCatTag(const char *tag, const char *value)
{
   BufCat(tag);
   BufCat(value);
}

void rpmRecordParser::BufCatDep(const char *pkg,
			        const char *version,
				int flags)
{
   char buf[16];
   char *ptr = buf;

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

void rpmRecordParser::BufCatDescr(const char *descr)
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


// RecordParser::GetRec - The record in raw text, in std Debian format	/*{{{*/
// ---------------------------------------------------------------------
void rpmRecordParser::GetRec(const char *&Start,const char *&Stop)
{
   // FIXME: This method is leaking memory from headerGetEntry().
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
   BufCatTag("\nMaintainer: ", Handler->Vendor().c_str());

   BufCat("\nVersion: ");
   // XXX FIXME: handle the epoch madness somewhere central instead of
   // figuring it out on every damn occasion separately

   string e, v, r, verstr;
   e = Handler->Epoch();
   v = Handler->Version();
   r = Handler->Release();

   verstr = v + "-" + r;
   if (e.empty() == false)
      verstr = e + ":" + verstr;
   BufCat(verstr.c_str());

//   headerGetEntry(HeaderP, RPMTAG_DISTRIBUTION, &type, (void **)&str, &count);
//   fprintf(f, "Distribution: %s\n", str);

// XXX FIXME: handle dependencies in handler as well
#if 0
   headerGetEntry(HeaderP, RPMTAG_REQUIRENAME, &type, (void **)&strv, &count);
   assert(type == RPM_STRING_ARRAY_TYPE || count == 0);

   headerGetEntry(HeaderP, RPMTAG_REQUIREVERSION, &type2, (void **)&strv2, &count);
   headerGetEntry(HeaderP, RPMTAG_REQUIREFLAGS, &type3, (void **)&numv, &count);

   if (count > 0)
   {
      int i, j;

      for (j = i = 0; i < count; i++)
      {
	 if ((numv[i] & RPMSENSE_PREREQ))
	 {
	    if (j == 0)
		BufCat("\nPre-Depends: ");
	    else
		BufCat(", ");
	    BufCatDep(strv[i], strv2[i], numv[i]);
	    j++;
	 }
      }

      for (j = 0, i = 0; i < count; i++)
      {
	 if (!(numv[i] & RPMSENSE_PREREQ))
	 {
	    if (j == 0)
		BufCat("\nDepends: ");
	    else
		BufCat(", ");
	    BufCatDep(strv[i], strv2[i], numv[i]);
	    j++;
	 }
      }
   }

   headerGetEntry(HeaderP, RPMTAG_CONFLICTNAME, &type, (void **)&strv, &count);
   assert(type == RPM_STRING_ARRAY_TYPE || count == 0);

   headerGetEntry(HeaderP, RPMTAG_CONFLICTVERSION, &type2, (void **)&strv2, &count);
   headerGetEntry(HeaderP, RPMTAG_CONFLICTFLAGS, &type3, (void **)&numv, &count);

   if (count > 0)
   {
      BufCat("\nConflicts: ");
      for (int i = 0; i < count; i++)
      {
	 if (i > 0)
	     BufCat(", ");
	 BufCatDep(strv[i], strv2[i], numv[i]);
      }
   }

   headerGetEntry(HeaderP, RPMTAG_PROVIDENAME, &type, (void **)&strv, &count);
   assert(type == RPM_STRING_ARRAY_TYPE || count == 0);

   headerGetEntry(HeaderP, RPMTAG_PROVIDEVERSION, &type2, (void **)&strv2, &count);
   headerGetEntry(HeaderP, RPMTAG_PROVIDEFLAGS, &type3, (void **)&numv, &count);

   if (count > 0)
   {
      BufCat("\nProvides: ");
      for (int i = 0; i < count; i++)
      {
	 if (i > 0)
	     BufCat(", ");
	 BufCatDep(strv[i], strv2[i], numv[i]);
      }
   }

   headerGetEntry(HeaderP, RPMTAG_OBSOLETENAME, &type, (void **)&strv, &count);
   assert(type == RPM_STRING_ARRAY_TYPE || count == 0);

   headerGetEntry(HeaderP, RPMTAG_OBSOLETEVERSION, &type2, (void **)&strv2, &count);
   headerGetEntry(HeaderP, RPMTAG_OBSOLETEFLAGS, &type3, (void **)&numv, &count);
   if (count > 0) {
      BufCat("\nObsoletes: ");
      for (int i = 0; i < count; i++)
      {
	 if (i > 0)
	     BufCat(", ");
	 BufCatDep(strv[i], strv2[i], numv[i]);
      }
   }
#endif

   BufCatTag("\nArchitecture: ", Handler->Arch().c_str());

   snprintf(buf, sizeof(buf), "%d", Handler->FileSize());
   BufCatTag("\nSize: ", buf);

   BufCatTag("\nMD5Sum: ", Handler->MD5Sum().c_str());

   BufCatTag("\nFilename: ", Handler->FileName().c_str());

   BufCatTag("\nSummary: ", Handler->Summary().c_str());
   BufCatTag("\nDescription: ", str);
   BufCat("\n");
   BufCatDescr(Handler->Description().c_str());
   BufCat("\n");

   Start = Buffer;
   Stop = Buffer + BufUsed;
}
									/*}}}*/

bool rpmRecordParser::HasFile(const char *File)
{
   if (*File == '\0')
      return false;
   char **names = NULL;
   int_32 count = 0;
   rpmHeaderGetEntry(HeaderP, RPMTAG_OLDFILENAMES,
		     NULL, (void **) &names, &count);
   while (count--)
   {
      char *name = names[count];
      if (strcmp(name, File) == 0)
	 return true;
   }
   free(names);
   return false;
}

#endif /* HAVE_RPM */

// vim:sts=3:sw=3
