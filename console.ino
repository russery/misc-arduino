#include <stdlib.h>
#include <Arduino.h>
#include "console.h"

#define kLoopDuration_ms 1000


namespace {
void PrintPrompt_(void) {
    Serial.printf("\r\n");
    char temp_buff[32] = {0};
    uint8_t temp_len = Serial.printf(temp_buff, "%08.3f ", millis()/1000.0f);
    Serial.printf(">> ");
}

void PrintInvalidCmd_(char *cmd, int cmd_len) {
    Serial.printf("\r\nInvalid command received: ");
    for (int i = 0; i < cmd_len; i++) {
        Serial.printf("%c", cmd[i]);
    }
}

void HandleCommand_(char cmd[], byte cmd_ind){
    char *p_last_char=0;
    if (strncmp(cmd, "CG", 2) == 0) {
        Serial.printf(" Done\r\n");
    } else if (strncmp(cmd, "CC", 2) == 0) {
        Serial.printf("OKEY DOKEY!!");
    } else {
        PrintInvalidCmd_(cmd, cmd_ind);
    }
}

long GetNextNum_(char *p_start, char* p_end, char** p_num_end, int radix) {
    char* p_num_start = p_start + strcspn(p_start, " .,/") + 1; // Find  whitespace or other delimiters
    if (p_num_start >= p_end) {
        return -1; // Return -1 if no delimiter found
    } else {
        long num = strtol(p_num_start, p_num_end, radix);
        return num;
    }
}

float GetNextFloat_(char *p_start, char* p_end, char** p_num_end) {
    char* p_num_start = p_start + strcspn(p_start, " .,/") + 1; // Find  whitespace or other delimiters
    if (p_num_start >= p_end) {
        return -1; // Return -1 if no delimiter found
    } else {
        float num = strtof(p_num_start, p_num_end);
        return num;
    }
}

} // namespace



Console::CommandType_t Console::CheckForCommand_(char *p_cmd_buff, uint *p_ind, uint max_len) {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') {
            p_cmd_buff[*p_ind] = 0; // Null-terminate sequence, but don't store the CR
            return kNewCommand;
        }
        if ((c == '\b') or (c == 0x7F)) {
            if (*p_ind > 0) { // Handle backspace
                (*p_ind)--;
                Serial.printf("\b \b"); // Erase character and back up
            }
        } else if(c==0x1B) { // Handle escape codes:
            if (Serial.available()){ // Handle arrow keys
                c = Serial.read();
                if(c==0x5B){
                    c = Serial.read(); // Read and discard the last byte
                    return kUseLastCommand;
                }
            }
            // Only got an escape, notify caller
            return kEscCommand;
        }else if ((isspace(c)) and (*p_ind == 0)) {
            ;// Don't capture any leading whitespace
        } else if (*p_ind >= max_len) {
            Serial.printf("Entry too long, length %d greater than max %d\r\n", *p_ind, max_len);
            return kNoCommand; // Truncate read and just send back what we have
        } else {
            // Store the character.
            p_cmd_buff[*p_ind] = toupper(c);
            (*p_ind)++;
        }
        Serial.printf("%c",c); // Echo character back to terminal
    }
    return kNoCommand;
}

Console::Console(uint baud_rate_bps, uint read_timeout_ms, uint cmd_buffer_size) {
    Serial.begin(baud_rate_bps);
    Serial.setTimeout(read_timeout_ms);
    buff_size_ = cmd_buffer_size;
    cmd_ = new char[buff_size_]();
    last_cmd_ = new char[buff_size_]();
}

Console::~Console(void){
    delete cmd_;
    delete last_cmd_;
}

bool Console::IsActive(void){
    return (bool)(Serial);
}

void Console::PrintHelp(void) {
    if (help_callback_){
        help_callback_();
    } else {
        Serial.println("No help defined.");
    }
}

void Console::PrintVersionInfo(void) {
    const char compile_date[] = __DATE__ " " __TIME__;
    Serial.printf("\r\nCompiled on: %s\r\n", compile_date);
}

void Console::PrintStatus(void) {
    if (status_callback_){
        status_callback_();
    } else {
        Serial.printf("\r\nUptime: %lus", (millis() / 1000L));
    }
}

void Console::Loop(void) {
    static uint last_ms = 0;
    static uint cmd_ind, last_cmd_ind = 0;
    CommandType_t cmd_type;

    switch (console_state_) {
        case kHome:
            if (millis()-last_ms > 1000) {
                last_ms = millis();
                this->PrintStatus();
                Serial.printf("--------------\r\nPress '<return>' for Interactive Mode.\r\n\n");
                if (CheckForCommand_(cmd_, &cmd_ind, buff_size_)==kNewCommand) {
                    cmd_ind = 0; // Reset command buffer.
                    console_state_ = kStartInteractive;
                }
            }
            break;
        case kStartInteractive:
            // User input received, print help and start console
            Serial.printf("\r\n\nWelcome to Interactive Mode.\r\n");
            this->PrintVersionInfo();
            PrintHelp();
            PrintPrompt_();
            console_state_ = kInteractiveCmd;
            ; // Intentional fallthrough to kInteractiveCmd:
        case kInteractiveCmd:
            cmd_type = CheckForCommand_(cmd_, &cmd_ind, buff_size_);
            if (cmd_type == kNewCommand) {
                memcpy(last_cmd_, cmd_, cmd_ind); // retain copy of command in case we want to repeat it.
                last_cmd_ind = cmd_ind;
                if ((cmd_ind > 0) && (cmd_ind < buff_size_)) {
                    HandleCommand_(cmd_, cmd_ind);
                } else if (cmd_ind == 0) {
                    ; // Empty command, do nothing... we'll just print a new prompt in a second.
                } else {
                    PrintInvalidCmd_(cmd_, cmd_ind);
                }
                cmd_ind = 0;
                PrintPrompt_();
            } else if(cmd_type == kUseLastCommand) {
                HandleCommand_(last_cmd_, last_cmd_ind);
                PrintPrompt_();
            } else if (cmd_type == kEscCommand) {
                PrintPrompt_();
            }
            break;
        case kStopInteractive:
            console_state_ = kHome;
            break;
    }
}
