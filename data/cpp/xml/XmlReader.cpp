/*
 * Copyright (c) 2007 Digital Bazaar, Inc.  All rights reserved.
 */
#include "xml/XmlReader.h"
#include "IOException.h"

using namespace std;
using namespace db::data;
using namespace db::data::xml;
using namespace db::io;
using namespace db::rt;

// initialize encoding
const char* XmlReader::CHAR_ENCODING = "UTF-8";

// initialize read size
unsigned int XmlReader::READ_SIZE = 4096;

XmlReader::XmlReader()
{
   // create parser
   mParser = XML_ParserCreateNS(CHAR_ENCODING, '|');
   
   // set user data to this reader
   XML_SetUserData(mParser, this);
   
   // set handlers
   XML_SetElementHandler(mParser, &startElement, &endElement);
   XML_SetCharacterDataHandler(mParser, &appendData);
}

XmlReader::~XmlReader()
{
   // free parser
   XML_ParserFree(mParser);
}

void XmlReader::startElement(const XML_Char* name, const XML_Char** attrs)
{
   if(mDataBindingsStack.front() != NULL)
   {
      // parse element namespace
      char* ns;
      parseNamespace(&name, &ns);
      
      // start data with given encoding, namespace and name and push
      // data binding onto stack
      DataBinding* db = mDataBindingsStack.front()->startData(
         CHAR_ENCODING, ns, (const char*)name);
      mDataBindingsStack.push_front(db);
      if(db != NULL)
      {
         char* attrns;
         for(int i = 0; attrs[i] != NULL; i += 2)
         {
            // parse attribute namespace
            parseNamespace(&attrs[i], &attrns);
            
            // set attribute data
            db->setData(
               CHAR_ENCODING, attrns, attrs[i],
               attrs[i + 1], strlen(attrs[i + 1]));
            
            // clean up attribute namespace
            if(attrns != NULL)
            {
               delete [] attrns;
            }
         }
      }
      
      // clean up element namespace
      if(ns != NULL)
      {
         delete [] ns;
      }
   }
   else
   {
      // no available data binding
      mDataBindingsStack.push_front(NULL);
   }
}

void XmlReader::endElement(const XML_Char* name)
{
   // get front data binding
   DataBinding* db = mDataBindingsStack.front();
   
   // pop front data binding
   mDataBindingsStack.pop_front();
   
   if(db != NULL && mDataBindingsStack.front() != NULL)
   {
      // parse element namespace
      char* ns;
      parseNamespace(&name, &ns);
      
      // end data
      mDataBindingsStack.front()->endData(CHAR_ENCODING, ns, name, db);
      
      // clean up element namespace
      if(ns != NULL)
      {
         delete [] ns;
      }
   }
}

void XmlReader::appendData(const XML_Char* data, int length)
{
   // get front data binding
   DataBinding* db = mDataBindingsStack.front();
   if(db != NULL)
   {
      // append data
      db->appendData(CHAR_ENCODING, data, length);
   }
}

void XmlReader::parseNamespace(const char** fullName, char** ns)
{
   // parse namespace, if one exists
   *ns = NULL;
   const char* sep = strchr(*fullName, '|');
   if(sep != NULL)
   {
      *ns = new char[sep - *fullName];
      strncpy(*ns, *fullName, sep - *fullName);
      memset(ns + (sep - *fullName), 0, 1);
      *fullName = sep + 1;
   }
}

void XmlReader::startElement(
   void* xr, const XML_Char* name, const XML_Char** attrs)
{
   // get reader, start element
   XmlReader* reader = (XmlReader*)xr;
   reader->startElement(name, attrs);
}

void XmlReader::endElement(void* xr, const XML_Char* name)
{
   // get reader, end element
   XmlReader* reader = (XmlReader*)xr;
   reader->endElement(name);
}

void XmlReader::appendData(void* xr, const XML_Char* data, int length)
{
   // get reader, append data
   XmlReader* reader = (XmlReader*)xr;
   reader->appendData(data, length);
}

bool XmlReader::read(DataBinding* db, InputStream* is)
{
   bool rval = false;
   
   // clear data bindings stack
   mDataBindingsStack.clear();
   
   // push root data binding onto stack
   mDataBindingsStack.push_front(db);
   
   char* b = (char*)XML_GetBuffer(mParser, READ_SIZE);
   if(b != NULL)
   {
      bool error = false;
      int numBytes;
      while(!error && (numBytes = is->read(b, READ_SIZE)) != -1)
      {
         error = (XML_ParseBuffer(mParser, numBytes, false) == 0);
      }
      
      if(!error)
      {
         // parse last data
         XML_ParseBuffer(mParser, 0, true);
      }
      
      if(error)
      {
         int line = XML_GetCurrentLineNumber(mParser);
         const char* errorString = XML_ErrorString(XML_GetErrorCode(mParser));
         char msg[100 + strlen(errorString)];
         sprintf(msg, "Xml parser error at line %d:\n%s\n", line, errorString);
         Exception::setLast(new IOException(msg));
      }
   }
   else
   {
      // set memory exception
      Exception::setLast(new IOException("Insufficient memory to parse xml!"));
   }
   
   return rval;
}
