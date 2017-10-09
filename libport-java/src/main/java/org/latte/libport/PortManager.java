package org.latte.libport;

import java.net.ServerSocket;
import java.net.SocketTimeoutException;

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
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-cp", "libport.jar", "org.latte.libport.P1");
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
            }
            LibPort.INSTANCE.delete_principal(pid1);
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of P1");
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
            LibPort.INSTANCE.create_principal(pid, "image_p2", "config-p2", 20);
            int ret = p.waitFor();
            if (ret == 0) {
                System.err.println("expect P2 to exit abnormally");
            }
            LibPort.INSTANCE.delete_principal(pid);
        } catch (Throwable e) {
            throw new RuntimeException("can not get pid of P2");
        }
    }

    public static void testAttester()
    {
        ProcessBuilder pb = new ProcessBuilder("/usr/bin/latte_exec", "/usr/bin/java", "-cp", "libport.jar", "org.latte.libport.Attester");

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
            System.err.println("recevie request from " + hostIP + ":" + port + "; attest val " + value);
            /// no access (0) -> return abnormally (1)
            // accessible (1) -> return normally (0)
            out.write(value == 0? 1: 0);
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
        new Thread(() -> {
            ServerSocket s = null;
            try {
                s = new ServerSocket(abacPort);
                s.setSoTimeout(100);
            } catch (IOException e) {
                System.err.println("error creating server socket: " + e);
                System.exit(1);
            }
            try {
                while (running) {
                    try {
                        Socket c = s.accept();
                        doWorker(c);
                    } catch (SocketTimeoutException e) {
                        System.err.println("Timeout: " + e);
                    }
                }
                System.err.println("shutting down\n");
            } catch(IOException e) {
                System.err.println("IO exception in server socket:" + e);
            }finally {
                try {
                    s.close();
                } catch (IOException e) {
                    System.err.println("uncaught exception: " + e);
                    e.printStackTrace();
                }
            }
        }).start();
    }


    public static void main( String[] args ) 
    {
        int pid = LibC.INSTANCE.getpid();
        startServerThread();
        LibPort.INSTANCE.libport_set_log_level(LibPort.LOG_DEBUG);
        System.err.println("preparing");

        LibC.INSTANCE.syscall(LibC.SET_LOCAL_PORT, 0, 30000, 40000);
        Utils.reportLocalPorts("test-main");
        LibPort.INSTANCE.libport_init("http://10.10.1.39:7777",
                "/tmp/libport-javawrapper" + pid, 1);
        LibPort.INSTANCE.create_image("image_p1", "git://github.com/jerryz920/p1", "C27571ADE", "");
        LibPort.INSTANCE.endorse_image("image_p1", "access");
        LibPort.INSTANCE.create_image("image_p2", "git://github.com/jerryz920/p1", "C27571ADE", "");
        LibPort.INSTANCE.post_object_acl("alice:access", "access");
        System.err.println("after posting object acl");

        testP1();
        System.err.println("after p1");
        testP2();
        System.err.println("after p2");
        if (args.length > 0) {
            System.err.println("args0: " + args[0]);
            System.exit(0);
        }
        testAttester();
        System.err.println("after attester");
        running = false;
    }
    private static boolean running = true;
}
