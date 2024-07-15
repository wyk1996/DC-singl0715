//============================================
#include <string.h>
#include "sysconfig.h"
#include "w5500.h"
#include "socket.h"
#include "DisplayMain.h"
#include "dhcp.h"
#include "4GMain.h"
#include "5500http.h"






INT8U func_analysis_http_download_header(INT8U* buffer, INT16U len_buf, INT8U *resp_code, INT8U *is_stream, INT32U *cont_len)
{
	*resp_code = 0;
	*is_stream = 0;
	*cont_len = 0;
	if(strstr((char*)buffer, "\r\n\r\n") != NULL)//response header ok
	{
		char *p1, *p2;

		p1 = strstr((char*)buffer, "HTTP/1.1 200");
		if(p1 != NULL)// io stream
		{
			*resp_code = 200;
		}

		p1 = strstr((char*)buffer, "Accept-Ranges: bytes");
		if(p1 != NULL)// io stream
		{
			*is_stream = 1;
		}

		p1 = strstr((char*)buffer, "Content-Length: ");
		if(p1 != NULL)
		{
			p2 = strstr(p1, "\r\n");
			if(p2 != NULL)
			{
				char str_len[8] = {0,};
				memcpy(str_len, p1 + strlen("Content-Length: "), p2 - p1 - strlen("Content-Length: "));
				*cont_len = atoi(str_len);
				return 0;
			}
		}
	}
	return 1;
}

void log_net(char *log)
{
	INT8U logsrv_ip[] = {121,42,236,89};
	socket(2, Sn_MR_UDP, 6780, 0x00);
	sendto(2, (INT8U*)log, strlen(log), logsrv_ip, 6780);

}

void func_pack_http_get_download_header(INT8U* buff, INT16U size_buff, INT8U* srv_ip, INT16U srv_port, char* interface)
{
	INT8U tmp[64];
	memset(buff, 0, size_buff);
	//header
	memcpy(buff, "GET ", strlen("GET "));
	strcat((char *)buff, interface);
	strcat((char *)buff, " HTTP/1.1\r\n");
	memset(tmp, 0 , sizeof(tmp));
	sprintf((char*)tmp, "Host: %d.%d.%d.%d:%d\r\n", srv_ip[0], srv_ip[1], srv_ip[2], srv_ip[3], srv_port);
	strcat((char *)buff, (char*)tmp);
	strcat((char *)buff, "Connection: Keep-Alive\r\n");
	strcat((char *)buff, "User-Agent: W5500\r\n");
	strcat((char *)buff, "Accept-Encoding: gzip,deflate\r\n");
	strcat((char *)buff, "\r\n");
}
