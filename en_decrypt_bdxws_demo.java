import java.io.*;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Base64;
import java.util.Scanner;
import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

public class en_decrypt_bdxws_demo {
  public static String toMD5(String original_passwd) {
    System.out.println("CalculatePwd [MD5]");
    String final_md5_str = null;
    try {
      byte[] tmp_bytes = original_passwd.getBytes();
      MessageDigest md_tmp = MessageDigest.getInstance("md5");
      md_tmp.reset();
      md_tmp.update(tmp_bytes);
      byte[] md5_bytes = md_tmp.digest();
      StringBuilder md5_str = new StringBuilder();
      for (int index_tmp = 0; index_tmp < md5_bytes.length; ++index_tmp) {
        String tmp_str = Integer.toHexString(md5_bytes[index_tmp] & 0xFF);
        if (tmp_str.length() == 1) {
          tmp_str = "0" + tmp_str;
        }
        md5_str.append(tmp_str);
      }

      final_md5_str = md5_str.toString();
    } catch (NoSuchAlgorithmException exp) {
      exp.printStackTrace();
    }
    return final_md5_str;
  }
  public static Cipher genCliper(int mode, String keya, String keyb) {
    Cipher var1;
    try {
      var1 = Cipher.getInstance("AES/CBC/PKCS5Padding");
    } catch (NoSuchAlgorithmException exp) {
      var1 = null;
    } catch (NoSuchPaddingException exp) {
      var1 = null;
    }

    try {
      var1.init(mode, new SecretKeySpec(keya.getBytes(), "AES"),
                new IvParameterSpec(keyb.getBytes()));
      return var1;
    } catch (InvalidAlgorithmParameterException exp) {
      var1 = null;
    } catch (InvalidKeyException exp) {
      var1 = null;
    }

    return var1;
  }
  public static void main(String[] args) throws BadPaddingException,
                                                IllegalBlockSizeException,
                                                UnsupportedEncodingException {
    Scanner sc = new Scanner(System.in);
    System.out.print("Plz enter Key:> ");
    String kkey = sc.nextLine();
    String key = toMD5(kkey).toUpperCase();
    String aes_key = key.substring(0, 16);
    String aes_iv = key.substring(16);
    System.out.print("Key A> ");
    System.out.println(aes_key);
    System.out.print("Key B> ");
    System.out.println(aes_iv);
    System.out.println("ModeCode:1 for encrypt,2 for decrypt");
    System.out.print("ModeCode:> ");
    int mode = sc.nextInt();

    if (mode == 1) {
      System.out.print("EncryptText :> ");
      String Original = sc.next();

      Cipher clipher = genCliper(1, aes_key, aes_iv);
      byte[] encrypted =  clipher.doFinal(Original.getBytes("UTF-8"));

      String base64en = Base64.getEncoder().encodeToString(encrypted);
      System.out.println("Encryped Text>");
      System.out.println(base64en);
    } else {
      if (mode == 2) {
        System.out.print("EncryptText :> ");
        String Original = sc.next();

        Cipher clipher = genCliper(2, aes_key, aes_iv);
        String decrypted =
            new String(clipher.doFinal(Base64.getDecoder().decode(Original)));
        System.out.println("Decryped Text>");
        System.out.println(decrypted);
      } else {
        System.out.println("No Such Mode!!!");
      }
    }
  }
}