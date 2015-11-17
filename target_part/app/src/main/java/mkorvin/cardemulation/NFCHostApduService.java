package mkorvin.cardemulation;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.util.Log;
import java.util.Arrays;

public class NFCHostApduService extends HostApduService {
        private int messageCounter = 0;
        private static final byte[] PIN_Request = new byte[] { (byte)0x00, (byte)0x20,(byte)0x00,
                                                               (byte)0x01, (byte)0x08 };

        private boolean compare_apdu_by_len(byte[] a, byte[] b, int length) {
            for (int i = 0; i < length; i++) {
                if (a[i] != b[i]) {
                    return false;
                }
            }

            return true;
        }

        private byte[] getPIN(byte[] apdu) {
            byte[] PIN = new byte[4];

            PIN[0] = (byte) (apdu[6] - 0x30);
            PIN[1] = (byte) (apdu[7] - 0x30);
            PIN[2] = (byte) (apdu[8] - 0x30);
            PIN[3] = (byte) (apdu[9] - 0x30);

            return PIN;
        }

        private String convert_apdu2string(byte[] apdu) {
            final StringBuilder builder = new StringBuilder();
            for(byte b : apdu) {
                builder.append(String.format("0x%02x ", b));
            }

            return builder.toString();
        }

        @Override
        public byte[] processCommandApdu(byte[] apdu, Bundle extras) {
            if (selectAidApdu(apdu)) {
                Log.i("HCEDEMO", "Application selected");
                return getWelcomeMessage();
            } else if (compare_apdu_by_len(PIN_Request,
                                           apdu,
                                           PIN_Request.length)) {
                Log.i("HCEDEMO", "PIN check request. Received: " + convert_apdu2string(apdu));
//                C2JavaWrapLib native_wrapper = new C2JavaWrapLib();
                Log.i("HCEDEMO", "After");
                if (C2JavaWrapLib.check_pin(getPIN(apdu))) {
                    return ("Authorization succeeded".getBytes());
                }

                return ("Authorization failed".getBytes());
            } else {
                Log.i("HCEDEMO", "Received: " + convert_apdu2string(apdu));

                return getNextMessage();
            }
        }

        private byte[] getWelcomeMessage() {
            return "Hello Desktop!".getBytes();
        }

        private byte[] getNextMessage() {
            return ("Message from android: " + messageCounter++).getBytes();
        }

        private boolean selectAidApdu(byte[] apdu) {
            return apdu.length >= 2 && apdu[0] == (byte)0 && apdu[1] == (byte)0xa4;
        }

        @Override
        public void onDeactivated(int reason) {
            Log.i("HCEDEMO", "Deactivated: " + reason);
        }
}