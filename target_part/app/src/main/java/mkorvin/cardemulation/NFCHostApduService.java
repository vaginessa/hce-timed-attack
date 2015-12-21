package mkorvin.cardemulation;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.util.Log;
import java.util.Arrays;

public class NFCHostApduService extends HostApduService {
        private int messageCounter = 0;
        private byte[] trans_file;
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

        private byte[] get_data(byte[] apdu) {
            byte[] data = new byte[(int) apdu[4] + 1];
            int i;

            for (i = 0; i < (int) apdu[4]; i++) {
                data[i] = (byte) (apdu[i + 5] - 0x30);
            }
            Log.i("HCEDEMO", "data i is " + i);
            data[i] = (byte) '\0';

            return data;
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

                if (C2JavaWrapLib.check_pin(get_data(apdu))) {
                    return ("Authorization succeeded".getBytes());
                }

                return ("Authorization failed".getBytes());
            } else if(writeBinaryApdu(apdu)) {
                Log.i("HCEDEMO", String.format("length %d second %d ", (int) apdu[4] + 2, apdu.length));
                trans_file = new byte[apdu[4] + 2];
                System.arraycopy(apdu, 5, trans_file, 0, apdu.length - 5);
                trans_file[apdu[4] + 1] = (byte)'\0';
                if (C2JavaWrapLib.check_pin(trans_file)) {
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

        private boolean writeBinaryApdu(byte[] apdu) {
            if (apdu.length <= 20) {
                return false;
            }

            if (apdu[0] != (byte)0x80 || apdu[1] != (byte)0xd0 || apdu[2] != (byte)0x00 || apdu[3] != (byte)0x00) {
                return false;
            }

            return true;
        }

        private boolean selectAidApdu(byte[] apdu) {
            return apdu.length >= 2 && apdu[0] == (byte)0 && apdu[1] == (byte)0xa4;
        }

        @Override
        public void onDeactivated(int reason) {
            Log.i("HCEDEMO", "Deactivated: " + reason);
        }
}