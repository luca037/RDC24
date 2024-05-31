const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64(unsigned char* buf, unsigned char* input, int rest) {
    unsigned char first_6;
    first_6 = (input[0] & (0xfc));
    first_6 = first_6 >> 2;

    unsigned char second_6;
    second_6 = (input[1] & 0xf0) >> 4;
    second_6 = second_6 | ((input[0] & 0x03) << 4);

    if (rest == 1) {
        buf[0] = base64_alphabet[first_6];
        buf[1] = base64_alphabet[second_6];
        return;
    }

    unsigned char third_6;
    third_6 = input[2] >> 6;
    third_6 = third_6 | ((input[1] & 0x0f) << 2);

    if (rest == 2) {
        buf[0] = base64_alphabet[first_6];
        buf[1] = base64_alphabet[second_6];
        buf[2] = base64_alphabet[third_6];
        return;
    }

    unsigned char fourth_6;
    fourth_6 = input[2] & 0x3f;

    buf[0] = base64_alphabet[first_6];
    buf[1] = base64_alphabet[second_6];
    buf[2] = base64_alphabet[third_6];
    buf[3] = base64_alphabet[fourth_6];
};
