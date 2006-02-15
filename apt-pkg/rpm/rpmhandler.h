
/* ######################################################################

   RPM database and hdlist related handling

   ######################################################################
 */


#ifndef PKGLIB_RPMHANDLER_H
#define PKGLIB_RPMHANDLER_H

#include <apt-pkg/fileutl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>

#include <config.h>

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

class RPMHandler
{
   protected:

   unsigned int iOffset;
   unsigned int iSize;
   Header HeaderP;
   string ID;
   xmlNode *NodeP;

   string GetTag(rpmTag Tag);

   public:

   // Return a unique ID for that handler. Actually, implemented used
   // the file/dir name.
   virtual string GetID() { return ID; };

   virtual bool Skip() = 0;
   virtual bool Jump(unsigned int Offset) = 0;
   virtual void Rewind() = 0;
   inline unsigned Offset() {return iOffset;};
   virtual bool OrderedOffset() {return true;};
   inline unsigned Size() {return iSize;};
   inline Header GetHeader() {return HeaderP;};
   // doesn't belong here.. should abstract out the rpm header stuff
   xmlNode *GetNode() {return NodeP;};
   virtual bool IsDatabase() = 0;

   virtual string FileName() {return "";};
   virtual string Directory() {return "";};
   virtual unsigned long FileSize() {return 1;};
   virtual string MD5Sum() {return "";};
   virtual bool ProvideFileName() {return false;};

   virtual string Name() {return GetTag(RPMTAG_NAME);};
   virtual string Arch() {return GetTag(RPMTAG_ARCH);};
   virtual string Epoch() {return GetTag(RPMTAG_EPOCH);};
   virtual string Version() {return GetTag(RPMTAG_VERSION);};
   virtual string Release() {return GetTag(RPMTAG_RELEASE);};
   virtual string Group() {return GetTag(RPMTAG_GROUP);};
   virtual string Packager() {return GetTag(RPMTAG_PACKAGER);};
   virtual string Vendor() {return GetTag(RPMTAG_VENDOR);};
   virtual string Summary() {return GetTag(RPMTAG_SUMMARY);};
   virtual string Description() {return GetTag(RPMTAG_DESCRIPTION);};

   virtual bool HasFile(const char *File);

   virtual unsigned long InstalledSize() {return atol(GetTag(RPMTAG_SIZE).c_str());};

   RPMHandler() : iOffset(0), iSize(0), HeaderP(0) {};
   virtual ~RPMHandler() {};
};


class RPMFileHandler : public RPMHandler
{
   protected:

   FD_t FD;

   public:

   virtual bool Skip();
   virtual bool Jump(unsigned int Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

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
   virtual bool Jump(unsigned int Offset);
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
   virtual bool Jump(unsigned Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return true;};
   virtual bool HasWriteLock() {return WriteLock;};
   virtual time_t Mtime() {return DbFileMtime;}
   virtual bool OrderedOffset() {return false;};

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
   virtual bool Jump(unsigned int Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

   virtual string FileName() {return (Dir == NULL)?"":sFileName;};
   virtual unsigned long FileSize();
   virtual string MD5Sum();

   RPMDirHandler(string DirName);
   virtual ~RPMDirHandler();
};

class RPMRepomdHandler : public RPMHandler
{

   xmlDocPtr Primary;
   xmlNode *Root;

   xmlNode *FindNode(const string Name);

   string GetTag(string Tag);
   string GetTag(xmlNode *Node, string Tag);
   string GetProp(xmlNode *Node, char *Prop);

   public:


   virtual bool Skip();
   virtual bool Jump(unsigned int Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

   virtual string FileName();
   virtual string Directory();
   virtual unsigned long FileSize();
   virtual unsigned long InstalledSize();
   virtual string MD5Sum();

   virtual string Name() {return GetTag("name");};
   virtual string Arch() {return GetTag("arch");};
   virtual string Epoch();
   virtual string Version();
   virtual string Release();

   virtual string Group();
   virtual string Packager() {return GetTag("packager");};
   virtual string Vendor();
   virtual string Summary() {return GetTag("summary");};
   virtual string Description() {return GetTag("description");};

   virtual bool HasFile(const char *File);

   RPMRepomdHandler(string File);
   virtual ~RPMRepomdHandler();
};

#endif
