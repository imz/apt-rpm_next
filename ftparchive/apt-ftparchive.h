// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: apt-ftparchive.h,v 1.2 2002/07/25 18:07:19 niemeyer Exp $
/* ######################################################################

   Writer 
   
   The file writer classes. These write various types of output, sources,
   packages and contents.
   
   ##################################################################### */
									/*}}}*/
#ifndef APT_FTPARCHIVE_H
#define APT_FTPARCHIVE_H

#ifdef __GNUG__
#pragma interface "apt-ftparchive.h"
#endif

#include <fstream>

using std::ostream;
using std::ofstream;

extern ostream c0out;
extern ostream c1out;
extern ostream c2out;
extern ofstream devnull;
extern unsigned Quiet;

#endif
