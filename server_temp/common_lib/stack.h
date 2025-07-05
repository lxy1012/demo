#ifndef _STACK_H_
#define _STACK_H_

#include "thread.h"
#include "emutex.h"
#include <netinet/in.h>

typedef enum SocketType
{
	SOCKET_UNKOWN	 = 0,
	TCP_SERVER_SOCKET,
	TCP_ACCEPT_SOCKET,
	TCP_CLIENT_SOCKET,
	UDP_COMMON_SOCKET
}SocketTypeDef;

typedef struct NodeStateMap
{
	int		check_state;
	int		check_oper;
	int		next_state;
}NodeStateMapDef;

typedef struct SocketNode
{
	SocketTypeDef	socket_type;
	int				heart_beat_time;
	int				iServerPort;//如果为TCP_SERVER_SOCKET的话，记录PORT

	int				socket;
	time_t 			time_out_time;
	int				read_msg_head_len;
	int				socket_node_state;

	char			*read_buffer;						
	int 			read_msg_len;
	int 			read_msg_pos;
	int 			total_read_bytes;
	
	char			*write_buffer;
	int				write_msg_len;
	int				total_write_bytes;
	
	int				iWaitOutTime;
	
	bool			bKill;
	
	int				iReserve1;//保留字段1，暂时作为ROOMID
	int				iReserve2;//保留字段2，暂时作为UserID
	int				iNoAesFlag;//是否不需要aes加密解密
}SocketNodeDef;

class Stack : public Thread
{
public:
						Stack(int node_num, int read_size, int write_size);
						~Stack();
	int 				AddTCPServerNode(int server_port, int msghead_len, int heartbeat_time = 0);
	int 				AddTCPClientNode(char *server_ip, int server_port, int msghead_len, int heartbeat_time = 0, int client_port = 0);
	int 				AddUDPCommonNode(char *server_ip, int server_port);
	void 				DelSocketNode(int index, bool callback_flag = false);
	int					WriteSocketNodeData(int index, void *data, int data_size, int if_important = 1);
	int					WriteAllSocketNode(void *data, int data_size,int iRoomID,int iUserID);
	
	virtual void				SetKillFlag(int iIndex,bool bFlag = true);//crystal add
	
	void				SetThreadRunTestLog();
	bool				m_bTestRunLog;
	
protected:
	virtual void 		CallbackAddSocketNode(int index);
	virtual void 		CallbackDelSocketNode(int index);
	virtual void 		CallbackNodeTimeOut(int index);
	virtual void 		CallbackTCPReadData(int index);
	virtual void 		CallbackUDPReadData(int index, unsigned long remote_ip, int remote_port);
	virtual void		CallbackWriteAllNode();
	void 				ResetSocketNode(int index);
	int 				TCPWriteData(int socket, void *data, const int data_size);
	int 				UDPWriteData(int socket, char *data, int data_len, unsigned long remote_ip, int remote_port);
	int					CheckSocketNodeState(int index, int curr_oper);
		
	SocketNodeDef 		*socket_node;
	NodeStateMapDef		*node_state_map;
	
	int					max_node_index;
private:
	int 				Run();
	int  				AddSocketNode(int socket, SocketTypeDef type, int msghead_len, int heartbeat_time = 0, bool callback_flag = false,int iPort = 0);
	
	int 				TCPReadData(int socket, void *data, int data_size);
	int 				UDPReadData(int socket, char *data, int data_len, unsigned long *remote_ip, int *remote_port);
	
	int					socket_node_num;
	
	int					all_node_num;
	
	int					read_buffer_size;
	int					write_buffer_size;
	EMutex				write_buffer_mutex;
};

#endif

