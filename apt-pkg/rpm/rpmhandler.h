
/* ######################################################################

   RPM database and hdlist related handling

   ######################################################################
 */


#ifndef PKGLIB_RPMHANDLER_H
#define PKGLIB_RPMHANDLER_H

#include <apt-pkg/aptconf.h>
#include <apt-pkg/fileutl.h>

#ifdef APT_WITH_REPOMD
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <sqlite3.h>
#endif

#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>

#include <vector>

// Our Extra RPM tags. These should not be accessed directly. Use
// the methods in RPMHandler instead.
#define CRPMTAG_FILENAME          (rpmTag)1000000
#define CRPMTAG_FILESIZE          (rpmTag)1000001
#define CRPMTAG_MD5               (rpmTag)1000005
#define CRPMTAG_SHA1              (rpmTag)1000006

#define CRPMTAG_DIRECTORY         (rpmTag)1000010
#define CRPMTAG_BINARY            (rpmTag)1000011

#define CRPMTAG_UPDATE_SUMMARY    (rpmTag)1000020
#define CRPMTAG_UPDATE_IMPORTANCE (rpmTag)1000021
#define CRPMTAG_UPDATE_DATE       (rpmTag)1000022
#define CRPMTAG_UPDATE_URL        (rpmTag)1000023

using namespace std;

struct Dependency
{
   string Name;
   string Version;
   unsigned int Op;
   unsigned int Type;
};

class RPMHandler
{
   protected:

   off_t iOffset;
   off_t iSize;
   Header HeaderP;
   string ID;

   string GetSTag(rpmTag Tag);
   unsigned long GetITag(rpmTag Tag);
   unsigned int DepOp(int_32 rpmflags);
   bool InternalDep(const char *name, const char *ver, int_32 flag);
   bool PutDep(const char *name, const char *ver, int_32 flags,
               unsigned int type, vector<Dependency*> &Deps);

   public:

   // Return a unique ID for that handler. Actually, implemented used
   // the file/dir name.
   virtual string GetID() { return ID; };

   virtual bool Skip() = 0;
   virtual bool Jump(off_t Offset) = 0;
   virtual void Rewind() = 0;
   inline unsigned Offset() {return iOffset;};
   virtual bool OrderedOffset() {return true;};
   inline unsigned Size() {return iSize;};
   virtual bool IsDatabase() = 0;

   virtual string FileName() {return "";};
   virtual string Directory() {return "";};
   virtual unsigned long FileSize() {return 1;};
   virtual string MD5Sum() {return "";};
   virtual string SHA1Sum() {return "";};
   virtual bool ProvideFileName() {return false;};

   virtual string Name() {return GetSTag(RPMTAG_NAME);};
   virtual string Arch() {return GetSTag(RPMTAG_ARCH);};
   virtual string Epoch();
   virtual string Version() {return GetSTag(RPMTAG_VERSION);};
   virtual string Release() {return GetSTag(RPMTAG_RELEASE);};
   virtual string EVR();
   virtual string Group() {return GetSTag(RPMTAG_GROUP);};
   virtual string Packager() {return GetSTag(RPMTAG_PACKAGER);};
   virtual string Vendor() {return GetSTag(RPMTAG_VENDOR);};
   virtual string Summary() {return GetSTag(RPMTAG_SUMMARY);};
   virtual string Description() {return GetSTag(RPMTAG_DESCRIPTION);};
   virtual unsigned long InstalledSize() {return GetITag(RPMTAG_SIZE);};
   virtual string SourceRpm() {return GetSTag(RPMTAG_SOURCERPM);};
   virtual bool IsSourceRpm() {return SourceRpm().empty();}

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps);
   virtual bool FileList(vector<string> &FileList);

   virtual bool HasFile(const char *File);

   RPMHandler() : iOffset(0), iSize(0), HeaderP(0) {};
   virtual ~RPMHandler() {};
};


class RPMFileHandler : public RPMHandler
{
   protected:

   FD_t FD;

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};
   virtual bool OrderedOffset() {return true;};

   virtual string FileName();
   virtual string Directory();
   virtual unsigned long FileSize();
   virtual string MD5Sum();

   RPMFileHandler(FileFd *File);
   RPMFileHandler(string File);
   virtual ~RPMFileHandler();
};

class RPMSingleFileHandler : public RPMFileHandler
{
   protected:

   string sFilePath;

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();

   virtual string FileName() {return sFilePath;};
   virtual string Directory() {return "";};
   virtual unsigned long FileSize();
   virtual string MD5Sum();
   virtual bool ProvideFileName() {return true;};

   RPMSingleFileHandler(string File) : RPMFileHandler(File), sFilePath(File) {};
   virtual ~RPMSingleFileHandler() {};
};

class RPMDBHandler : public RPMHandler
{
   protected:

#if RPM_VERSION >= 0x040100
   rpmts Handler;
#else
   rpmdb Handler;
#endif
#if RPM_VERSION >= 0x040000
   rpmdbMatchIterator RpmIter;
#endif
   bool WriteLock;

   time_t DbFileMtime;

   public:

   static string DataPath(bool DirectoryOnly=true);
   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return true;};
   virtual bool HasWriteLock() {return WriteLock;};
   virtual time_t Mtime() {return DbFileMtime;}
   virtual bool OrderedOffset() {return false;};

   // used by rpmSystem::DistroVer()
   bool JumpByName(string PkgName);

   RPMDBHandler(bool WriteLock=false);
   virtual ~RPMDBHandler();
};

class RPMDirHandler : public RPMHandler
{
   protected:

   DIR *Dir;
   string sDirName;
   string sFileName;
   string sFilePath;

#if RPM_VERSION >= 0x040100
   rpmts TS;
#endif

   const char *nextFileName();

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

   virtual string FileName() {return (Dir == NULL)?"":sFileName;};
   virtual unsigned long FileSize();
   virtual string MD5Sum();

   RPMDirHandler(string DirName);
   virtual ~RPMDirHandler();
};

#ifdef APT_WITH_REPOMD
class RPMRepomdHandler : public RPMHandler
{

   xmlDocPtr Primary;
   xmlNode *Root;
   xmlNode *NodeP;

   vector<xmlNode *> Pkgs;
   vector<xmlNode *>::iterator PkgIter;

   string PrimaryFile;

   xmlNode *FindNode(const string Name);
   xmlNode *FindNode(xmlNode *Node, const string Name);

   string FindTag(xmlNode *Node, const string Tag);
   string GetProp(xmlNode *Node, char *Prop);

   public:


   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

   virtual string FileName();
   virtual string Directory();
   virtual unsigned long FileSize();
   virtual unsigned long InstalledSize();
   virtual string MD5Sum();
   virtual string SHA1Sum();

   virtual string Name() {return FindTag(NodeP, "name");};
   virtual string Arch() {return FindTag(NodeP, "arch");};
   virtual string Epoch();
   virtual string Version();
   virtual string Release();

   virtual string Group();
   virtual string Packager() {return FindTag(NodeP, "packager");};
   virtual string Vendor();
   virtual string Summary() {return FindTag(NodeP, "summary");};
   virtual string Description() {return FindTag(NodeP, "description");};
   virtual string SourceRpm();

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps);
   virtual bool FileList(vector<string> &FileList);

   RPMRepomdHandler(string File);
   virtual ~RPMRepomdHandler();
};

class RPMRepomdFLHandler : public RPMHandler
{
   protected:
   xmlTextReaderPtr Filelist;
   string FilelistFile;
   xmlNode *NodeP;

   string FindTag(char *Tag);

   public:
   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual bool IsDatabase() {return false;};

   virtual string FileName() {return FilelistFile;};
   virtual string Directory() {return "";};
   virtual unsigned long FileSize() {return 0;};
   virtual unsigned long InstalledSize() {return 0;};
   virtual string MD5Sum() {return "";};
   virtual string SHA1Sum() {return "";};

   virtual string Name() {return FindTag("name");};
   virtual string Arch() {return FindTag("arch");};
   virtual string Epoch() {return FindTag("epoch");};
   virtual string Version() {return FindTag("version");};
   virtual string Release() {return FindTag("release");};
   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps)
       {return true;};

   virtual bool FileList(vector<string> &FileList);
   RPMRepomdFLHandler(string File);
   virtual ~RPMRepomdFLHandler();
};

class RPMSqliteHandler : public RPMHandler
{
   protected:

   sqlite3 *db;
   sqlite3 *filedb;
   sqlite3 *otherdb;
   string DBPath;
   string FilesDBPath;
   string OtherDBPath;

   vector<off_t> Pkgs;
   vector<off_t>::iterator PkgIter;

   string FindTag(string Tag);

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

   virtual string FileName();
   virtual string Directory();
   virtual unsigned long FileSize();
   virtual unsigned long InstalledSize();
   virtual string MD5Sum();
   virtual string SHA1Sum();

   virtual string Name();
   virtual string Arch();
   virtual string Epoch();
   virtual string Version();
   virtual string Release();

   virtual string Group();
   virtual string Packager();
   virtual string Vendor();
   virtual string Summary();
   virtual string Description();
   virtual string SourceRpm();

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps);
   virtual bool FileList(vector<string> &FileList);

   RPMSqliteHandler(string File);
   virtual ~RPMSqliteHandler();
};
#endif /* APT_WITH_REPOMD */

#endif
