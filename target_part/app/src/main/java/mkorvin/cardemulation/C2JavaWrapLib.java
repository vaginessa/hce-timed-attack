package mkorvin.cardemulation;

/**
 * Created by mkorvin on 31.10.15.
 */
public class C2JavaWrapLib {
    public static native boolean check_pin(byte[] input);

    static
    {
        System.loadLibrary("security");
    }
}
