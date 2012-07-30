/*
 * Copyright (C) 2011 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.cyanogenmod.settings.device;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;

import android.util.Log;

public class Utils {
    private static final String TAG = "FolioParts_Utils";

    private static final String BUILD_PROP = "/system/build.prop" ;
   
    /**
     * Update a property inside build.prop file
     * 
     * @param key
     * @param value
     */
    public static void updateProp( String key, String value ) {
    	try {    
        	String oldValue = readBuildPropValue(key) ;
            Process p = Runtime.getRuntime().exec( new String[]{ "/system/xbin/su", "-c", 
            	"mount -o remount,rw /system && " + 
                "sed -e 's/" + key + "=" + oldValue + "/" + key + "=" + value + "/g' -i /system/build.prop &&" +
                "mount -o remount,o /system" 
            } );
            p.waitFor() ;
        } catch( Exception e ) {
            Log.e( TAG, "Failed to read " + BUILD_PROP , e ) ;            
        } 
    }
    
    /**
     * Read the value from a propertie in build.prop file
     * 
     * @param key
     * @return
     */
    public static String readBuildPropValue( String key ) {
        BufferedReader brBuffer = null ;
        String sLine = null;
        String res = null ;
        boolean found = false; 
        try {
            brBuffer = new BufferedReader(new FileReader( BUILD_PROP ), 512);
            
            while( ( sLine = brBuffer.readLine() ) != null && !found ) {
                if( sLine.startsWith( key ) ) {
                    found = true ;
                    res = sLine.substring( sLine.indexOf("=") + 1 ) ;
                }
            } 
        } catch( Exception e ) {
            Log.e( TAG, "Failed to read " + BUILD_PROP ) ;
        } finally {
            if( brBuffer != null ) {
                try {
                    brBuffer.close() ;    
                } catch( IOException ioe ) {
                    Log.w( TAG, "Error closing " + BUILD_PROP ) ;
                }
            }            
        }
        return res ;
    }

    private static String debugProcessOutput( Process p ) throws IOException {
        BufferedReader stdInput = new BufferedReader(new 
                InputStreamReader(p.getInputStream()));

           BufferedReader stdError = new BufferedReader(new 
                InputStreamReader(p.getErrorStream()));

           String s ;
           StringBuffer str = new StringBuffer() ;
           // read the output from the command
           str.append("Here is the standard output of the command:\n");
           while ((s = stdInput.readLine()) != null) {
        	   str.append(s);
           }
           
           // read any errors from the attempted command
           str.append("Here is the standard error of the command (if any):\n");
           while ((s = stdError.readLine()) != null) {
        	   str.append(s);
           }
           return str.toString() ;
    }
}
