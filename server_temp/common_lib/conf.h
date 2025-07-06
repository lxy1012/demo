#ifndef _CONF_H_
#define _CONF_H_


#ifdef __cplusplus
extern "C"{
#endif

int GetValueInt(int * value,  char * name, char *filename, char *section, char *defval);
int GetValueShort(short int *value,  char *name, char *filename, char *section, char *defval);
int GetValueFloat(float* value,  char * name, char *filename, char *section, char *defval);
int GetValueStr(char* value,  char * name, char *filename, char *section, char *defval);
int GetValueIPToLong(unsigned long *value, char * name, char *filename, char *section, char *defval);
int GetValueHex(unsigned long *value,  char * name, char *filename, char *section, char *defval);
int GetValueILongLong(long long* value, char* name, char* filename, char* section, char* defval);
#ifdef __cplusplus
}
#endif

#endif /*_CONF_H_*/
