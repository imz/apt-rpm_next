// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: rpmversion.cc,v 1.4 2002/11/19 13:03:29 niemeyer Exp $
/* ######################################################################

   RPM Version - Versioning system for RPM

   This implements the standard RPM versioning system.
   
   ##################################################################### 
 */
									/*}}}*/
// Include Files							/*{{{*/
#include <config.h>

#ifdef HAVE_RPM

#include "rapttypes.h"
#include "rpmversion.h"
#include <apt-pkg/pkgcache.h>

#include <rpm/rpmlib.h>

#include <stdlib.h>
#include <assert.h>

#include <rpm/rpmds.h>

rpmVersioningSystem rpmVS;

// rpmVS::rpmVersioningSystem - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmVersioningSystem::rpmVersioningSystem()
{
   Label = "Standard .rpm";
}
									/*}}}*/
// rpmVS::ParseVersion - Parse a version into it's components           /*{{{*/
// ---------------------------------------------------------------------
/* Code ripped from rpmlib */
static void ParseVersion(char *evr,
			 const char **Epoch, 
			 const char **Version, 
			 const char **Release)
{
   const char *epoch = NULL;
   const char *version = NULL;
   const char *release = NULL;
   char *s;

   s = strrchr(evr, '-');
   if (s) {
      *s++ = '\0';
      release = s;
   }
   s = evr;
   while (isdigit(*s)) s++;
   if (*s == ':')
   {
      epoch = evr;
      *s++ = '\0';
      version = s;
      if (*epoch == '\0') epoch = "0";
   }
   else
   {
      epoch = "0";
      version = evr;
   }

   *Epoch = epoch;
   *Version = version;
   *Release = release;
}
									/*}}}*/
// rpmVS::CmpVersion - Comparison for versions				/*{{{*/
// ---------------------------------------------------------------------
/* This fragments the version into E:V-R triples and compares each 
   portion separately. */
int rpmVersioningSystem::DoCmpVersion(const char *A,const char *AEnd,
				      const char *B,const char *BEnd)
{
   size_t alen = AEnd-A;
   size_t blen = BEnd-B;
   char AVer[alen+1], BVer[blen+1];
   const char *AE, *AV, *AR;
   const char *BE, *BV, *BR;
   int rc = 0;

   strncpy(AVer, A, alen);
   strncpy(BVer, B, blen);
   AVer[alen] = '\0';
   BVer[blen] = '\0';
   ParseVersion(AVer, &AE, &AV, &AR);
   ParseVersion(BVer, &BE, &BV, &BR);
   if (AE && !BE)
       rc = 1;
   else if (!AE && BE)
       rc = -1;
   else if (AE && BE)
   {
      int AEi, BEi;
      AEi = atoi(AE);
      BEi = atoi(BE);
      if (AEi < BEi)
	  rc = -1;
      else if (AEi > BEi)
	  rc = 1;
   }
   if (rc == 0)
   {
      rc = rpmvercmp(AV, BV);
      if (rc == 0) {
	  if (AR && !BR)
	      rc = 1;
	  else if (!AR && BR)
	      rc = -1;
	  else if (AR && BR)
	      rc = rpmvercmp(AR, BR);
      }
   }
   return rc;
}
									/*}}}*/
// rpmVS::DoCmpVersionArch - Compare versions, using architecture	/*{{{*/
// ---------------------------------------------------------------------
/* */
int rpmVersioningSystem::DoCmpVersionArch(const char *A,const char *Aend,
					  const char *AA,const char *AAend,
					  const char *B,const char *Bend,
					  const char *BA,const char *BAend)
{
   int rc = DoCmpVersion(A, Aend, B, Bend);
   if (rc == 0)
   {
      int aa = rpmMachineScore(RPM_MACHTABLE_INSTARCH, AA); 
      int ba = rpmMachineScore(RPM_MACHTABLE_INSTARCH, BA); 
      if (aa < ba)
	 rc = 1;
      else if (aa > ba)
	 rc = -1;
   }
   return rc;
}
									/*}}}*/
// rpmVS::CheckDep - Check a single dependency				/*{{{*/
// ---------------------------------------------------------------------
/* This simply preforms the version comparison and switch based on 
   operator. If DepVer is 0 then we are comparing against a provides
   with no version. */
bool rpmVersioningSystem::CheckDep(const char *PkgVer,
				   int Op,const char *DepVer)
{
   int PkgFlags = RPMSENSE_EQUAL;
   int DepFlags = 0;
   bool invert = false;
   int rc;
   
   switch (Op & 0x0F)
   {
    case pkgCache::Dep::LessEq:
      DepFlags = RPMSENSE_LESS|RPMSENSE_EQUAL;
      break;

    case pkgCache::Dep::GreaterEq:
      DepFlags = RPMSENSE_GREATER|RPMSENSE_EQUAL;
      break;
      
    case pkgCache::Dep::Less:
      DepFlags = RPMSENSE_LESS;
      break;
      
    case pkgCache::Dep::Greater:
      DepFlags = RPMSENSE_GREATER;
      break;

    case pkgCache::Dep::Equals:
      DepFlags = RPMSENSE_EQUAL;
      break;
      
    case pkgCache::Dep::NotEquals:
      DepFlags = RPMSENSE_EQUAL;
      invert = true;
      break;
      
    default:
      DepFlags = RPMSENSE_ANY;
      break;
   }

   rpmds pds = rpmdsSingle(RPMTAG_PROVIDENAME, "", PkgVer, (raptDepFlags) PkgFlags);
   rpmds dds = rpmdsSingle(RPMTAG_REQUIRENAME, "", DepVer, (raptDepFlags) DepFlags);
   rpmdsSetNoPromote(pds, _rpmds_nopromote);
   rpmdsSetNoPromote(dds, _rpmds_nopromote);
   rc = rpmdsCompare(pds, dds);
   rpmdsFree(pds);
   rpmdsFree(dds);
    
   return (!invert && rc) || (invert && !rc);
}
									/*}}}*/
// rpmVS::CheckDep - Check a single dependency				/*{{{*/
// ---------------------------------------------------------------------
/* This prototype is a wrapper over CheckDep above. It's useful in the
   cases where the kind of dependency matters to decide if it matches
   or not */
bool rpmVersioningSystem::CheckDep(const char *PkgVer,
				   pkgCache::DepIterator Dep)
{
   if (Dep->Type == pkgCache::Dep::Obsoletes &&
       (PkgVer == 0 || PkgVer[0] == 0))
      return false;
   return CheckDep(PkgVer,Dep->CompareOp,Dep.TargetVer());
}
									/*}}}*/
// rpmVS::UpstreamVersion - Return the upstream version string		/*{{{*/
// ---------------------------------------------------------------------
/* This strips all the vendor specific information from the version number */
string rpmVersioningSystem::UpstreamVersion(const char *Ver)
{
   // Strip off the bit before the first colon
   const char *I = Ver;
   for (; *I != 0 && *I != ':'; I++);
   if (*I == ':')
      Ver = I + 1;
   
   // Chop off the trailing -
   I = Ver;
   size_t Last = strlen(Ver);
   for (; *I != 0; I++)
      if (*I == '-')
	 Last = I - Ver;
   
   return string(Ver,Last);
}
									/*}}}*/

#endif /* HAVE_RPM */

// vim:sts=3:sw=3
