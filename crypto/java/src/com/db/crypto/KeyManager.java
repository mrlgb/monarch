/*
 * Copyright (c) 2003-2006 Digital Bazaar, Inc.  All rights reserved.
 */
package com.db.crypto;

import com.db.util.Base64Coder;
import com.db.logging.Logger;
import com.db.logging.LoggerManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.security.Key;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;

import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;

/**
 * A class that provides management code for private, public,
 * and secret keys.
 * 
 * This class works with PKCS#8 (ASN.1 DER) encoded private keys and
 * X.509 (ASN.1 DER) encoded public keys. It can also Base64 encode and decode
 * those keys and therefore produce PEM formatted keys.
 * 
 * @author Dave Longley
 */
public class KeyManager
{
   /**
    * The last generated private key.
    */
   protected PrivateKey mPrivateKey;
   
   /**
    * The last generated public key.
    */
   protected PublicKey mPublicKey;
   
   /**
    * The last error that occurred.
    */
   protected String mError;
   
   /**
    * The private key PEM header. 
    */
   public static final String PRIVATE_KEY_PEM_HEADER =
      "-----BEGIN PRIVATE KEY-----";
   
   /**
    * The private key PEM footer. 
    */
   public static final String PRIVATE_KEY_PEM_FOOTER =
      "-----END PRIVATE KEY-----";   
   
   /**
    * The encrypted private key PEM header. 
    */
   public static final String ENCRYPTED_PRIVATE_KEY_PEM_HEADER =
      "-----BEGIN ENCRYPTED PRIVATE KEY-----";
   
   /**
    * The encrypted private key PEM footer. 
    */
   public static final String ENCRYPTED_PRIVATE_KEY_PEM_FOOTER =
      "-----END ENCRYPTED PRIVATE KEY-----";
   
   /**
    * The public key PEM header. 
    */
   public static final String PUBLIC_KEY_PEM_HEADER =
      "-----BEGIN PUBLIC KEY-----";
   
   /**
    * The public key PEM footer. 
    */
   public static final String PUBLIC_KEY_PEM_FOOTER =
      "-----END PUBLIC KEY-----";   
   
   /**
    * Creates a KeyManager. For every new set of keys, a new
    * KeyManager should be instantiated.
    */
   public KeyManager()
   {
      mPrivateKey = null;
      mPublicKey = null;
      
      mError = "";
   }
   
   /**
    * This method is provided for backward compatibility with the old
    * method for encrypting private keys that did not wrap the encrypted
    * private key bytes in a DER encoded EncryptedPrivateKeyInfo.
    *  
    * It SHOULD NOT be extended.
    * 
    * Loads a private key from the passed straight-encrypted private key
    * bytes that are locked with the passed password.
    *
    * @param encryptedKeyBytes the encrypted key bytes.
    * @param password the password to unlock the file.
    * 
    * @return true if successful, false if not.
    * 
    * @deprecated
    */
   private boolean loadPrivateKeyFromEncryptedBytes(
      byte[] encryptedKeyBytes, String password)
   {
      boolean rval = false;
      
      try
      {
         // decrypt the key with the passed password
         byte[] decryptedKey = Cryptor.decrypt(encryptedKeyBytes, password);

         // if decryptedKey is not null, decode the key
         if(decryptedKey != null)
         {
            if((mPrivateKey = decodePrivateKey(decryptedKey)) != null)
            {
               rval = true;
            }
            else
            {
               mError = "password-invalid";
            }
         }
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(), "Unable to load private key.");
         mError = "key-file-not-found";
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }

      return rval;
   }

   /**
    * Generates a pair of keys, one public, one private and stores
    * them internally. The keys are generated using DSA/SHA
    * algorithms with 1024 bit encryption. They can be obtained by
    * calling the appropriate get() method.
    *
    * @return true if successful, false if not.
    */
   public boolean generateKeyPair()
   {
      boolean rval = false;
      
      try
      {
         // uses digital signature algorithm (DSA)
         KeyPairGenerator kpg = KeyPairGenerator.getInstance("DSA");
         
         // set up a secure random number generator for key generation
         // using SHA1 pseudo-random-number-generator provided by Sun
         SecureRandom srng = SecureRandom.getInstance("SHA1PRNG", "SUN");
         
         // initialize the key generator with 1024 bit encryption
         kpg.initialize(1024, srng);
         
         // generate the private and public keys
         KeyPair keyPair = kpg.generateKeyPair();
         mPrivateKey = keyPair.getPrivate();
         mPublicKey = keyPair.getPublic();

         rval = true;
      }
      catch(NoSuchAlgorithmException nsae)
      {
         LoggerManager.getLogger("dbcrypto").error(KeyManager.class,  
            "DSA algorithm not supported.");
         LoggerManager.getLogger("dbcrypto").debug(
            KeyManager.class, Logger.getStackTrace(nsae));
      }
      catch(NoSuchProviderException nspe)
      {
         LoggerManager.getLogger("dbcrypto").error(KeyManager.class,  
            "SUN is not a supported provider.");
         LoggerManager.getLogger("dbcrypto").debug(
            KeyManager.class, Logger.getStackTrace(nspe));
      }

      return rval;
   }
   
   /**
    * Stores a private key in a file with the passed filename
    * and password. The private key is stored as an EncryptedPrivateKeyInfo
    * ASN.1 structure using DER encoding.
    *
    * @param filename the name of the file to store the key in.
    * @param password the password to lock the file with.
    * 
    * @return true if successful, false if not.
    */
   public boolean storePrivateKey(String filename, String password)
   {
      return storePrivateKey(getPrivateKey(), filename, password);
   }
   
   /**
    * Stores a private key in a PEM file with the passed filename
    * and password. The private key is stored as an EncryptedPrivateKeyInfo
    * ASN.1 structure using DER encoding that is base64-encoded along with
    * a PEM header and footer.
    *
    * @param filename the name of the file to store the key in.
    * @param password the password to lock the file with.
    * 
    * @return true if successful, false if not.
    */
   public boolean storePEMPrivateKey(String filename, String password)
   {
      return storePEMPrivateKey(getPrivateKey(), filename, password);
   }
   
   /**
    * Stores a public key in a file with the passed filename.
    *
    * @param filename the name of the file to store the key in.
    * 
    * @return true if successful, false if not.
    */
   public boolean storePublicKey(String filename)
   {
      return storePublicKey(getPublicKey(), filename);
   }
   
   /**
    * Stores a public key in a PEM file with the passed filename.
    *
    * @param filename the name of the file to store the key in.
    * 
    * @return true if successful, false if not.
    */
   public boolean storePEMPublicKey(String filename)
   {
      return storePEMPublicKey(getPublicKey(), filename);
   }   
   
   /**
    * Loads a private key from the passed encrypted key (DER encoded
    * EncryptedPrivateKeyInfo encoded in base64) that is locked with the
    * passed password.
    *
    * @param encryptedKey the encrypted key in a base64-encoded string.
    * @param password the password to unlock the key.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPrivateKey(String encryptedKey, String password)
   {
      boolean rval = false;
      
      Base64Coder base64 = new Base64Coder();
      byte[] bytes = base64.decode(encryptedKey);
      rval = loadPrivateKey(bytes, password);
      
      return rval;
   }
   
   /**
    * Loads a private key from the passed DER encoded EncryptedPrivateKeyInfo
    * bytes that are locked with the passed password.
    *
    * @param encryptedKey the encrypted key in a byte array.
    * @param password the password to unlock the key.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPrivateKey(byte[] encryptedKey, String password)
   {
      boolean rval = false;
      
      try
      {
         // decrypt the private key with the passed password
         byte[] decryptedKey = Cryptor.decryptPrivateKey(
            encryptedKey, password);
         
         // see if the decryption worked
         if(decryptedKey != null)
         {
            // decode the key
            if((mPrivateKey = decodePrivateKey(decryptedKey)) != null)
            {
               rval = true;
            }
            else
            {
               mError = "password-invalid";
            }
         }
         else
         {
            // provided for backwards compatibility: try a straight
            // decryption of the given bytes instead of interpreting the
            // bytes as an EncryptedPrivateKeyInfo ASN.1 structure
            rval = loadPrivateKeyFromEncryptedBytes(encryptedKey, password);
         }
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(), "Unable to load private key.");
         mError = "key-file-not-found";
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }

      return rval;
   }
   
   /**
    * Loads a private key from the passed PEM formatted string (DER encoded
    * EncryptedPrivateKeyInfo encoded in base64 with a PEM header and
    * footer) that is locked with the passed password.
    * 
    * @param pem the pem string.
    * @param password the password to unlock the key.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPEMPrivateKey(String pem, String password)
   {
      boolean rval = false;
      
      try
      {
         // strip PEM header and footer
         int startIndex =
            pem.indexOf(ENCRYPTED_PRIVATE_KEY_PEM_HEADER) +
            ENCRYPTED_PRIVATE_KEY_PEM_HEADER.length();
         int endIndex = pem.indexOf(ENCRYPTED_PRIVATE_KEY_PEM_FOOTER);
         String encryptedKey = pem.substring(startIndex, endIndex);
         
         // load encrypted key
         rval = loadPrivateKey(encryptedKey, password);      
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(),
            "Unable to load PEM encrypted private key.");
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }
      
      return rval;
   }

   /**
    * Loads a private key from the file with the passed filename,
    * that is locked with the passed password. The filename should point at
    * a file that has the private key stored as an EncryptedPrivateKeyInfo
    * ASN.1 structure using DER encoding.
    *
    * @param filename the file that contains the private key.
    * @param password the password to unlock the file.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPrivateKeyFromFile(String filename, String password)
   {
      boolean rval = false;
      
      try
      {
         // open encrypted-encoded private key file, get bytes
         File file = new File(filename);
         FileInputStream fis = new FileInputStream(file);
         byte[] bytes = new byte[(int)file.length()];
         fis.read(bytes);
         fis.close();
         
         // load encrypted key
         rval = loadPrivateKey(bytes, password);
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(), "Unable to load private key.");
         mError = "key-file-not-found";
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }

      return rval;
   }   
   
   /**
    * Loads a private key from the PEM file with the passed filename,
    * that is locked with the passed password. The filename should point
    * at a file with private key in PEM format -- that is an
    * EncryptedPrivateKeyInfo stored in DER-encoded format that is base64
    * encoded with a PEM header and footer.
    *
    * @param filename the file that contains the private key.
    * @param password the password to unlock the file.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPEMPrivateKeyFromFile(String filename, String password)
   {
      boolean rval = false;
      
      try
      {
         // open encrypted-encoded private key file, get bytes
         File file = new File(filename);
         FileInputStream fis = new FileInputStream(file);
         byte[] bytes = new byte[(int)file.length()];
         fis.read(bytes);
         fis.close();
         
         // load PEM encrypted key
         rval = loadPEMPrivateKey(new String(bytes), password);
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(), "Unable to load private key.");
         mError = "key-file-not-found";
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }

      return rval;
   }

   /**
    * Loads a public key from the file with the passed filename. The filename
    * should point to a file that has a X.509 public key in DER encoded format.
    * 
    *
    * @param filename the file that contains the public key.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPublicKeyFromFile(String filename)
   {
      boolean rval = false;
      
      try
      {
         // open encoded public key file, get bytes
         File file = new File(filename);
         FileInputStream fis = new FileInputStream(file);
         byte[] bytes = new byte[(int)file.length()];
         fis.read(bytes);
         fis.close();

         rval = ((mPublicKey = decodePublicKey(bytes)) != null);
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(), "Unable to load public key.");
         mError = "key-file-not-found";
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }

      return rval;
   }
   
   /**
    * Loads a public key from the PEM file with the passed filename.
    *
    * @param filename the file that contains the public key.
    * 
    * @return true if successful, false if not.
    */
   public boolean loadPEMPublicKeyFromFile(String filename)
   {
      boolean rval = false;
      
      try
      {
         // open encoded public key file, get bytes
         File file = new File(filename);
         FileInputStream fis = new FileInputStream(file);
         byte[] bytes = new byte[(int)file.length()];
         fis.read(bytes);
         fis.close();
         
         // strip PEM header and footer
         String pem = new String(bytes);
         int startIndex =
            pem.indexOf(PUBLIC_KEY_PEM_HEADER) + PUBLIC_KEY_PEM_HEADER.length();
         int endIndex = pem.indexOf(PUBLIC_KEY_PEM_FOOTER);
         String key = pem.substring(startIndex, endIndex);

         rval = ((mPublicKey = decodePublicKey(key)) != null);
      }
      catch(Throwable t)
      {
         getLogger().error(getClass(), "Unable to load public key.");
         mError = "key-file-not-found";
         getLogger().debug(getClass(), Logger.getStackTrace(t));
      }

      return rval;
   }

   /**
    * Returns the internal private key.
    * 
    * @return the internal private key.
    */
   public PrivateKey getPrivateKey()
   {
      return mPrivateKey;
   }
   
   /**
    * Encrypts the private key and returns it in DER format. The private
    * key is stored as an EncryptedPrivateKeyInfo ASN.1 structure using DER
    * encoding.
    *
    * @param password the password to lock the key with.
    * 
    * @return the DER formatted encrypted private key or null.
    */
   public byte[] getEncryptedPrivateKey(String password)   
   {
      byte[] encryptedKey = null;
      
      PrivateKey key = getPrivateKey();
      if(key != null)
      {
         encryptedKey = getEncryptedPrivateKey(key, password);
      }
      
      return encryptedKey;      
   }
   
   /**
    * Returns the internal public key.
    * 
    * @return the internal public key.
    */
   public PublicKey getPublicKey()
   {
      return mPublicKey;
   }
   
   /**
    * Returns a string that represents a Base64-PKCS8-encoded
    * private key. This method PKCS8-encodes the previously generated
    * private key in a byte array and then Base64-encodes the byte
    * array to produce a string.
    *
    * @return the Base64-PKCS8-encoded string or null.
    */
   public String getPrivateKeyString()
   {
      return base64EncodeKey(getPrivateKey());
   }
   
   /**
    * Returns a string that represents a Base64-X509-encoded
    * public key. This method X509-encodes the previously generated
    * public key in a byte array and then Base64-encodes the byte
    * array to produce a string.
    *
    * @return the Base64-X509-encoded string or null.
    */
   public String getPublicKeyString()
   {
      return base64EncodeKey(getPublicKey());
   }
   
   /**
    * Returns a PEM private key. It is a string that represents a
    * Base64-PKCS8-encoded private key that includes a header/footer describing
    * the key as a private key. This method PKCS8-encodes the previously
    * generated private key in a byte array and then Base64-encodes the byte
    * array to produce a string. It then adds the PRIVATE KEY header and
    * footer.
    *
    * @return the PEM formatted private key or null.
    */
   public String getPEMPrivateKey()
   {
      String pem = null;
      
      String key = getPrivateKeyString();
      if(key != null)
      {
         pem =
            PRIVATE_KEY_PEM_HEADER +
            "\n" + key + "\n" +
            PRIVATE_KEY_PEM_FOOTER;
      }
      
      return pem;
   }
   
   /**
    * Encrypts the private key and returns it in PEM format. The private
    * key is stored as an EncryptedPrivateKeyInfo ASN.1 structure using DER
    * encoding that is base64-encoded along with a PEM header and footer.
    *
    * @param password the password to lock the key with.
    * 
    * @return the PEM formatted encrypted private key or null.
    */
   public String getPEMEncryptedPrivateKey(String password)   
   {
      String pem = null;
      
      PrivateKey key = getPrivateKey();
      if(key != null)
      {
         pem = getPEMEncryptedPrivateKey(key, password);
      }
      
      return pem; 
   }
   
   /**
    * Returns a PEM public key. It is a string that represents a
    * Base64-X509-encoded public key that includes a header/footer describing
    * the key as a public key. This method X509-encodes the previously
    * generated public key in a byte array and then Base64-encodes the byte
    * array to produce a string. It then adds a PUBLIC KEY header and footer.
    *
    * @return the PEM formatter public key or null.
    */
   public String getPEMPublicKey()
   {
      String pem = null;
      
      String key = getPublicKeyString();
      if(key != null)
      {
         pem =
            PUBLIC_KEY_PEM_HEADER +
            "\n" + key + "\n" +
            PUBLIC_KEY_PEM_FOOTER;
      }
      
      return pem;      
   }   
   
   /**
    * Clears the error flag.
    */
   public void clearError()
   {
      mError = "";
   }
   
   /**
    * Returns the last error that occurred.
    * 
    * @return the last error that occurred.
    */
   public String getError()
   {
      return mError;
   }
   
   /**
    * Gets the logger for this key manager.
    * 
    * @return the logger.
    */
   public Logger getLogger()
   {
      return LoggerManager.getLogger("dbcrypto");
   }

   /**
    * Encodes the passed key that is in raw encoded byte form and
    * and encodes it with Base64 encoding. A string is the result.
    *
    * @param bytes the key in raw encoded byte form.
    * 
    * @return the Base64-encoded string.
    */
   public static String base64EncodeKey(byte[] bytes)
   {
      Base64Coder encoder = new Base64Coder();
      return encoder.encode(bytes);
   }

   /**
    * Encodes the passed key by retrieving its raw encoded bytes
    * and encoding them with Base64 encoding. A string is the result.
    *
    * @param key the key to encode.
    * 
    * @return the Base64-encoded string.
    */
   public static String base64EncodeKey(Key key)
   {
      return base64EncodeKey(key.getEncoded());
   }
   
   /**
    * Decodes the passed key from a Base64-encoded string into its
    * encoded byte form.
    * 
    * @param encodedKey the encoded key in Base64-encoded string form.
    * 
    * @return the encodedKey in byte form.
    */
   public static byte[] base64DecodeKey(String encodedKey)
   {
      byte[] key = null;
      
      if(encodedKey != null)
      {
         try
         {
            Base64Coder decoder = new Base64Coder(); 
            key = decoder.decode(encodedKey);
         }
         catch(Exception e)
         {
            LoggerManager.getLogger("dbcrypto").debug(
               KeyManager.class, Logger.getStackTrace(e));
         }
      }
      
      return key;
   }   
   
   /**
    * Decodes any private key that is PKCS8-encoded.
    *
    * @param encodedKey the PKCS8-encoded byte array.
    * 
    * @return the decoded private key object.
    */
   public static PrivateKey decodePrivateKey(byte[] encodedKey)
   {
      PrivateKey privateKey = null;

      if(encodedKey != null)
      {
         try
         {
            PKCS8EncodedKeySpec keySpec = new PKCS8EncodedKeySpec(encodedKey);
            KeyFactory keyFactory = KeyFactory.getInstance("DSA");
            privateKey = keyFactory.generatePrivate(keySpec);
         }
         catch(NoSuchAlgorithmException nsae)
         {
            LoggerManager.getLogger("dbcrypto").error(KeyManager.class,  
               "DSA algorithm is not supported.");
            LoggerManager.getLogger("dbcrypto").debug(KeyManager.class, 
               Logger.getStackTrace(nsae));
         }
         catch(InvalidKeySpecException ikse)
         {
            LoggerManager.getLogger("dbcrypto").error(
               KeyManager.class, "KeySpec is invalid.");
            LoggerManager.getLogger("dbcrypto").debug(KeyManager.class, 
               Logger.getStackTrace(ikse));
         }
      }
      
      return privateKey;
   }
   
   /**
    * Decodes any public key that is represented by a string
    * that is Base64-PKCS8-encoded. The passed string is
    * first Base64-decoded into a byte array, and then PKCS8-decoded
    * into a PrivateKey object.
    *
    * @param encodedKey the Base64-PKCS8(DER)-encoded string.
    * 
    * @return the decoded public key object.
    */
   public static PrivateKey decodePrivateKey(String encodedKey)
   {
      PrivateKey privateKey = null;
      
      if(encodedKey != null)
      {
         privateKey = decodePrivateKey(base64DecodeKey(encodedKey));
      }

      return privateKey;
   }   

   /**
    * Decodes any public key that is X509-encoded.
    *
    * @param encodedKey the X509-encoded byte array.
    * 
    * @return the decoded public key object.
    */
   public static PublicKey decodePublicKey(byte[] encodedKey)
   {
      PublicKey publicKey = null;

      if(encodedKey != null)
      {
         try
         {
            X509EncodedKeySpec keySpec = new X509EncodedKeySpec(encodedKey);
            KeyFactory keyFactory = KeyFactory.getInstance("DSA");
            publicKey = keyFactory.generatePublic(keySpec);
         }
         catch(NoSuchAlgorithmException nsae)
         {
            LoggerManager.getLogger("dbcrypto").error(KeyManager.class, 
               "DSA algorithm is not supported.");
            LoggerManager.getLogger("dbcrypto").debug(KeyManager.class, 
               Logger.getStackTrace(nsae));
         }
         catch(InvalidKeySpecException ikse)
         {
            LoggerManager.getLogger("dbcrypto").error(
               KeyManager.class, "KeySpec is invalid.");
            LoggerManager.getLogger("dbcrypto").debug(KeyManager.class, 
               Logger.getStackTrace(ikse));
         }
      }

      return publicKey;
   }

   /**
    * Decodes any public key that is represented by a string
    * that is Base64-X509-encoded. The passed string is
    * first Base64-decoded into a byte array, and then X509-decoded
    * into a PublicKey object.
    *
    * @param encodedKey the Base64-X509-encoded string.
    * 
    * @return the decoded public key object.
    */
   public static PublicKey decodePublicKey(String encodedKey)
   {
      PublicKey publicKey = null;
      
      if(encodedKey != null)
      {
         publicKey = decodePublicKey(base64DecodeKey(encodedKey));
      }

      return publicKey;
   }

   /**
    * Decodes a DES secret key from its encoded byte form.
    *
    * @param encodedKey the encoded byte array of key material.
    * 
    * @return the decoded secret key object.
    */
   public static SecretKey decodeDESKey(byte[] encodedKey)
   {
      SecretKey secretKey = null;

      if(encodedKey != null)
      {
         try
         {
            DESKeySpec keySpec = new DESKeySpec(encodedKey);
            SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("DES");
            secretKey = keyFactory.generateSecret(keySpec);
         }
         catch(Throwable t)
         {
            LoggerManager.getLogger("dbcrypto").debug(
               KeyManager.class, Logger.getStackTrace(t));
         }
      }

      return secretKey;
   }
   
   /**
    * Encrypts the passed private key and returns it DER format. The private
    * key is stored as an EncryptedPrivateKeyInfo ASN.1 structure using DER
    * encoding.
    *
    * @param key the private key to encrypt.
    * @param password the password to lock the key with.
    * 
    * @return the DER formatted encrypted private key.
    */
   public static byte[] getEncryptedPrivateKey(PrivateKey key, String password)
   {
      // encrypt the key with the passed password
      byte[] bytes = Cryptor.encryptPrivateKey(
         key.getEncoded(), password);

      return bytes;
   }
   
   /**
    * Encrypts the passed private key and returns it in PEM format. The private
    * key is stored as an EncryptedPrivateKeyInfo ASN.1 structure using DER
    * encoding that is base64-encoded along with a PEM header and footer.
    *
    * @param key the private key to encrypt.
    * @param password the password to lock the key with.
    * 
    * @return the PEM formatted encrypted private key.
    */
   public static String getPEMEncryptedPrivateKey(
      PrivateKey key, String password)   
   {
      // encrypt the key with the passed password
      byte[] bytes = Cryptor.encryptPrivateKey(
         key.getEncoded(), password);
      
      // base64 encode bytes
      String base64 = base64EncodeKey(bytes);
      
      // add PEM header and footer
      String pem =
         ENCRYPTED_PRIVATE_KEY_PEM_HEADER +
         "\n" + base64 + "\n" +
         ENCRYPTED_PRIVATE_KEY_PEM_FOOTER;
      
      return pem;
   }

   /**
    * Stores the passed private key in a file with the passed filename
    * and password. The private key is stored as an EncryptedPrivateKeyInfo
    * ASN.1 structure using DER encoding.
    *
    * @param key the private key to store.
    * @param filename the name of the file to store the key in.
    * @param password the password to lock the file with.
    * 
    * @return true if successful, false if not.
    */
   public static boolean storePrivateKey(
      PrivateKey key, String filename, String password)
   {
      boolean rval = false;
      
      if(key != null)
      {
         try
         {
            // encrypt the key with the passed password
            byte[] bytes = getEncryptedPrivateKey(key, password);
         
            // create private key file, write encrypted-encoded bytes
            File file = new File(filename);
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(bytes);
            fos.close();
            
            rval = true;
         }
         catch(Throwable t)
         {
            LoggerManager.getLogger("dbcrypto").debug(KeyManager.class, 
               "Unable to write private key to file.");
            LoggerManager.getLogger("dbcrypto").debug(
               KeyManager.class, Logger.getStackTrace(t));
         }
      }

      return rval;
   }
   
   /**
    * Stores the passed private key in a PEM file with the passed filename
    * and password. The private key is stored as an EncryptedPrivateKeyInfo
    * ASN.1 structure using DER encoding that is base64-encoded along with a
    * PEM header and footer.
    *
    * @param key the private key to store.
    * @param filename the name of the file to store the key in.
    * @param password the password to lock the file with.
    * 
    * @return true if successful, false if not.
    */
   public static boolean storePEMPrivateKey(
      PrivateKey key, String filename, String password)
   {
      boolean rval = false;
      
      if(key != null)
      {
         try
         {
            // get PEM encrypted private key
            String pem = getPEMEncryptedPrivateKey(key, password);
         
            // create private key file, write encrypted-encoded bytes
            File file = new File(filename);
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(pem.getBytes());
            fos.close();
            
            rval = true;
         }
         catch(Throwable t)
         {
            LoggerManager.getLogger("dbcrypto").debug(KeyManager.class, 
               "Unable to write private key to file.");
            LoggerManager.getLogger("dbcrypto").debug(
               KeyManager.class, Logger.getStackTrace(t));
         }
      }

      return rval;
   }

   /**
    * Stores the passed public key in a file with the passed filename. The
    * key is stored as an ASN.1 X.509 public key structure using DER encoding.
    *
    * @param key the public key to store.
    * @param filename the name of the file to store the key in.
    * 
    * @return true if successful, false if not.
    */
   public static boolean storePublicKey(PublicKey key, String filename)
   {
      boolean rval = false;
      
      if(key != null)
      {
         try
         {
            // create public key file, write encoded bytes
            File file = new File(filename);
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(key.getEncoded());
            fos.close();
            
            rval = true;
         }
         catch(Throwable t)
         {
            LoggerManager.getLogger("dbcrypto").error(KeyManager.class, 
               "Unable to write public key to file.");
            LoggerManager.getLogger("dbcrypto").debug(
               KeyManager.class, Logger.getStackTrace(t));
         }
      }
      
      return rval;
   }
   
   /**
    * Stores the passed public key in a PEM file with the passed filename. The
    * key is stored as an ASN.1 X.509 public key structure using DER encoding
    * that is base64-encoded along with a PEM header and footer.
    *
    * @param key the public key to store.
    * @param filename the name of the file to store the key in.
    * 
    * @return true if successful, false if not.
    */
   public static boolean storePEMPublicKey(PublicKey key, String filename)
   {
      boolean rval = false;
      
      if(key != null)
      {
         try
         {
            // base64 encode key bytes
            String base64 = base64EncodeKey(key.getEncoded());
            
            // add PEM header and footer
            String pem =
               PUBLIC_KEY_PEM_HEADER +
               "\n" + base64 + "\n" +
               PUBLIC_KEY_PEM_FOOTER;
            
            // create public key file, write encoded bytes
            File file = new File(filename);
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(pem.getBytes());
            fos.close();
            
            rval = true;
         }
         catch(Throwable t)
         {
            LoggerManager.getLogger("dbcrypto").error(KeyManager.class, 
               "Unable to write public key to file.");
            LoggerManager.getLogger("dbcrypto").debug(
               KeyManager.class, Logger.getStackTrace(t));
         }
      }
      
      return rval;
   }
}
