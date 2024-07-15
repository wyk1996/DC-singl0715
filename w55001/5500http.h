
#ifndef	_5500HTTP_H_
#define	_5500HTTP_H_

#include <stdint.h>
#include <string.h>
#include "bsp.h"


void log_net(char *log);
void func_pack_http_get_download_header(INT8U* buff, INT16U size_buff, INT8U* srv_ip, INT16U srv_port, char* interface);
INT8U func_analysis_http_download_header(INT8U* buffer, INT16U len_buf, INT8U *resp_code, INT8U *is_stream, INT32U *cont_len);

#endif	


