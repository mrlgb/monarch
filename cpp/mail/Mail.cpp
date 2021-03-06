/*
 * Copyright (c) 2007-2011 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/mail/Mail.h"

#include "monarch/util/Base64Codec.h"
#include "monarch/util/StringTools.h"

#include <cstdio>

using namespace std;
using namespace monarch::mail;
using namespace monarch::rt;
using namespace monarch::util;

Mail::Mail() :
   mRecipients(Array)
{
   mMessage["headers"]["To"]->setType(Array);
   mMessage["headers"]["Subject"] = "";
   mMessage["body"]->setType(String);
}

Mail::~Mail()
{
}

void Mail::clear()
{
   mRecipients->setType(Array);
   mRecipients->clear();
   mMessage["headers"]["To"]->setType(Array);
   mMessage["headers"]["To"]->clear();
   mMessage["headers"]["Subject"] = "";
   mMessage["body"]->setType(String);
   mMessage["body"]->clear();
}

bool Mail::setAddress(Address& a, const char* address)
{
   bool rval = false;

   // FIXME: obviously this needs work to be robust

   // set address
   a["address"] = address;

   // parse these forms of addresses:
   //  user@example.com
   //  <user@example.com>
   //  User <user@example.com>
   //  "Example User" <user@example.com>

   // find mailbox
   size_t alen = strlen(address);
   string mailbox;

   if(alen > 0)
   {
      // assume simple mailbox with no display name
      const char* mbstart = address;
      const char* mbend = address + alen;

      // check if this appears to have a "<...>" enclosed mailbox part
      if(*(mbend - 1) == '>')
      {
         --mbend;
         // find starting '<'
         mbstart = strrchr(address, '<');
         // in corner case of missing start '<' just assume mailbox is
         // everything up to closing '>'
         if(mbstart != NULL)
         {
            // advance over '<'
            ++mbstart;
         }
      }
      mailbox.assign(mbstart, mbend - mbstart);
   }

   // check for domain
   const char* at = strchr(mailbox.c_str(), '@');
   if(at != NULL)
   {
      // set domain
      a["domain"] = (at + 1);

      // set smtp encoding of address
      a["smtpEncoding"]->format("<%s>", mailbox.c_str());

      rval = true;
   }

   return rval;
}

bool Mail::addRecipient(const char* header, const char* address)
{
   bool rval = false;

   Address a;
   rval = setAddress(a, address);
   if(rval)
   {
      // add to list of recipients
      mRecipients->append(a);

      // add header if not NULL
      if(header != NULL)
      {
         mMessage["headers"][header]->setType(Array);
         mMessage["headers"][header]->append(address);
      }
   }

   return rval;
}

bool Mail::setSender(const char* address)
{
   bool rval = false;

   rval = setAddress(mSender, address);
   if(rval)
   {
      // set "From" header
      mMessage["headers"]["From"] = address;
   }

   return rval;
}

Address& Mail::getSender()
{
   return mSender;
}

bool Mail::addTo(const char* address)
{
   return addRecipient("To", address);
}

bool Mail::addCc(const char* address)
{
   return addRecipient("CC", address);
}

bool Mail::addBcc(const char* address)
{
   return addRecipient(NULL, address);
}

AddressList& Mail::getRecipients()
{
   return mRecipients;
}

static void biCapitalize(char* name)
{
   // capitalize first letter and letters after hyphens
   // decapitalize other letters
   // NOTE: hardcoded version is faster than using toupper/tolower
   int length = 0;
   if(name != NULL && *name != '\0')
   {
      ++length;
      char* ptr = name;
      // cap first
      if(*ptr >= 'a' && *ptr <= 'z')
      {
         *ptr -= 'a' - 'A';
      }
      for(++ptr; *ptr != '\0'; ++ptr, ++length)
      {
         // cap after '-'
         if(*(ptr - 1) == '-')
         {
            if(*ptr >= 'a' && *ptr <= 'z')
            {
               *ptr -= 'a' - 'A';
            }
         }
         // decap rest
         else if(*ptr >= 'A' && *ptr <= 'Z')
         {
            *ptr += 'a' - 'A';
         }
      }

      // special case TE header
      if(length == 2 && name[0] == 'T' && name[1] == 'e')
      {
         name[1] = 'E';
      }
   }
}

void Mail::setHeader(const char* header, const char* value)
{
   if(strcasecmp(header, "from") == 0)
   {
      setSender(value);
   }
   else if(strcasecmp(header, "to") == 0)
   {
      addTo(value);
   }
   else if(strcasecmp(header, "cc") == 0)
   {
      addCc(value);
   }
   else if(strcasecmp(header, "bcc") == 0)
   {
      addBcc(value);
   }
   else if(strcasecmp(header, "subject") == 0)
   {
      setSubject(value);
   }
   else
   {
      char tmp[strlen(header) + 1];
      strcpy(tmp, header);
      biCapitalize(tmp);
      mMessage["headers"][tmp] = value;
   }
}

void Mail::setSubject(const char* subject)
{
   mMessage["headers"]["Subject"] = subject;
}

void Mail::setBody(const char* body)
{
   mMessage["body"] = body;
}

void Mail::appendBodyLine(const char* line)
{
   string str = mMessage["body"]->getString();
   str.append(line);
   str.push_back('\r');
   str.push_back('\n');
   mMessage["body"] = str.c_str();
}

Message& Mail::getMessage()
{
   return mMessage;
}

bool Mail::shouldTransferEncodeBody()
{
   bool rval = false;

   if(mMessage["headers"]->hasMember("Content-Transfer-Encoding"))
   {
      const char* encoding =
         mMessage["headers"]["Content-Transfer-Encoding"]->getString();
      if(strcasecmp(encoding, "base64") == 0)
      {
         rval = true;
      }
   }

   return rval;
}

string Mail::getTransferEncodedBody()
{
   string rval = mMessage["body"]->getString();

   bool encoded = false;
   if(mMessage["headers"]->hasMember("Content-Transfer-Encoding"))
   {
      const char* encoding =
         mMessage["headers"]["Content-Transfer-Encoding"]->getString();
      if(strcasecmp(encoding, "base64") == 0)
      {
         // base64 encode message
         rval = Base64Codec::encode(
            rval.c_str(), rval.length(), Base64Codec::sMaxMimeLength);
         encoded = true;
      }
   }

   if(!encoded)
   {
      // use default smtp-encoding
      smtpMessageEncode(rval);
   }

   return rval;
}

string Mail::toTemplate()
{
   string str;

   // add headers
   DynamicObjectIterator i = mMessage["headers"].getIterator();
   while(i->hasNext())
   {
      DynamicObject& header = i->next();

      if(header->getType() == Array)
      {
         DynamicObjectIterator ii = header.getIterator();
         while(ii->hasNext())
         {
            DynamicObject& each = ii->next();
            str.append(i->getName());
            str.append(": ");
            str.append(each->getString());
            str.append("\r\n");
         }
      }
      else
      {
         str.append(i->getName());
         str.append(": ");
         str.append(header->getString());
         str.append("\r\n");
      }
   }

   // terminate headers
   str.append("\r\n");

   // add body
   str.append(mMessage["body"]->getString());

   // escape all '{'
   StringTools::replaceAll(str, "{", "{:ldelim}");

   // escape all '}' that do not follow '{:ldelim'
   string::size_type pos = str.find('}');
   while(pos != string::npos)
   {
      if(pos < 8 || strncmp(str.c_str() + pos - 8, "{:ldelim}", 9) != 0)
      {
         str.replace(pos, 1, "{:rdelim}");
         pos += 8;
      }
      pos = str.find('}', pos + 1);
   }

   return str;
}

string& Mail::smtpMessageEncode(string& str)
{
   // insert second dot for any line that starts with a dot
   return StringTools::replaceAll(str, "\r\n.", "\r\n..");
}
