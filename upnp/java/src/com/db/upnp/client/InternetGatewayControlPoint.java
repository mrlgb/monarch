/*
 * Copyright (c) 2006 Digital Bazaar, Inc.  All rights reserved.
 */
package com.db.upnp.client;

import java.util.Iterator;
import java.util.Vector;

import com.db.upnp.client.igd.InternetGatewayDeviceClient;
import com.db.upnp.device.UPnPDevice;

/**
 * An InternetGatewayControlPoint is a UPnPControlPoint specifically used to
 * control Internet Gateway Devices.  
 * 
 * @author Dave Longley
 */
public class InternetGatewayControlPoint extends UPnPControlPoint
{
   /**
    * Creates a new InternetGatewayControlPoint.
    */
   public InternetGatewayControlPoint()
   {
   }
   
   /**
    * Discovers all Internet Gateway Devices and caches them for use. This
    * method may take some time to execute.
    */
   public void discoverInternetGateways()   
   {
      discoverDevices(InternetGatewayDeviceClient.IGD_DEVICE_TYPE);
   }
   
   /**
    * Gets all previously discovered Internet Gateway Devices in a vector.
    * 
    * @return the previously discovered Internet Gateway Devices in a vector.
    */
   public Vector getDiscoveredInternetGateways()
   {
      Vector igds = new Vector();
      
      // get the discovered devices that are Internet Gateways
      Vector devices = getDiscoveredDevices(
         InternetGatewayDeviceClient.IGD_DEVICE_TYPE);
      
      // get all of the device implementations
      for(Iterator i = devices.iterator(); i.hasNext();)
      {
         UPnPDevice device = (UPnPDevice)i.next();
         
         if(device.getImplementation() != null &&
            device.getImplementation() instanceof InternetGatewayDeviceClient)
         {
            igds.add(device.getImplementation());
         }
      }
      
      return igds;
   }
   
   /**
    * Gets all discovered Internet Gateway Devices with the specified external
    * IP address.
    * 
    * @param externalIPAddress the external IP address to check against.
    * 
    * @return all discovered Internet Gateway Devices with the specified
    *         external IP address.
    */
   public Vector getDiscoveredInternetGateways(String externalIPAddress)
   {
      // get all discovered internet gateways
      Vector igds = getDiscoveredInternetGateways();
      
      // remove internet gateways with external IP addresses that don't match
      for(Iterator i = igds.iterator(); i.hasNext();)
      {
         InternetGatewayDeviceClient igd =
            (InternetGatewayDeviceClient)i.next();
         
         try
         {
            String ip = igd.getWanIPConnectionServiceClient().
               getExternalIPAddress();
            if(!ip.equals(externalIPAddress))
            {
               // remove the internet gateway from the list, its external IP
               // does not match
               i.remove();
            }
         }
         catch(Throwable t)
         {
            // remove the internet gateway from the list, its external IP
            // could not be detected
            i.remove();
         }
      }
      
      return igds;
   }
}
