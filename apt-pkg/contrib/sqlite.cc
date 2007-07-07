#ifdef __GNUG__
#pragma implementation "apt-pkg/sqlite.h"
#endif

#include <config.h>

#ifdef WITH_SQLITE3

#include <apt-pkg/sqlite.h>
#include <apt-pkg/error.h>

// debug stuff..
#include <iostream>
using namespace std;

SqliteDB::SqliteDB(string DBPath): DBPath(DBPath), DB(NULL)
{
   int rc;
   //cout << __PRETTY_FUNCTION__ << " " << DBPath << endl;
   rc = sqlite3_open(DBPath.c_str(), &DB);
   if (rc != SQLITE_OK) {
      _error->Error("opening %s db failed", DBPath.c_str());
   }
}

SqliteDB::~SqliteDB()
{
   //cout << __PRETTY_FUNCTION__ << endl;
   if (DB) {
      sqlite3_close(DB);
   }
}

SqliteQuery *SqliteDB::Query()
{
   return new SqliteQuery(DB);
}

bool SqliteQuery::Exec(string SQL)
{
   int rc;
   rc = sqlite3_get_table(DB, SQL.c_str(), &res, &nrow, &ncol, NULL);
   if (rc != SQLITE_OK) {
      sqlite3_free_table(res);
      nrow = 0;
      ncol = 0;
      res = NULL;
   }
   ColNames.clear();
   for (int col = 0; col < ncol; col++) {
      ColNames[res[col]] = col;
   }

   return (rc == SQLITE_OK);
}

bool SqliteQuery::Step()
{
   if (cur >= nrow) {
      return false;
   }
   cur ++;
   return true;
}

bool SqliteQuery::Rewind()
{
   cur = 0;
   return true;
}

bool SqliteQuery::Jump(unsigned long Pos)
{
   if (Pos >= nrow) {
      return false;
   }
   cur = Pos;
   return true;
}

string SqliteQuery::GetCol(string ColName)
{
   string val = "";
   char *item = res[cur*ncol+ColNames[ColName]];
   if (item != NULL)
      val = item;
   return val;
}

unsigned long SqliteQuery::GetColI(string ColName)
{
   unsigned long val = 0;
   char *item = res[cur*ncol+ColNames[ColName]];
   if (item != NULL)
      val = atol(item);
   return val;
}
SqliteQuery::SqliteQuery(sqlite3 *DB) :
   DB(DB), res(NULL), nrow(0), ncol(0), cur(0)
{
   //cout << __PRETTY_FUNCTION__ << endl;
}

SqliteQuery::~SqliteQuery()
{
   //cout << __PRETTY_FUNCTION__ << endl;
   if (res) {
      sqlite3_free_table(res);
   }
}

#endif /* WITH_SQLITE3 */


// vim:sts=3:sw=3
