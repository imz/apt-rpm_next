#include <map>
#include <sstream>
#include <rpm/rpmlib.h>
#include <apti18n.h>

#include <apt-pkg/progress.h>
#include "aptcallback.h"
#include "raptheader.h"

#include <iostream>
using namespace std;

static const char *copyTags[] = {"name", 
			   "version", 
			   "release", 
			   "arch", 
			   "summary", 
			   NULL};

static void getPackageData(const Header h, map<string,string> &Data)
{
   raptHeader hdr(h);
   Data.clear();
   for (const char **Tag = &copyTags[0]; *Tag != NULL; *Tag++) {
      ostringstream fmt;
      fmt << "%{" << *Tag << "}";
      Data[*Tag] = hdr.format(fmt.str());
   }
}

/* TODO: test number of arguments in configure instead */
#if RPM_VERSION >= 0x04085a
#define FDLINK(_fd) fdLink((_fd))
#define FDFREE(_fd) fdFree((_fd))
#else
#define FDLINK(_fd) fdLink((_fd), "")
#define FDFREE(_fd) fdFree((_fd), "")
#endif

void * rpmCallback(const void * arg, 
		   const rpmCallbackType what,
                   const raptCallbackSize amount,
                   const raptCallbackSize total,
		   const void * pkgKey, void * data)

{
   Header h = (Header) arg;

   InstProgress *Prog = (InstProgress*)data;
   void * rc = NULL;
   const char * filename = (const char *) pkgKey;
   static FD_t fd = NULL;
   static rpmCallbackType state;
   static bool repackage;
   static map<string,string> Data;

   switch (what) {
   case RPMCALLBACK_INST_OPEN_FILE:
      if (filename == NULL || filename[0] == '\0')
	 return NULL;
      fd = Fopen(filename, "r.ufdio");
      if (fd)
	 fd = FDLINK(fd);
      return fd;
      break;

   case RPMCALLBACK_INST_CLOSE_FILE:
      fd = FDFREE(fd);
      if (fd) {
	 (void) Fclose(fd);
	 fd = NULL;
      }
      break;

   case RPMCALLBACK_INST_START:
      if (state != what && repackage == false) {
	 state = what;
	 Prog->OverallProgress(0,1,1, "Updating / installing");
	 Prog->SetState(InstProgress::Installing);
      }

      getPackageData(h, Data);
      Prog->SubProgress(total, Data["name"]);
      Prog->Progress(amount);
      break;

   case RPMCALLBACK_TRANS_PROGRESS:
   case RPMCALLBACK_INST_PROGRESS:
      Prog->Progress(amount);
      break;

   case RPMCALLBACK_TRANS_START:
      state = what;
      repackage = false;
      Prog->SetState(InstProgress::Preparing);
      Prog->SubProgress(total, "Preparing");
      Prog->Progress(0);
      Prog->SetPackageData(&Data);
   break;

   case RPMCALLBACK_TRANS_STOP:
      Prog->Progress(total);
      break;

#if RPM_VERSION >= 0x040100
   case RPMCALLBACK_REPACKAGE_START:
      repackage = true;
      Prog->OverallProgress(0,1,1, "Repackaging");
      Prog->SetState(InstProgress::Repackaging);
      break;

   case RPMCALLBACK_REPACKAGE_PROGRESS:
      Prog->Progress(amount);
      break;

   case RPMCALLBACK_REPACKAGE_STOP:
      repackage = false;
      break;
#endif

   case RPMCALLBACK_UNINST_PROGRESS:
      break;

   case RPMCALLBACK_UNINST_START:
      if (state != what) {
	 state = what;
	 Prog->OverallProgress(0,1,1, "Cleaning up / removing");
	 Prog->SetState(InstProgress::Removing);
      }
      if (h == NULL) {
	 break;
      }
      getPackageData(h, Data);
      Prog->SubProgress(100, Data["name"]);
      Prog->Progress(0);
      break;

   case RPMCALLBACK_UNINST_STOP:
      Prog->Progress(100);
      if (h == NULL)
	 break;
      getPackageData(h, Data);
      break;
   default: // Fall through
      break;
 
   }
   return rc;
}	

// vim:sts=3:sw=3
