// CNC:2002-07-03

// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: repository.cc,v 1.4 2002/07/29 18:13:52 niemeyer Exp $
/* ######################################################################

   Repository abstraction for 1 or more unique URI+Dist

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/repomd.h"
#endif

#include <iostream>
#include <apt-pkg/repomd.h>
#include <apt-pkg/error.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <apti18n.h>

using namespace std;

// Repository::ParseRelease - Parse Release file for checksums		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool repomdRepository::ParseRelease(string File)
{
   //cout << "parsing repomd repository release " << File << endl;
   RepoMD = xmlReadFile(File.c_str(), NULL, XML_PARSE_NONET);
   if ((Root = xmlDocGetRootElement(RepoMD)) == NULL) {
      xmlFreeDoc(RepoMD);
      return _error->Error(_("could not open Release file '%s'"),File.c_str());
   }
   //cout << "opened repomd ok" << endl;

   GotRelease = true;

   return true;
}

// Repository::FindChecksums - Get checksum info for file		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool repomdRepository::FindChecksums(string URI,unsigned long &Size, string &MD5)
{
   cout << "XXX findchecks " << URI << endl;
   string Path = string(URI,RootURI.size());
   return true;
}
									/*}}}*/
									/*}}}*/
// vim:sts=3:sw=3
