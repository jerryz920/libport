package org.latte.libport;

import com.sun.jna.Library;
import com.sun.jna.Native;




interface LibC extends Library
{
  LibC INSTANCE = (LibC) Native.loadLibrary("c", LibC.class);
  /// Only works for x86-64, x86 it's different
  int GET_LOCAL_PORT = 326;
  int SET_LOCAL_PORT = 327;
  int getpid();
  int syscall(int num, Object ...values);

}
