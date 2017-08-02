package org.latte.portlib;

import com.sun.jna.Library;
import com.sun.jna.Native;


interface CLib extends Library
{
  CLib INSTANCE = (CLib) Native.loadLibrary("c", CLib.class);
  int getpid();
}



public class PortManager
{
    public static void main( String[] args )
    {
      	System.out.println(CLib.INSTANCE.getpid());
    }
}
