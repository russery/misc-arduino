#ifndef __CONSOLE_H
#define __CONSOLE_H

class Console {
public:
	Console(uint baud_rate_bps=115200, uint read_timeout_ms=1, uint cmd_buffer_size=64, uint cmd_callback_count=0);
	~Console();

	bool IsActive(void);
	void PrintStatus(void);
	void PrintVersionInfo(void);
	void PrintHelp(void);
	void Loop(void);

	typedef void (*HandleCommandFunction_t)(void *cmd_data);
    void RegisterCommand(HandleCommandFunction_t func, const char cmd[], cmd_len);
    typedef void (*PrintFunction_t)(void);
    void RegisterStatusFunction(PrintFunction_t func);
    void RegisterHelpFunction(PrintFunction_t func);

private:
	typedef enum {kHome, kStartInteractive, kInteractiveCmd, kStopInteractive} ConsoleState_t;
	typedef enum {kNoCommand, kNewCommand, kUseLastCommand, kEscCommand} CommandType_t;
	typedef struct {HandleCommandFunction_t *func; const char* cmd} CommandCallback_t;
	ConsoleState_t console_state_ = kHome;
	char *cmd_;
    char *last_cmd_;
    uint buff_size_;
    CommandCallback_t cmd_callback_list_[];
    PrintFunction_t status_callback_ = NULL;
    PrintFunction_t help_callback_ = NULL;

    CommandType_t CheckForCommand_(char *p_cmd_buff, uint *p_ind, uint max_len);
};


#endif //__CONSOLE_H