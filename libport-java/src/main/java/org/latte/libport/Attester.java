package org.latte.libport;

import java.net.Socket;
import java.io.IOException;
import java.io.BufferedReader;
import java.lang.reflect.Field;

public class Attester {
    public static final int abacPort = PortManager.abacPort;
    public static void testP1()
    {
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-cp", "libport.jar", "org.latte.libport.P1");

        int pid1;
        try {
            Process p = pb.start();
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            pid1 = f.getInt(p);
            LibPort.INSTANCE.liblatte_create_principal(pid1, "image_p3", "config-p3", 20);
            int ret = p.waitFor();
            if (ret < 0) {
                System.err.println("expect P3 to exit normally");
            }
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of P3");
        }
    }

    public static void testP2()
    {
        /// reuse the same process but create with different image
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-cp", "libport.jar", "org.latte.libport.P1");

        int pid;
        try {
            Process p = pb.start();
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            pid = f.getInt(p);
            LibPort.INSTANCE.liblatte_create_principal(pid, "image_p4", "config-p4", 20);
            int ret = p.waitFor();
            if (ret == 0) {
                System.err.println("expect P4 to exit abnormally");
            }
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of P4");
        }
    }

    public static void main(String[] args)
    {
        int pid = LibC.INSTANCE.getpid();
        Utils.reportLocalPorts("attester" + pid);
        System.err.println("initialize Attester");
        LibPort.INSTANCE.liblatte_init("", 0, "");
        LibPort.INSTANCE.liblatte_endorse_attester("image_p3", "*");
        LibPort.INSTANCE.liblatte_endorse_image("image_p3", "*", 
	    "git://github.com/jerryz920/p2");
        LibPort.INSTANCE.liblatte_endorse_image("image_p3", "*", "access");
        LibPort.INSTANCE.liblatte_endorse_attester("image_p4", "*");
        LibPort.INSTANCE.liblatte_endorse_image("image_p4", "*", 
	    "git://github.com/jerryz920/p2");
        testP1();
        System.err.println("after attester P1");
        testP2();
        System.err.println("after attester P2");
    }
}
