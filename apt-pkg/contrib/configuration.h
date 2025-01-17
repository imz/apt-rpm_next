// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: configuration.h,v 1.2 2003/01/29 18:43:48 niemeyer Exp $
/* ######################################################################

   Configuration Class

   This class provides a configuration file and command line parser
   for a tree-oriented configuration environment. All runtime configuration
   is stored in here.

   Each configuration name is given as a fully scoped string such as
     Foo::Bar
   And has associated with it a text string. The Configuration class only
   provides storage and lookup for this tree, other classes provide
   configuration file formats (and parsers/emitters if needed).

   Most things can get by quite happily with,
     cout << _config->Find("Foo::Bar") << endl;

   A special extension, support for ordered lists is provided by using the
   special syntax, "block::list::" the trailing :: designates the
   item as a list. To access the list you must use the tree function on
   "block::list".

   ##################################################################### */
									/*}}}*/
#ifndef PKGLIB_CONFIGURATION_H
#define PKGLIB_CONFIGURATION_H

#include <string>
#include <iostream>

using std::string;

class Configuration
{
   public:

   struct Item
   {
      string Value;
      string Tag;
      Item *Parent;
      Item *Child;
      Item *Next;

      string FullTag(const Item *Stop = 0) const;

      Item() : Parent(0), Child(0), Next(0) {};
   };

   private:

   Item *Root;
   bool ToFree;

   Item *Lookup(Item *Head,const char *S,unsigned long Len,bool Create);
   Item *Lookup(const char *Name,bool Create);
   inline const Item *Lookup(const char *Name) const
   {
      return ((Configuration *)this)->Lookup(Name,false);
   }

   // CNC:2003-02-23 - Helper for copy constructor.
   void CopyChildren(Item *From, Item *To);

   public:

   string Find(const char *Name,const char *Default = 0) const;
   string Find(string Name,const char *Default = 0) const {return Find(Name.c_str(),Default);}
   string FindFile(const char *Name,const char *Default = 0) const;
   string FindDir(const char *Name,const char *Default = 0) const;
   int FindI(const char *Name,int Default = 0) const;
   int FindI(string Name,int Default = 0) const {return FindI(Name.c_str(),Default);}
   bool FindB(const char *Name,bool Default = false) const;
   bool FindB(string Name,bool Default = false) const {return FindB(Name.c_str(),Default);}
   string FindAny(const char *Name,const char *Default = 0) const;

   inline void Set(string Name,string Value) {Set(Name.c_str(),Value);}
   void CndSet(const char *Name,string Value);
   void Set(const char *Name,string Value);
   void Set(const char *Name,int Value);

   inline bool Exists(string Name) const {return Exists(Name.c_str());}
   bool Exists(const char *Name) const;
   bool ExistsAny(const char *Name) const;

   void Clear(string Name);

   inline const Item *Tree(const char *Name) const {return Lookup(Name);}

   inline void Dump() { Dump(std::clog); }
   void Dump(std::ostream& str);

   // CNC:2003-02-23 - Copy constructor.
   Configuration(Configuration &Conf);

   Configuration(const Item *Root);
   Configuration();
   ~Configuration();
};

extern Configuration *_config;

bool ReadConfigFile(Configuration &Conf,string FName,bool AsSectional = false,
		    unsigned Depth = 0);

bool ReadConfigDir(Configuration &Conf,string Dir,bool AsSectional = false,
		    unsigned Depth = 0);

#ifdef SWIG
   struct Configuration::Item
   {
      string Value;
      string Tag;
      Item *Parent;
      Item *Child;
      Item *Next;

      string FullTag(const Item *Stop = 0) const;

      Item() : Parent(0), Child(0), Next(0) {}
   };
#endif

#endif
