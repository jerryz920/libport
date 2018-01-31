package org.latte.libport;

import com.sun.jna.Library;
import com.sun.jna.Native;



public interface LibPort extends Library
{
  LibPort INSTANCE = (LibPort) Native.loadLibrary("port", LibPort.class);
  public static final int LOG_DEBUG = 7;
  public static final int LOG_INFO = 6;
  public static final int LOG_WARNING = 4;
  int liblatte_init(String speaker, int run_as_iaas, String daemon_path);
  int liblatte_create_principal_new(long uuid, String image, String config,
      int nport, String new_ip);
  int liblatte_create_principal(long uuid, String image, String config,
      int nport);
  int liblatte_create_principal_with_allocated_ports(long uuid, String image,
      String config, String ip, int port_lo, int port_hi);
  int liblatte_post_object_acl(String obj_id, String requirement);
  int liblatte_delete_principal(long uuid);
  int liblatte_endorse_image(String id, String config, String property) ;
  int liblatte_endorse_membership(String ip, int port, long gn, String master);
  int liblatte_endorse_attester(String id, String config);
  int liblatte_check_worker_access(String ip, int port, String object);
  int liblatte_check_property(String ip, int port, String property);
  int liblatte_check_access(String ip, int port, String object);
  void liblatte_set_log_level(int upto);
}

