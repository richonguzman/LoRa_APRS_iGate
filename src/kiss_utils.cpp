#include <Arduino.h>
#include "kiss_protocol.h"

bool validateTNC2Frame(const String& tnc2FormattedFrame) {
    return (tnc2FormattedFrame.indexOf(':') != -1) && (tnc2FormattedFrame.indexOf('>') != -1);
}

bool validateKISSFrame(const String& kissFormattedFrame) {
    return kissFormattedFrame.charAt(0) == (char)FEND && kissFormattedFrame.charAt(kissFormattedFrame.length() - 1) == (char)FEND;
}

String encodeAddressAX25(String tnc2Address) {
    bool hasBeenDigipited = tnc2Address.indexOf('*') != -1;

    if (tnc2Address.indexOf('-') == -1) {
        if (hasBeenDigipited) {
            tnc2Address = tnc2Address.substring(0, tnc2Address.length() - 1);
        }

        tnc2Address += "-0";
    }

    int separatorIndex = tnc2Address.indexOf('-');
    int ssid = tnc2Address.substring(separatorIndex + 1).toInt();

    String kissAddress = "";
    for (int i = 0; i < 6; ++i) {
        char addressChar;
        if (tnc2Address.length() > i && i < separatorIndex) {
            addressChar = tnc2Address.charAt(i);
        } else {
            addressChar = ' ';
        }
        kissAddress += (char)(addressChar << 1);
    }

    kissAddress += (char)((ssid << 1) | 0b01100000 | (hasBeenDigipited ? HAS_BEEN_DIGIPITED_MASK : 0));
    return kissAddress;
}

String decodeAddressAX25(const String& ax25Address, bool& isLast, bool isRelay) {
    String address = "";
    for (int i = 0; i < 6; ++i) {
        uint8_t currentCharacter = ax25Address.charAt(i);
        currentCharacter >>= 1;
        if (currentCharacter != ' ') {
            address += (char)currentCharacter;
        }
    }
    auto ssidChar = (uint8_t)ax25Address.charAt(6);
    bool hasBeenDigipited = ssidChar & HAS_BEEN_DIGIPITED_MASK;
    isLast = ssidChar & IS_LAST_ADDRESS_POSITION_MASK;
    ssidChar >>= 1;

    int ssid = 0b1111 & ssidChar;

    if (ssid) {
        address += '-';
        address += ssid;
    }
    if (isRelay && hasBeenDigipited) {
        address += '*';
    }

    return address;
}

String encapsulateKISS(const String& ax25Frame, uint8_t cmd) {
    String kissFrame = "";
    kissFrame += (char)FEND;
    kissFrame += (char)(0x0f & cmd);

    for (int i = 0; i < ax25Frame.length(); ++i) {
        char currentChar = ax25Frame.charAt(i);
        if (currentChar == (char)FEND) {
            kissFrame += (char)FESC;
            kissFrame += (char)TFEND;
        } else if (currentChar == (char)FESC) {
            kissFrame += (char)FESC;
            kissFrame += (char)TFESC;
        } else {
            kissFrame += currentChar;
        }
    }
    kissFrame += (char)FEND; // end of frame
    return kissFrame;
}


String decapsulateKISS(const String& frame) {
    String ax25Frame = "";
    for (int i = 2; i < frame.length() - 1; ++i) {
        char currentChar = frame.charAt(i);
        if (currentChar == (char)FESC) {
            char nextChar = frame.charAt(i + 1);
            if (nextChar == (char)TFEND) {
                ax25Frame += (char)FEND;
            } else if (nextChar == (char)TFESC) {
                ax25Frame += (char)FESC;
            }
            i++;
        } else {
            ax25Frame += currentChar;
        }
    }

    return ax25Frame;
}

String encodeKISS(const String& frame) {
    String ax25Frame = "";

    if (validateTNC2Frame(frame)) {
        String address = "";
        bool dstAddresWritten = false;
        for (int p = 0; p <= frame.indexOf(':'); p++) {
            char currentChar = frame.charAt(p);
            if (currentChar == ':' || currentChar == '>' || currentChar == ',') {
                if (!dstAddresWritten && (currentChar == ',' || currentChar == ':')) {
                    ax25Frame = encodeAddressAX25(address) + ax25Frame;
                    dstAddresWritten = true;
                } else {
                    ax25Frame += encodeAddressAX25(address);
                }
                address = "";
            } else {
                address += currentChar;
            }
        }

        auto lastAddressChar = (uint8_t)ax25Frame.charAt(ax25Frame.length() - 1);
        ax25Frame.setCharAt(ax25Frame.length() - 1, (char)(lastAddressChar | IS_LAST_ADDRESS_POSITION_MASK));
        ax25Frame += (char)APRS_CONTROL_FIELD;
        ax25Frame += (char)APRS_INFORMATION_FIELD;
        ax25Frame += frame.substring(frame.indexOf(':') + 1);
    }

    String kissFrame = encapsulateKISS(ax25Frame, CMD_DATA);
    return kissFrame;
}

String decodeKISS(const String& inputFrame, bool& dataFrame) {
    String frame = "";

    if (validateKISSFrame(inputFrame)) {
        dataFrame = inputFrame.charAt(1) == CMD_DATA;
        if (dataFrame) {
            String ax25Frame = decapsulateKISS(inputFrame);
            bool isLast = false;
            String dstAddr = decodeAddressAX25(ax25Frame.substring(0, 7), isLast, false);
            String srcAddr = decodeAddressAX25(ax25Frame.substring(7, 14), isLast, false);

            frame = srcAddr + ">" + dstAddr;

            int digiInfoIndex = 14;
            while (!isLast && digiInfoIndex + 7 < ax25Frame.length()) {
                String digiAddr = decodeAddressAX25(ax25Frame.substring(digiInfoIndex, digiInfoIndex + 7), isLast, true);
                frame += ',' + digiAddr;
                digiInfoIndex += 7;
            }

            frame += ':';
            frame += ax25Frame.substring(digiInfoIndex + 2);
        } else {
            frame += inputFrame;
        }
    }

    return frame;
}