// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: init.h,v 1.2 2002/07/25 18:07:18 niemeyer Exp $
/* ######################################################################

   Init - Initialize the package library

   This function must be called to configure the config class before
   calling many APT library functions.
   
   ##################################################################### */
									/*}}}*/
#ifndef PKGLIB_INIT_H
#define PKGLIB_INIT_H

#include <apt-pkg/configuration.h>
#include <apt-pkg/pkgsystem.h>

// See the makefile
#define APT_PKG_MAJOR 3
#define APT_PKG_MINOR 3
#define APT_PKG_RELEASE 0
    
extern const char *pkgVersion;
extern const char *pkgLibVersion;
extern const char *pkgOS;
extern const char *pkgCPU;

bool pkgInitConfig(Configuration &Cnf);
bool pkgInitSystem(Configuration &Cnf,pkgSystem *&Sys);

#ifdef APT_COMPATIBILITY
#if APT_COMPATIBILITY != 986
#warning "Using APT_COMPATIBILITY"
#endif

inline bool pkgInitialize(Configuration &Cnf) 
{
   return pkgInitConfig(Cnf) && pkgInitSystem(Cnf,_system);
};
#endif

#endif
