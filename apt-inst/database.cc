// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: database.cc,v 1.2 2002/07/25 18:07:17 niemeyer Exp $
/* ######################################################################

   Data Base Abstraction
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/database.h"
#endif

#include <apt-pkg/database.h>
									/*}}}*/

// DataBase::GetMetaTmp - Get the temp dir				/*{{{*/
// ---------------------------------------------------------------------
/* This re-initializes the meta temporary directory if it hasn't yet 
   been inited for this cycle. The flag is the emptyness of MetaDir */
bool pkgDataBase::GetMetaTmp(string &Dir)
{
   if (MetaDir.empty() == true)
      if (InitMetaTmp(MetaDir) == false)
	 return false;
   Dir = MetaDir;
   return true;
}
									/*}}}*/
