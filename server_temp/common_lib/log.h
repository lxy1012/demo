#ifndef _LOG_H_
#define _LOG_H_

#include <time.h>

#define	_LOG_CONSOLE	0x01
#define	_LOG_FILE		0x02
#define	_LOG_FILE_DAILY	0x04
#define	_LOG_SYSLOG		0x08
#define	_LOG_SIZE		0x10

#define _ALL			0
#define _DEBUG			1
#define _NOTE			10
#define _WARN			15
#define _ERROR			20
#define _SPECIAL		30

#define LOG_PROCESS		0x01
#define LOG_THREAD		0x02

#define LOG_DATE		0x01
#define LOG_TIME		0x02
#define LOG_LEVEL		0x04
#define LOG_THREAD_ID	0x08
#define LOG_MODULE		0x10
#define LOG_MESSAGE		0x20
#define LOG_HEX			0x40

typedef struct LogConf
{
	int 	flag;					/* _LOG_CONSOLE, LOG_FILE or LOG_FILE_DAILY(LOG_FILE_DAILY can only be used with LOG_PROCESS) */
	int 	level;					/* _ALL, _DEBUG, _NOTE or _ERROR */
	int		method;					/* LOG_PROCESS or LOG_THREAD, only available when set LOG_FILE or LOG_FILE_DAILY */
	int		format;					/* LOG_DATE|LOG_TIME|LOG_LEVEL|LOG_THREAD_ID|LOG_MODULE|LOG_MESSAGE|LOG_HEX */
	char	log_file_name[200];		/* only available when set LOG_FILE */
	char	syslog_name[200];
	char	modules[500];			/* for example, "CSM,CSMManager" */
	struct 	tm *daily_log_time;		/*only use tm_hour, tm_min and tm_sec to store daily log time */
}LogConfDef;


#ifdef __cplusplus
extern "C"{
#endif

void init_log_conf(LogConfDef conf);
void release_log_conf();
void _log(int level, char *module, char *fmt, ...);
void _note(char *fmt, ...);
void _hex(char *p, int len);
char* _hex2a(char *input, int input_len, char *output);
char* _a2hex(char *input, char *output, int *output_len);
void set_log_level(int level);
char* _hex2str(char *input, int input_len, char *output);
#ifdef __cplusplus
}
#endif

#endif /*_LOG_H_*/
