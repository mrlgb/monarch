/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_rt_DynamicLibrary_H
#define monarch_rt_DynamicLibrary_H

#ifndef WIN32
#include <dlfcn.h>

#else
#include <windows.h>

// these flags are not used in windows
#define RTLD_LAZY     0x00001
#define RTLD_NOW      0x00002
#define RTLD_GLOBAL   0x00100

/**
 * Opens a dynamic library.
 *
 * @param filename the filename for the dll to open.
 * @param flag not used in Windows (in Linux, can specify lazy resolution, etc).
 *
 * @return a handle to the loaded library or NULL if the loading failed.
 */
inline static void* dlopen(const char* filename, int flag)
{
   return LoadLibrary(filename);
}

static char gDynamicLibraryError[100];

/**
 * Gets a dynamically allocated null-terminated string with the last
 * error that occurred and clears the last error. Will return NULL if there
 * is no last error.
 *
 * The caller of this method must free the returned string.
 *
 * @return the last error that occurred or NULL.
 */
inline static char* dlerror()
{
   char* rval = NULL;

   // get the last error
   DWORD dwLastError = GetLastError();
   if(dwLastError != 0)
   {
      // get the last error in an allocated string buffer
      //
      // Note: A LPTSTR is a pointer to an UTF-8 string (TSTR). If any characters
      // in the error message are not ASCII, then they will get munged unless
      // the bytes in the returned string are converted back to wide characters
      // for display/other use
      LPVOID lpBuffer;
      unsigned int size = FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
         FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwLastError, 0,
         (LPTSTR)&lpBuffer, 0, NULL);

      if(size > 0)
      {
         // copy into global error
         size = (size > 99) ? 99 : size;
         memset(gDynamicLibraryError, 0, 100);
         memcpy(gDynamicLibraryError, lpBuffer, size);
         rval = gDynamicLibraryError;
      }

      // free lpBuffer
      LocalFree(lpBuffer);

      // set last error to 0
      SetLastError(0);
   }

   return rval;
}

/**
 * Gets a pointer to the address for a symbol in a dynamic library.
 *
 * @param handle the handle to the dynamic library.
 * @param symbol the null-terminated string with the name of the symbol.
 *
 * @return a pointer to the address for the symbol.
 */
inline static void* dlsym(void* handle, const char* symbol)
{
   return (void*)GetProcAddress((HINSTANCE)handle, symbol);
}

/**
 * Closes a dynamic library. This method will actually decrement the reference
 * count on given handle. If that count reaches 0, the library will be
 * unloaded.
 *
 * @param handle the handle to the dynamic library to close.
 *
 * @return 0 on success, non-zero on error.
 */
inline static int dlclose(void* handle)
{
   // FreeLibrary's return value is the opposite of standard dlclose()
   return (FreeLibrary((HINSTANCE)handle) == 0) ? 1 : 0;
}

#endif // end windows definitions

#endif
