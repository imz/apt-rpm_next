// CNC:2002-07-03

// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: repository.cc,v 1.4 2002/07/29 18:13:52 niemeyer Exp $
/* ######################################################################

   Repository abstraction for 1 or more unique URI+Dist

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include <config.h>

#ifdef APT_WITH_REPOMD

#include <iostream>
#include <cstring>

#include <apt-pkg/error.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "xmlutil.h"
#include "repomd.h"

#include <apti18n.h>


using namespace std;

repomdXML::repomdXML(const string File) : Path(File)
{
   xmlDocPtr RepoMD;
   xmlNode *Root;

   RepoMD = xmlReadFile(File.c_str(), NULL, XML_PARSE_NONET);
   if ((Root = xmlDocGetRootElement(RepoMD)) == NULL) {
      xmlFreeDoc(RepoMD);
      _error->Error(_("Could not open metadata file '%s'"),File.c_str());
      return;
   }

   for (xmlNode *Node = Root->children; Node; Node = Node->next) {
      if (Node->type != XML_ELEMENT_NODE ||
	  strcmp((char*)Node->name, "data") != 0)
	 continue;

      string Hash = "";
      string HashType = "";
      string RealPath = "";
      string Path = "";
      string DataType = "";
      string TimeStamp = "";
      xmlNode *n = NULL;

      xmlChar *type = xmlGetProp(Node, (xmlChar*)"type");
      DataType = (char*)type;
      xmlFree(type);

      // If it's a sqlite _db file, see if we can use it and skip otherwise
      if (DataType.substr(DataType.size()-3, 3) == "_db") {
	 n = XmlFindNode(Node, "database_version");
	 bool db_supported = false;
	 if (n) {
	    xmlChar *x = xmlNodeGetContent(n);
	    int dbver = atoi((char*)x);
	    xmlFree(x);
	    // XXX should ask about supported version from sqlite handler
	    // or something instead of hardcoding in several places...
	    if (dbver == 10) {
	       db_supported = true;
	    }
	 }
	 if (db_supported == false) {
	    continue;
	 }
      }

      n = XmlFindNode(Node, "location");
      if (n) {
	 xmlChar *href = xmlGetProp(n, (xmlChar*)"href");
	 Path = (char*)href;
	 xmlFree(href);
      }

      n = NULL;
      RealPath = Path;
      if (flExtension(Path) == "gz" || flExtension(Path) == "bz2") {
	 Path = Path.substr(0, Path.size()-flExtension(Path).size()-1);
	 n = XmlFindNode(Node, "open-checksum");
      } else {
	 n = XmlFindNode(Node, "checksum");
      }
      if (n) {
	 xmlChar *hash = xmlNodeGetContent(n);
	 xmlChar *type = xmlGetProp(n, (xmlChar*)"type");
	 Hash = (char*)hash;
	 HashType = (char*)type;
	 xmlFree(hash);
	 xmlFree(type);
      }

      RepoFiles[DataType].Path = Path;
      RepoFiles[DataType].RealPath = RealPath;
      RepoFiles[DataType].TimeStamp = TimeStamp;
      RepoFiles[DataType].Hash = Hash;
      RepoFiles[DataType].HashType = chk2hash(HashType);
      RepoFiles[DataType].Size = 0;
   }

   xmlFreeDoc(RepoMD);
}

RPMHandler *repomdXML::CreateHandler() const
{
#ifdef WITH_SQLITE3
   if (RepoFiles.find("primary_db") != RepoFiles.end()) {
      return new RPMSqliteHandler(this);
   }
#endif
   return new RPMRepomdHandler(this);
}

string repomdXML::FindURI(string DataType) const
{
   string Res = "";
   map<string,RepoFile>::const_iterator I = RepoFiles.find(DataType);
   if (I != RepoFiles.end()) {
      Res = I->second.Path;
   }
   return Res;
}

string repomdXML::GetComprMethod(string Path) const
{
   string Res = "";
   map<string,RepoFile>::const_iterator I;
   for (I = RepoFiles.begin(); I != RepoFiles.end(); I++) {
      if (Path == flUnCompressed(I->second.RealPath)) {
	 Res = flExtension(I->second.RealPath);
      }
   }
   return Res;
}

// Parse repomd.xml file for checksums
bool repomdRepository::ParseRelease(string File)
{
   repomd = new repomdXML(File);
   string htype = "";

   map<string,repomdXML::RepoFile>::const_iterator I;
   for (I = repomd->RepoFiles.begin(); I != repomd->RepoFiles.end(); I++) {
      IndexChecksums[I->second.Path].Hash = I->second.Hash;
      IndexChecksums[I->second.Path].HashType = I->second.HashType;
      IndexChecksums[I->second.Path].Size = 0;
   }

   GotRelease = true;

   return true;
}

string repomdRepository::FindURI(string DataType)
{
   return repomd->FindURI(DataType);
}

string repomdRepository::GetComprMethod(string URI)
{
   string Path = string(URI,RootURI.size());
   return repomd->GetComprMethod(Path);

}

#endif /* APT_WITH_REPOMD */

// vim:sts=3:sw=3
