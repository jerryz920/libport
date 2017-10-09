
package org.latte.libport;

import com.sun.jna.Native;
import com.sun.jna.ptr.IntByReference;
import java.io.PrintWriter;
import java.io.IOException;
import java.io.FileWriter;

public class Utils {

    static void reportFailure(String fname)
    {
        try {
            PrintWriter p = new PrintWriter(new FileWriter(fname));
            p.println("fail to get local ports");
            p.close();
        } catch (IOException e) {
            System.err.println("IO exception " + e);
        }
    }
    static void reportRange(String fname, int p1, int p2)
    {
        try {
            PrintWriter p = new PrintWriter(new FileWriter(fname));
            p.println("local range:" + p1 + " " + p2);
            p.close();
        } catch (IOException e) {
            System.err.println("IO exception " + e);
        }
    }


    public static void reportLocalPorts(String fname)
    {
        IntByReference p1 = new IntByReference();
        IntByReference p2 = new IntByReference();
        int ret = LibC.INSTANCE.syscall(LibC.GET_LOCAL_PORT, 0, p1, p2);
        if (ret < 0) {
            reportFailure(fname);
        } else {
            reportRange(fname, p1.getValue(), p2.getValue());
        }
    }

}
