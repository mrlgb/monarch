/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "db/test/Test.h"
#include "db/test/Tester.h"
#include "db/test/TestRunner.h"
#include "db/io/ByteArrayInputStream.h"
#include "db/data/xml/DomTypes.h"
#include "db/upnp/SoapEnvelope.h"

using namespace std;
using namespace db::test;
using namespace db::data::xml;
using namespace db::io;
using namespace db::rt;
using namespace db::upnp;

void runSoapEnvelopeTest(TestRunner& tr)
{
   tr.group("SoapEnvelope");
   
   tr.test("create");
   {
      const char* expect =
         "<soap:Envelope "
         "soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding\" "
         "xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope\">"
         "<soap:Body xmlns:m=\"http://www.example.org/stock\">"
         "<m:GetStockPrice>"
         "<m:StockName>IBM</m:StockName>"
         "</m:GetStockPrice>"
         "</soap:Body>"
         "</soap:Envelope>";
      
      SoapEnvelope env;
      SoapMessage msg;
      msg["name"] = "GetStockPrice";
      msg["namespace"] = "http://www.example.org/stock";
      msg["params"]["StockName"] = "IBM";
      string envelope = env.create(msg);
      
      assertStrCmp(expect, envelope.c_str());
   }
   tr.passIfNoException();
   
   tr.test("parse message");
   {
      const char* expect =
         "<soap:Envelope "
         "soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding\" "
         "xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope\">"
         "<soap:Body xmlns:m=\"http://www.example.org/stock\">"
         "<m:GetStockPrice>"
         "<m:StockName>IBM</m:StockName>"
         "</m:GetStockPrice>"
         "</soap:Body>"
         "</soap:Envelope>";
      
      ByteArrayInputStream bais(expect, strlen(expect));
      
      SoapEnvelope env;
      SoapResult result;
      env.parse(&bais, result);
      assertNoException();
      
      // result is not a fault
      assert(!result["fault"]->getBoolean());
      
      // compare message
      SoapMessage expectMessage;
      expectMessage["name"] = "GetStockPrice";
      expectMessage["namespace"] = "http://www.example.org/stock";
      expectMessage["params"]["StockName"] = "IBM";
      assert(expectMessage == result["message"]);
   }
   tr.passIfNoException();
   
   tr.test("parse fault");
   {
      const char* expect =
         "<soap:Envelope "
         "soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding\" "
         "xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope\">"
         "<soap:Body xmlns:m=\"http://www.example.org/stock\">"
         "<soap:Fault>"
         "<faultcode>soap:Client.AppError</faultcode>"
         "<faultstring>Application Error</faultstring>"
         "<detail>"
         "<message>You did something wrong.</message>"
         "<errorcode>1000</errorcode>"
         "</detail>"
         "</soap:Fault>"
         "</soap:Body>"
         "</soap:Envelope>";
      
      ByteArrayInputStream bais(expect, strlen(expect));
      
      SoapEnvelope env;
      SoapResult result;
      env.parse(&bais, result);
      assertNoException();
      
      // result is a fault
      assert(result["fault"]->getBoolean());
      
      // compare message
      SoapMessage expectMessage;
      expectMessage["name"] = "Fault";
      expectMessage["namespace"] = "http://schemas.xmlsoap.org/soap/envelope";
      expectMessage["params"]["faultcode"] = "soap:Client.AppError";
      expectMessage["params"]["faultstring"] = "Application Error";
      expectMessage["params"]["detail"]["message"] = "You did something wrong.";
      expectMessage["params"]["detail"]["errorcode"] = 1000;
      assert(expectMessage == result["message"]);
   }
   tr.passIfNoException();
   
   tr.ungroup();
}

void runPortMappingTest(TestRunner& tr)
{
   tr.group("PortMapping");
   
   tr.test("remove if exists");
   {
      // FIXME:
   }
   tr.passIfNoException();
   
   tr.test("add");
   {
      // FIXME:
   }
   tr.passIfNoException();
   
   tr.test("remove");
   {
      // FIXME:
   }
   tr.passIfNoException();
}

class DbUpnpTester : public db::test::Tester
{
public:
   DbUpnpTester()
   {
      setName("upnp");
   }

   /**
    * Run automatic unit tests.
    */
   virtual int runAutomaticTests(TestRunner& tr)
   {
      runSoapEnvelopeTest(tr);
      
      return 0;
   }

   /**
    * Runs interactive unit tests.
    */
   virtual int runInteractiveTests(TestRunner& tr)
   {
      runPortMappingTest(tr);
      
      return 0;
   }
};

db::test::Tester* getDbUpnpTester() { return new DbUpnpTester(); }


DB_TEST_MAIN(DbUpnpTester)
