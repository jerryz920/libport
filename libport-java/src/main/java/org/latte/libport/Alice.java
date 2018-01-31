
package org.latte.libport;

public class Alice {
    public static void main( String[] args ) {
        LibPort.INSTANCE.liblatte_init("alice", 0, "");
        LibPort.INSTANCE.liblatte_post_object_acl("alice:access", "access");
    }
};
