
package org.latte.libport;
import com.sun.jna.Native;


public class Alice {
    public static void main( String[] args ) {
        LibPort.INSTANCE.liblatte_init("alice", 0, "");
        LibPort.INSTANCE.liblatte_post_object_acl("alice:newobject", "access");
	byte[] s = new byte[100];
	LibPort.INSTANCE.liblatte_authip(s, 100);
	System.err.println("ip: "  + Native.toString(s));
	LibPort.INSTANCE.liblatte_speaker(s, 100);
	System.err.println("ip: "  + Native.toString(s));
    }
};
