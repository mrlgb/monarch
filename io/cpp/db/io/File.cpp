/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#include "db/io/File.h"

#include "db/io/FileFunctions.h"
#include "db/io/FileList.h"
#include "db/io/IOException.h"
#include "db/rt/DynamicObject.h"
#include "db/util/StringTokenizer.h"

#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <vector>

using namespace std;
using namespace db::io;
using namespace db::rt;
using namespace db::util;

// FIXME: paths on windows are case-insensitive -- this means we should make
// a macro/function to replace strcmp in most of these functions so it uses
// strcasecmp when WIN32 is defined

#ifdef WIN32
   const char* File::NAME_SEPARATOR = "\\";
   const char File::NAME_SEPARATOR_CHAR = '\\';
   const char* File::PATH_SEPARATOR = ";";
   const char File::PATH_SEPARATOR_CHAR = ';';
   
   // a helper function for stripping drive letters from windows paths
   static string stripDriveLetter(const char* path, string* drive = NULL)
   {
      string rval;
      
      if(drive != NULL)
      {
         drive->erase();
      }
      
      int len = strlen(path);
      if(len > 1)
      {
         if(path[1] == ':')
         {
            if(len > 2)
            {
               rval = (path + 2);
               drive.push_back(path[0]);
               drive.push_back(path[1]);
            }
            else
            {
               rval = File::NAME_SEPARATOR;
            }
         }
         else
         {
            rval = path;
         }
      }
      else
      {
         rval = path;
      }
      
      return rval;
   }
#else
   const char* File::NAME_SEPARATOR = "/";
   const char File::NAME_SEPARATOR_CHAR = '/';
   const char* File::PATH_SEPARATOR = ":";
   const char File::PATH_SEPARATOR_CHAR = ':';
#endif

FileImpl::FileImpl()
{
   mPath = strdup(".");
   
   // initialize absolute path
   string abs;
   File::getAbsolutePath(mPath, abs);
   mAbsolutePath = strdup(abs.c_str());
   
   mBaseName = mCanonicalPath = mExtension = NULL;
}

FileImpl::FileImpl(const char* path)
{
   mPath = strdup(path);
   
   // initialize absolute path
   string abs;
   File::getAbsolutePath(mPath, abs);
   mAbsolutePath = strdup(abs.c_str());
   
   mBaseName = mCanonicalPath = mExtension = NULL;
}

FileImpl::~FileImpl()
{
   free(mPath);
   free(mAbsolutePath);
   
   if(mBaseName != NULL)
   {
      free(mBaseName);
   }
   
   if(mCanonicalPath != NULL)
   {
      free(mCanonicalPath);
   }
   
   if(mExtension != NULL)
   {
      free(mExtension);
   }
}

bool FileImpl::create()
{
   bool rval = false;
   
   FILE* fp = fopen(mAbsolutePath, "w");
   if(fp != NULL)
   {
      rval = true;
      fclose(fp);
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not create file",
         "db.io.File.CreateFailed");
      e->getDetails()["error"] = strerror(errno);
      Exception::setLast(e, false);
   }
   
   return rval;
}

bool FileImpl::mkdirs()
{
   bool rval = true;
   
   // get path
   string path = (isDirectory() ?
      mAbsolutePath : File::parentname(mAbsolutePath));
   
   // create stack of directories in the path
   vector<string> dirStack;
   while(!File::isPathRoot(path.c_str()))
   {
      dirStack.push_back(path);
      path = File::parentname(path.c_str());
   }
   
   // iteratively create directories
   struct stat s;
   int rc;
   while(rval && !dirStack.empty())
   {
      path = dirStack.back();
      dirStack.pop_back();
      
      // try to stat directory
      rc = stat(path.c_str(), &s);
      if(rc != 0)
      {
         // directory doesn't exist, so try to create it
         // Note: windows ignores permissions in mkdir(), always 0777
         if(mkdir(path.c_str(), 0777) < 0)
         {
            ExceptionRef e = new Exception(
               "Could not create directory.",
               "db.io.File.CreateDirectoryFailed");
            e->getDetails()["path"] = path.c_str();
            e->getDetails()["error"] = strerror(errno);
            Exception::setLast(e, false);
            rval = false;
         }
      }
   }
   
   return rval;
}

bool FileImpl::exists()
{
   bool rval = false;
   
   struct stat s;
   int rc = stat(mAbsolutePath, &s);
   if(rc == 0)
   {
      rval = true;
   }
   else
   {
      // does not set an exception intentionally, the file just doesn't exist
   }
   
   return rval;
}

bool FileImpl::remove()
{
   bool rval = false;
   
   int rc = ::remove(mAbsolutePath);
   if(rc == 0)
   {
      rval = true;
   }
   else if(exists())
   {
      // only set exception when the file exists and could not be removed
      ExceptionRef e = new Exception(
         "Could not delete file.",
         "db.io.File.DeleteFailed");
      e->getDetails()["error"] = strerror(errno);
      Exception::setLast(e, false);
   }
   
   return rval;
}

bool FileImpl::rename(File& file)
{
   bool rval = false;
   
   // delete old file
   file->remove();
   
   // rename file
   int rc = ::rename(mAbsolutePath, file->getAbsolutePath());
   if(rc == 0)
   {
      rval = true;
   }
   else
   {
      ExceptionRef e = new Exception(
         "Could not rename file.",
         "db.io.File.RenameFailed");
      e->getDetails()["error"] = strerror(errno);
      Exception::setLast(e, false);
   }
   
   return rval;
}

const char* FileImpl::getBaseName()
{
   if(mBaseName == NULL)
   {
      // initialize base name
      mBaseName = strdup(File::basename(mAbsolutePath).c_str());
   }
   
   return mBaseName;
}

const char* FileImpl::getPath() const
{
   return mPath;
}

const char* FileImpl::getAbsolutePath() const
{
   return mAbsolutePath;
}

const char* FileImpl::getCanonicalPath()
{
   if(mCanonicalPath == NULL)
   {
      string path; 
      File::getCanonicalPath(mAbsolutePath, path);
      mCanonicalPath = strdup(path.c_str());
   }
   
   return mCanonicalPath;
}

const char* FileImpl::getExtension()
{
   if(mExtension == NULL)
   {
      string root;
      string ext;
      File::splitext(mAbsolutePath, root, ext);
      mExtension = strdup(ext.c_str());
   }
   
   return mExtension;
}

off_t FileImpl::getLength()
{
   struct stat s;
   int rc = stat(mAbsolutePath, &s);
   if(rc != 0)
   {
      ExceptionRef e = new Exception(
         "Could not stat file.",
         "db.io.File.StatFailed");
      e->getDetails()["error"] = strerror(errno);
      Exception::setLast(e, false);
   }
   
   return s.st_size;
}

FileImpl::Type FileImpl::getType(bool follow)
{
   Type rval = Unknown;
   
   struct stat s;
   int rc;
   
   if(follow)
   {
      // ensure follow links are followed with stat
      rc = stat(mAbsolutePath, &s);
   }
   else
   {
      // use lstat so symbolic links aren't followed
      rc = lstat(mAbsolutePath, &s);
   }
   
   if(rc == 0)
   {
      switch(s.st_mode & S_IFMT)
      {
         case S_IFREG:
            rval = RegularFile;
            break;
         case S_IFDIR:
            rval = Directory;
            break;
         case S_IFLNK:
            rval = SymbolicLink;
            break;
         default:
            break;
      }
   }
   
   return rval;
}

bool FileImpl::isCurrentDirectory()
{
   string dirname;
   string basename;
   File::split(mAbsolutePath, dirname, basename);
   
   return (strcmp(basename.c_str(), ".") == 0);
}

bool FileImpl::isFile()
{
   return getType() == RegularFile;
}

bool FileImpl::contains(const char* path)
{
   bool rval = false;
   
   string containee;
   if(File::getAbsolutePath(path, containee))
   {
      rval = (containee.find(mAbsolutePath, 0) == 0);
   }
   
   return rval;
}

bool FileImpl::contains(File& path)
{
   return contains(path->getAbsolutePath());
}

bool FileImpl::isDirectory()
{
   return getType() == Directory;
}

bool FileImpl::isParentDirectory()
{
   string dirname;
   string basename;
   File::split(mAbsolutePath, dirname, basename);
   
   return (strcmp(basename.c_str(), "..") == 0);
}

bool FileImpl::isRoot()
{
   // just check to see if the absolute path is the same as the parent
   string parent = File::parentname(mAbsolutePath);
   return (strcmp(parent.c_str(), mAbsolutePath) == 0);
}

bool FileImpl::isReadable()
{
   return File::isPathReadable(mAbsolutePath);
}

bool FileImpl::isSymbolicLink()
{
   return getType(false) == SymbolicLink;
}

bool FileImpl::isWritable()
{
   return File::isPathWritable(mAbsolutePath);
}

void FileImpl::listFiles(FileList& files)
{
   if(isDirectory())
   {
      // open directory
      DIR* dir = opendir(mAbsolutePath);
      if(dir == NULL)
      {
         // FIXME: add error handling
      }
      else
      {
         // read each directory entry
         struct dirent* entry;
         unsigned int len1 = strlen(mAbsolutePath);
         bool separator = mAbsolutePath[len1 - 1] != File::NAME_SEPARATOR_CHAR;
         while((entry = readdir(dir)) != NULL)
         {
            // d_name is null-terminated name for file, without path name
            // so copy file name before d_name to get full path
            unsigned int len2 = strlen(entry->d_name);
            char path[len1 + len2 + 2];
            memcpy(path, mAbsolutePath, len1);
            if(separator)
            {
               // add path separator as appropriate
               path[len1] = File::NAME_SEPARATOR_CHAR;
               memcpy(path + len1 + 1, entry->d_name, len2 + 1);
            }
            else
            {
               memcpy(path + len1, entry->d_name, len2 + 1);
            }
            
            // add new file to list
            File file(path);
            files->add(file);
         }
         
         // close directory
         closedir(dir);
      }
   }
}

Date FileImpl::getModifiedDate()
{
   Date date(0);
   
   struct stat s;
   if(stat(mAbsolutePath, &s) == 0)
   {
      date.setSeconds(s.st_mtime);
   }
   
   return date;
}

bool File::operator==(const File& rhs) const
{
   bool rval = false;
   
   File& file = *((File*)&rhs);
   
   // compare absolute paths and types for equality
   if(strcmp((*this)->getAbsolutePath(), file->getAbsolutePath()) == 0)
   {
      rval = ((*this)->getType() == file->getType());
   }
   
   return rval;
}

bool File::getAbsolutePath(const char* path, string& absolutePath)
{
   bool rval = true;
   
   // if the path isn't absolute, prepend the current working directory
   // to the path
   string tmp;
   if(!isPathAbsolute(path))
   {
      string cwd;
      if((rval = getCurrentWorkingDirectory(cwd)))
      {
         tmp = File::join(cwd.c_str(), path);
      }
   }
   else
   {
      // path already absolute
      tmp.assign(path);
   }
   
   // normalize path
   rval = rval && normalizePath(tmp.c_str(), absolutePath);
   
   return rval;
}

bool File::getCanonicalPath(const char* path, string& canonicalPath)
{
   bool rval = true;
   
   // FIXME: add while loop to keep following symbolic links after
   // getting absolute path
   // FIXME: call readlink(path, outputbuffer, outputbuffersize), returns
   // number of characters in buffer or -1 with errno set
   
   // get absolute path
   rval = getAbsolutePath(path, canonicalPath);
   
   return rval;
}

bool File::normalizePath(const char* path, string& normalizedPath)
{
   bool rval = true;
   
#ifdef WIN32
   // strip drive letter, point "path" at stripped path
   string drive;
   string stripped = stripDriveLetter(path, &drive);
   path = stripped.c_str();
#endif
   
   string tempPath;
   if(strlen(path) > 0)
   {
      // store whether or not path begins with path name separator
      bool separator = (path[0] == NAME_SEPARATOR_CHAR);
      
      // clean up the relative directory references, by traversing the
      // path in reverse
      StringTokenizer st(path, NAME_SEPARATOR_CHAR, false);
      int skip = 0;
      while(st.hasPreviousToken())
      {
         const char* token = st.previousToken();
         if(strcmp(token, "..") == 0)
         {
            // since we're traversing the path backwards, skip the upcoming
            // previous directory because we found a ".."
            skip++;
         }
         else if(strcmp(token, ".") != 0)
         {
            if(skip == 0)
            {
               // not skipping directory, so join to the normalized path
               tempPath = File::join(token, tempPath.c_str()); 
            }
            else
            {
               // directory skipped
               skip--;
            }
         }
      }
      
      // re-insert path name separator
      if(separator)
      {
         tempPath.insert(0, 1, NAME_SEPARATOR_CHAR);
      }
      
      if(skip > 0 && !isPathAbsolute(path))
      {
         ExceptionRef e = new Exception(
            "Could not normalize relative path. Too many \"..\" directories.",
            "db.io.File.BadNormalization");
         e->getDetails()["path"] = path;
         Exception::setLast(e, false);
         rval = false;
      }
   }
   
#ifdef WIN32
   // re-add drive letter before assigning to temp path
   normalizedPath.assign(drive);
   normalizedPath.append(tempPath);
#else
   // assign to temp path
   normalizedPath.assign(tempPath);
#endif
   
   return rval;
}

bool File::expandUser(const char* path, string& expandedPath)
{
   bool rval = true;
   
   // FIXME: handle windows issues
   
   size_t pathlen = 0;
   if(path != NULL)
   {
      pathlen = strlen(path);
   }
   
   if(pathlen > 0 && path[0] == '~')
   {
      // FIXME add getpwnam support
      // only handle current user right now
      if(pathlen > 1 && path[1] != '/')
      {
         ExceptionRef e = new Exception(
            "Only current user supported (ie, \"~/...\").",
            "db.io.File.NotImplemented");
         Exception::setLast(e, false);
         rval = false;
      }
      else
      {
         const char* home = getenv("HOME");
         if(home != NULL)
         {
            // use temp string to avoid problems if path is same expandedPath
            // common for code like expandUser(path.c_str(), path)
            // add HOME
            string newPath(home);
            // add rest of path
            newPath.append(path + 1);
            // copy to output
            expandedPath.assign(newPath);
         }
         else
         {
            // no HOME set
            ExceptionRef e = new Exception(
               "No home path set.",
               "db.io.File.HomeNotSet");
            Exception::setLast(e, false);
            rval = false;
         }
      }
   }
   else
   {
      expandedPath.assign(path);
   }
   
   return rval;
}

bool File::getCurrentWorkingDirectory(string& cwd)
{
   bool rval = true;
   bool found = false;
   size_t path_max;
#ifdef PATH_MAX
   path_max = PATH_MAX;
#else
   path_max = 1024;
#endif
   
   char* b = (char*)malloc(path_max);
   while(rval && !found)
   {
      if(getcwd(b, path_max) != NULL)
      {
         cwd.assign(b);
         found = true;
      }
      else
      {
         // not enough space for path
         // fail to check again if bigger than arbitrary size of 8k
         if(errno == ERANGE && path_max < (1024 * 8))
         {
            path_max *= 2;
            b = (char*)realloc(b, path_max);
         }
         else
         {
            // path was too large for getcwd
            ExceptionRef e = new Exception(
               "Could not get current working directory, path too long!",
               "db.io.File.PathTooLong");
            Exception::setLast(e, false);
            rval = false;
         }
      }
   }
   free(b);
   
   return rval;
}

bool File::isPathReadable(const char* path)
{
   return (access(path, R_OK) == 0);
}

bool File::isPathWritable(const char* path)
{
   return (access(path, W_OK) == 0);
}

void File::split(const char* path, string& dirname, string& basename)
{
   // FIXME: support non-posix paths
   string sPath = path;
   string::size_type pos = sPath.rfind(NAME_SEPARATOR_CHAR) + 1;
   dirname.assign(sPath.substr(0, pos));
   basename.assign(sPath.substr(pos));
   if(dirname.length() > 0 && dirname != "/")
   {
      dirname.erase(dirname.find_last_not_of("/") + 1);
   }
}

void File::splitext(
   const char* path, string& root, string& ext, const char* sep)
{
   string sPath = path;
   string::size_type pos = sPath.rfind(sep);
   if(pos != string::npos)
   {
      root.assign(sPath.substr(0, pos));
      ext.assign(sPath.substr(pos));
   }
   else
   {
      root.assign(path);
      ext.clear();
   }
}

string File::parentname(const char* path)
{
   // FIXME: figure out drive letter stuff for windows
   
   string dirname = File::dirname(path);
   if(strcmp(dirname.c_str(), path) == 0)
   {
      // drop last slash if dirname != "/"
      if(dirname.length() > 1)
      {
         dirname.erase(dirname.end());
         dirname = File::dirname(dirname.c_str());
      }
   }
   
   return dirname;
}

string File::dirname(const char* path)
{
   string dirname;
   string basename;
   
   File::split(path, dirname, basename);
   
   return dirname;
}

string File::basename(const char* path)
{
   string dirname;
   string basename;
   
   File::split(path, dirname, basename);
   
   return basename;
}

bool File::isPathAbsolute(const char* path)
{
   // FIXME: windows path is absolute if drive letter starts path or
   // '\\' starts path
   
   // FIXME: linux path is absolute if starts with '/'
   
   // FIXME: support non-posix paths.
   return path != NULL && strlen(path) > 0 && path[0] == '/';
}

bool File::isPathRoot(const char* path)
{
   bool rval = false;
   
   // just check to see if the absolute path is the same as the parent
   string abs;
   if(File::getAbsolutePath(path, abs))
   {
      string parent = File::parentname(abs.c_str());
      rval = (strcmp(parent.c_str(), abs.c_str()) == 0);
   }
   
   return rval;
}

string File::join(const char* path1, const char* path2)
{
   // start with path1
   string path = path1;
   
   // skip if path2 empty
   if(strlen(path2) > 0)
   {
      string::size_type plen = path.length();
      if(plen == 0)
      {
         // empty path1 so just assign path2
         path.assign(path2);
      }
      else
      {
         bool path1HasSep = (path[plen - 1] == NAME_SEPARATOR_CHAR);
         bool path2HasSep = (path2[0] == NAME_SEPARATOR_CHAR);
         if(!path1HasSep && !path2HasSep)
         {
            // no trailing path1 separator or leading path2 separator
            path.push_back(NAME_SEPARATOR_CHAR);
            path.append(path2);
         }
         else if(path1HasSep && path2HasSep)
         {
            // trailing and leading slash, skip one
            path.append(path2 + 1);
         }
         else
         {
            // only one of trailing or leading, just append
            path.append(path2);
         }
      }
   }
   
   return path;
}
