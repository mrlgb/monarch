/*
 * Copyright (c) 2004-2006 Digital Bazaar, Inc.  All rights reserved.
 */
package com.db.data.format;

/**
 * An ID3Tag Frame.
 * 
 * @author Manu Sporny
 * @author Dave Longley
 */
public class ID3TagFrame
{
   /**
    * The frame header.
    */
   protected byte[] mFrameHeader;
   
   /**
    * The frame data.
    */
   protected byte[] mFrameData; 
   
   /**
    * Default constructor
    */
   public ID3TagFrame()
   {
      mFrameHeader = new byte[10];
      mFrameData = new byte[1];
   }

   /**
    * Sets the name of the ID3 Tag frame. 
    * 
    * @param name the name of this frame.
    * @return true if successful, false otherwise
    */
   public boolean setName(String name)
   {
      boolean rval = false;
      byte[] bName = name.getBytes();
      
      if(bName.length == 4)
      {
         mFrameHeader[0] = bName[0];
         mFrameHeader[1] = bName[1];
         mFrameHeader[2] = bName[2];
         mFrameHeader[3] = bName[3];
         
         rval = true;
      }
      
      return rval;
   }
  
   /**
    * Gets the name of the ID3 Tag frame.
    * 
    * @return the four character name of the ID3 tag frame. 
    */
   public String getName()
   {
      String s = new String(mFrameHeader, 0, 4);
      return s;
   }
   
   /**
    * Sets the data associated with this frame.
    * 
    * @param s the data associated with this frame.
    * @return true if the set was successful, false otherwise.
    */
   public boolean setData(String s)
   {
      mFrameData = new byte[s.length() + 1];
      System.arraycopy(s.getBytes(), 0, mFrameData, 1, s.length());
      convertToBytes();
      
      return true;
   }
   
   /**
    * Gets the data of the ID3 tag as a human-readable string.
    * 
    * @return the human readable string associated with this ID3 tag.
    */
   public String getDataAsString()
   {
      StringBuffer strValue = new StringBuffer();
      
      for(int i = 0; i < mFrameData.length; i++)
      {
         char c = (char)mFrameData[i];
         if(c != 0)
         {
            strValue.append(c);
         }
      }
      
      return strValue.toString();
   }   
   
   /**
    * Gets the size of this frame.
    * 
    * @return the size in bytes for this frame when converted to a byte array.
    */
   public int getSize()
   {
      return mFrameHeader.length + mFrameData.length;
   }
   
   /**
    * This method initializes this ID3TagFrame using a set of bytes read from
    * a byte stream.
    * 
    * @param b the bytes to use as the tag frame data.
    * @param tagOffset the offset at which the id3 tag frame starts
    * @param length the number of valid bytes in the byte array.
    * 
    * @return true if conversion was successful, false otherwise
    */
   public boolean convertFromBytes(byte[] b, int tagOffset, int length)
   {
      boolean rval = false;
      
      if(tagOffset + 7 < length)
      {
         int b1, b2, b3, b4;
         
         b1 = (b[tagOffset + 4] << 24) & 0x0f000000;
         b2 = (b[tagOffset + 5] << 16) & 0x00ff0000;
         b3 = (b[tagOffset + 6] << 8) & 0x0000ff00;
         b4 = b[tagOffset + 7] & 0x000000ff;

         // get the tag data length
         int tagDataLength = (b1 | b2 | b3 | b4);
         
         // make sure there is enough data for the conversion
         if(tagOffset + 10 + tagDataLength <= length)
         {
            // copy the frame header
            mFrameHeader = new byte[10];
            System.arraycopy(b, tagOffset, mFrameHeader, 0, 10);

            // copy the tag data
            mFrameData = new byte[tagDataLength];
            System.arraycopy(b, tagOffset + 10, mFrameData, 0, tagDataLength);
         
            rval = true;
         }
      }
      
      return rval;
   }

   /**
    * Converts this ID3 Tag Frame into a valid byte array.
    * 
    * @return the byte array encoding of this tag frame.
    */
   public byte[] convertToBytes()
   {
      byte[] b = new byte[mFrameHeader.length + mFrameData.length];
      
      System.arraycopy(mFrameHeader, 0, b, 0, mFrameHeader.length);
      System.arraycopy(mFrameData, 0,
                       b, mFrameHeader.length, mFrameData.length);
      
      int dataLength = mFrameData.length;
      
      // update the tag frame length      
      b[4] = (byte)((dataLength & 0x0f000000) >> 24);
      b[5] = (byte)((dataLength & 0x00ff0000) >> 16);
      b[6] = (byte)((dataLength & 0x0000ff00) >> 8);
      b[7] = (byte)(dataLength  & 0x000000ff);
      
      return b;
   }

   /**
    * Converts this ID3 tag frame into a human-readable string.
    * 
    * @return the human readable string associated with this ID3 tag frame.
    */
   public String convertToString()
   {
      return "[" + getName() + ":" + getSize() + ":'" +
             getDataAsString() + "']";
   }
   
   /**
    * Converts this ID3 tag frame into a human-readable string.
    * 
    * @return the human readable string associated with this ID3 tag frame.
    */
   public String toString()
   {
      return convertToString();
   }
}
