#include "sun_lib.h"

using namespace std;

int verify_subscribe(char* buffer) {
    char* helper;
    strtok(buffer, " "); // subscribe
    if (strtok(NULL, " ") != NULL) { // topic
        helper = strtok(NULL, "\n"); // SF
        if (helper != NULL) {
            if (strcmp(helper,"0") == 0 || strcmp(helper,"1") == 0) 
                return 1;
        }
    }
    return 0;
}

int verify_unsubscribe(char* buffer) {
    char* helper;
    strtok(buffer, " "); // unsubscribe
    helper = strtok(NULL, " ");
    if (helper != NULL && strncmp(helper, "\n", 1) != 0) { // topic
        if (strtok(NULL, " ") == NULL) // no more fields after the topic is typed
            return 1;
    }
    return 0;
}

uint32_t parse_int_udp_message(char* char_value, unsigned int sign) {
    uint32_t value = ((char_value[0] << 24) & 0xff000000)
                    | ((char_value[1] << 16) & 0x00ff0000)
                    | ((char_value[2] << 8) & 0x0000ff00)
                    | ((char_value[3] << 0) & 0x000000ff);

    if (sign == 1 && value != 0)
        value = -value;

    return value;
}

char* parse_short_udp_message(char* char_value) {
    uint16_t initial_value = ((char_value[0] << 8) & 0xff00)
                    | ((char_value[1] & 0x00ff));
    int real = initial_value / 100;
    int fractional = initial_value % 100;
    char* value = (char*) malloc(SAMPLE_SIZE);

    if (fractional != 0) {
        if (fractional < 10) {
            sprintf(value, "%d.0%d", real, fractional);
        } else {
            sprintf(value, "%d.%d", real, fractional);
        }
    } else {
        sprintf(value, "%d", real);
    }

    return value;
}

char* parse_float_udp_message(char* char_value, unsigned int sign, unsigned int floating_points) {
    uint32_t number_value = ((char_value[0] << 24) & 0xff000000)
                    | ((char_value[1] << 16) & 0x00ff0000)
                    | ((char_value[2] << 8) & 0x0000ff00)
                    | ((char_value[3] << 0) & 0x000000ff);

    char* return_text = (char*) malloc(SAMPLE_SIZE);
    char* helper = (char*) malloc(SAMPLE_SIZE);
    if (sign == 1) {
        sprintf(return_text, "-");
    } else {
        sprintf(return_text, "");
    }

    char* whole_number = (char*) malloc(SAMPLE_SIZE);
    sprintf(whole_number, "%d", number_value); // the whole number;
    int real_len = strlen(whole_number) - floating_points;

    if (real_len <= 0) { // this is a number < 1 with >= 1 zeros at the beginning
        sprintf(helper, "0.");
        strcat(return_text, helper);
        int floating_points_zeros = floating_points - strlen(whole_number);
        for (int i = 0; i < floating_points_zeros; i++) {
            sprintf(helper, "0");
            strcat(return_text, helper);
        }

        strncpy(helper, whole_number, strlen(whole_number));
        strcat(return_text, helper);
        return return_text;
    } else {
        strncpy(helper, whole_number, real_len); // copy the real part in helper
        strcat(return_text, helper);

        if (floating_points != 0) {
            strcat(return_text, ".");
            strncpy(helper, whole_number + real_len, floating_points);
            strcat(return_text, helper);
        } else return return_text;
    }
    free(helper);
    free(whole_number);
    return return_text;
}