package org.latte.libport;

import java.net.Socket;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;


public class P1 {
    public static final int abacPort = PortManager.abacPort;
    public static void main()
    {
        int value = 0;
        try {
            Socket s = new Socket("127.0.0.1", abacPort);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(s.getInputStream()));
            value = in.read();
            s.close();
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
        System.exit(value);
    }
}
