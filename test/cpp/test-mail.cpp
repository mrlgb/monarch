/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#include <iostream>
#include <sstream>

#include "db/test/Test.h"
#include "db/test/Tester.h"
#include "db/test/TestRunner.h"
#include "db/io/ByteArrayInputStream.h"
#include "db/mail/SmtpClient.h"
#include "db/mail/MailTemplateParser.h"
#include "db/net/Url.h"

using namespace std;
using namespace db::io;
using namespace db::net;
using namespace db::test;
using namespace db::rt;

void runSmtpClientTest(TestRunner& tr)
{
   tr.test("SmtpClient");
   
   // set url of mail server
   Url url("smtp://localhost:25");
   
   // set mail
   db::mail::Mail mail;
   mail.setSender("testuser@bitmunk.com");
   mail.addTo("support@bitmunk.com");
   mail.addCc("support@bitmunk.com");
   mail.setSubject("This is an autogenerated unit test email");
   mail.setBody("This is the test body");
   
   // send mail
   db::mail::SmtpClient c;
   c.sendMail(&url, &mail);
   
   tr.passIfNoException();
}

void runMailTemplateParser(TestRunner& tr)
{
   tr.test("MailTemplateParser");
   
   // create mail template
   const char* tpl =
      "From: testuser@bitmunk.com\r\n"
      "To: support@bitmunk.com\r\n"
      "Cc: support@bitmunk.com\r\n"
      "Bcc: $bccAddress1\r\n"
      "Subject: This is an autogenerated unit test email\r\n"
      "This is the test body. I want \\$10.00.\n"
      "I used a variable: \\$bccAddress1 with the value of "
      "'$bccAddress1'.\n"
      "Slash before variable \\\\$bccAddress1.\n"
      "2 slashes before variable \\\\\\\\$bccAddress1.\n"
      "Slash before escaped variable \\\\\\$bccAddress1.\n"
      "2 slashes before escaped variable \\\\\\\\\\$bccAddress1.\n"
      "$eggs$bacon$ham$sausage.";
   
   // create template parser
   db::mail::MailTemplateParser parser;
   
   // create input stream
   ByteArrayInputStream bais(tpl, strlen(tpl));
   
   // create variables
   DynamicObject vars;
   vars["bccAddress1"] = "support@bitmunk.com";
   vars["eggs"] = "This is a ";
   //vars["bacon"] -- no bacon
   vars["ham"] = "number ";
   vars["sausage"] = 5;
   
   // parse mail
   db::mail::Mail mail;
   parser.parse(&mail, vars, &bais);
   
   const char* expect =
      "This is the test body. I want $10.00.\r\n"
      "I used a variable: $bccAddress1 with the value of "
      "'support@bitmunk.com'.\r\n"
      "Slash before variable \\support@bitmunk.com.\r\n"
      "2 slashes before variable \\\\support@bitmunk.com.\r\n"
      "Slash before escaped variable \\$bccAddress1.\r\n"
      "2 slashes before escaped variable \\\\$bccAddress1.\r\n"
      "This is a number 5.\r\n";
   
   // get mail message
   db::mail::Message msg = mail.getMessage();
   
   // assert body parsed properly
   const char* body = msg["body"]->getString();
   assertStrCmp(body, expect);
   
//   // print out mail message
//   cout << "\nHeaders=\n";
//   DynamicObjectIterator i = msg["headers"].getIterator();
//   while(i->hasNext())
//   {
//      DynamicObject header = i->next();
//      DynamicObjectIterator doi = header.getIterator();
//      while(doi->hasNext())
//      {
//         cout << i->getName() << ": " << doi->next()->getString() << endl;
//      }
//   }
//   
//   cout << "Expect=\n" << expect << endl;
//   cout << "Body=\n" << msg["body"]->getString() << endl;
   
//   // set url of mail server
//   Url url("smtp://localhost:25");
//   
//   // send mail
//   db::mail::SmtpClient c;
//   c.sendMail(&url, &mail);
   
   tr.passIfNoException();
}

class DbMailTester : public db::test::Tester
{
public:
   DbMailTester()
   {
      setName("mail");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      //runSmtpClientTest(tr);
      runMailTemplateParser(tr);
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      return 0;
   }
};

#ifndef DB_TEST_NO_MAIN
DB_TEST_MAIN(DbMailTester)
#endif
