package org.latte.libport;

import java.net.ServerSocket;
import java.net.Socket;
import java.net.InetAddress;
import java.io.OutputStream;
import java.io.IOException;
import java.lang.reflect.Field;


public class PortManager
{
    public static final int abacPort = 10111;

    public static void testP1()
    {
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-jar", "libport.jar", "P1");
        int pid1;
        try {
            Process p = pb.start();
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            pid1 = f.getInt(p);
            LibPort.INSTANCE.create_principal(pid1, "image_p1", "config-p1", 20);
            int ret = p.waitFor();
            if (ret < 0) {
                System.err.println("expect P1 to exit normally");
                System.exit(1);
            }
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of P1");
        }
    }

    public static void testP2()
    {
        /// reuse the same process but create with different image
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-jar", "libport.jar", "P1");

        int pid;
        try {
            Process p = pb.start();
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            pid = f.getInt(p);
            LibPort.INSTANCE.create_principal(pid, "image_p2", "config-p2", 20);
            int ret = p.waitFor();
            if (ret == 0) {
                System.err.println("expect P2 to exit abnormally");
                System.exit(1);
            }
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of P2");
        }
    }

    public static void testAttester()
    {
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-jar", "libport.jar", "Attester");

        int pid;
        try {
            Process p = pb.start();
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            pid = f.getInt(p);
            LibPort.INSTANCE.create_principal(pid, "image_attester", "config-attester", 100);
            int ret = p.waitFor();
            if (ret == 0) {
                System.err.println("expect Attester to exit abnormally");
                System.exit(1);
            }
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of Attester");
        }

    }

    public static void doWorker(Socket s)
    {
        try {
            OutputStream out = s.getOutputStream();
            InetAddress addr = s.getInetAddress();
            String hostIP = addr.getHostAddress();
            int port = s.getPort();
            int value = LibPort.INSTANCE.attest_principal_access(hostIP, port, "alice:access");
            out.write(value);
        } catch (IOException e) {
            System.err.println("uncaught io exception: " + e);
            e.printStackTrace();
        } finally {
            try {
                s.close();
            } catch (IOException e) {
                System.err.println("close error");
            }
        }
    }

    public static void startServerThread()
    {
        ServerSocket s = null;
        try {
            s = new ServerSocket(abacPort);
        } catch (IOException e) {
            System.err.println("error creating server socket: " + e);
            System.exit(1);
        }
        try {
            while (true) {
                Socket c = s.accept();
                doWorker(c);
            }
        } catch (IOException e) {
            System.err.println("uncaught exception: " + e);
            e.printStackTrace();
        } finally {
            try {
                s.close();
            } catch (IOException e) {
                System.err.println("uncaught exception: " + e);
                e.printStackTrace();
            }
        }
    }


    public static void main( String[] args ) 
    {
        int pid = LibC.INSTANCE.getpid();
        startServerThread();

        LibC.INSTANCE.syscall(LibC.SET_LOCAL_PORT, 0, 30000, 40000);
        LibPort.INSTANCE.libport_init("http://10.10.1.39:7777",
                "/tmp/libport-javawrapper" + pid, 1);
        LibPort.INSTANCE.create_image("image_p1", "git://github.com/jerryz920/p1", "C27571ADE", "");
        LibPort.INSTANCE.endorse_image("image_p1", "access");
        LibPort.INSTANCE.create_image("image_p2", "git://github.com/jerryz920/p1", "C27571ADE", "");
        LibPort.INSTANCE.post_object_acl("alice:access", "access");

        testP1();
        testP2();
        testAttester();
    }
}
