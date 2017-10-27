package org.latte.libport;

import com.sun.jna.Library;
import com.sun.jna.Native;



public interface LibPort extends Library
{
  LibPort INSTANCE = (LibPort) Native.loadLibrary("port", LibPort.class);
  public static final int LOG_DEBUG = 7;
  public static final int LOG_INFO = 6;
  public static final int LOG_WARNING = 4;
  int libport_init(String server_url, String persistence_path, int run_as_iaas);
  int libport_reinit(String server_url, String persistence_path, int run_as_iaas);

  int create_principal(long uuid, String image, String config, int nport);
  int create_image(String image_hash, String source_url,
    String source_rev, String misc_conf);
  int post_object_acl(String obj_id, String requirement);
  int endorse_image(String image_hash, String endorsement);
  int endorse_image_new(String image_hash, String endorsement, String config);
  int attest_principal_property(String ip, int port, String prop);
  int attest_principal_access(String ip, int port, String obj);
  int delete_principal(long uuid);

  void libport_set_log_level(int upto);
}

