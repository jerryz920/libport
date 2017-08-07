package org.latte.libport;

import com.sun.jna.Library;
import com.sun.jna.Native;


interface CLib extends Library
{
  CLib INSTANCE = (CLib) Native.loadLibrary("c", CLib.class);
  int getpid();
  int syscall(int num, Object ...values);
}

interface PortLib extends Library
{
  PortLib INSTANCE = (PortLib) Native.loadLibrary("port-core", PortLib.class);
  int syscallused();
}



public class PortManager
{
    public static void main( String[] args )
    {
      	System.out.println(CLib.INSTANCE.getpid());
	System.out.println(CLib.INSTANCE.syscall(39));
	System.out.println(PortLib.INSTANCE.syscallused());
    }
}
