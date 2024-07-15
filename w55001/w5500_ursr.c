/*****************************************Copyright(C)*************************************
*******************************************汇誉科技****************************************
*------------------------------------------文件信息---------------------------------
* FileName			: GPRSRecv.c
* Author			:
* Date First Issued	:
* Version			:
* Description		:
*----------------------------------------历史版本信息-------------------------------------------
* History			:
* //2010		    : V
* Description		:
*-----------------------------------------------------------------------------------------------
**********************************************************************************************/
/* Includes----------------------------------------------------------------------------*/
//#include "W5500Main.h"
#include <string.h>
#include "sysconfig.h"
#include "w5500.h"
#include "socket.h"
#include "DisplayMain.h"
#include "dhcp.h"
#include "4GMain.h"
#include "ftpc.h"
#include "5500http.h"


uint8_t ip_dhcp_get_flag = 0;
/* Private define-----------------------------------------------------------------------------*/
#define ETHMAIN_Q_LEN  15
static void *ETHMAINOSQ[ETHMAIN_Q_LEN];					// 消息队列
static OS_EVENT *ETHMainTaskEvent;				            // 使用的事件

//网络参数
wiz_NetInfo gWIZNETINFO =
{
	0x00,0x66,0x8F,0x5A,0x76,0xCD, 	//MAC地址
	192,168,2,120,                  //开发板IP，如果使用DHCP无需关心
	255,255,255,0,                  //子网掩码  如果使用DHCP无需关心
	192,168,2,1,                    //网关，    如果使用DHCP无需关心
	192,168,2,1,                    //DNS       如果使用DHCP无需关心
	NETINFO_STATIC 					//NETINFO_DHCP = 使用DHCP    NETINFO_STATIC= 使用静态IP
};

INT8U ETHStatus = SOCK_CLOSED;
#ifndef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#endif //MIN
/* Private function prototypes----------------------------------------------------------------*/
/* Private functions--------------------------------------------------------------------------*/
/*****************************************************************************
* Function     : APP_GetETHLinKState
* Description  : 获取以太网连接状态
* Input        : void
* Output       : TRUE    连接成功
				 FALSE   连接失败
* Return       :
* Note(s)      :
* Contributor  : 2018年6月16日
*****************************************************************************/
INT8U APP_GetETHLinKState(void)
{
	if(ETHStatus == SOCK_ESTABLISHED)
	{
		return TRUE;
	}
	return FALSE;
}

/**
*@brief		W5500复位设置函数
*@param		无
*@return	无
*/
void reset_w5500(void)
{
//	GPIO_SetBits(GPIOE, GPIO_Pin_10);
//	OSTimeDly(SYS_DELAY_1s);
//	GPIO_ResetBits(GPIOE, GPIO_Pin_10);
//	OSTimeDly(SYS_DELAY_1ms);
//	GPIO_SetBits(GPIOE, GPIO_Pin_10);
//	//OSTimeDly(SYS_DELAY_5ms);

	//		GPIO_SetBits(GPIOC, GPIO_Pin_5);
//		OSTimeDly(SYS_DELAY_1s);
//		GPIO_ResetBits(GPIOC, GPIO_Pin_5);
//		OSTimeDly(SYS_DELAY_1ms);
//		GPIO_SetBits(GPIOC, GPIO_Pin_5);


	GPIO_SetBits(GPIOD, GPIO_Pin_4);
	OSTimeDly(SYS_DELAY_1s);
	GPIO_ResetBits(GPIOD, GPIO_Pin_4);
	OSTimeDly(SYS_DELAY_1ms);
	GPIO_SetBits(GPIOD, GPIO_Pin_4);
	//OSTimeDly(SYS_DELAY_5ms);

}

//#define W5500_CS_GPIO			GPIOB
//#define W5500_CS_PIN			GPIO_Pin_12
///* 片选口线置低选中  */
//#define SPI_W5500_CS_LOW	(GPIOB->BSRRH = W5500_CS_PIN)
///* 片选口线置高不选中 */
//#define SPI_W5500_CS_HIGH	(GPIOB->BSRRL = W5500_CS_PIN)
//#define SPI_W5500_SPI		SPI2


/**
*@brief		配置W5500的GPIO接口
*@param		无
*@return	无
*/
void gpio_for_w5500_config(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;

	bsp_InitSPI1();		//引脚初始化
	bsp_CfgSPI2Init();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);      //PD4=NET_RST    PD3=NET_INT
	/*¶¨ÒåRESETÒý½Å*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;						 /*Ñ¡ÔñÒª¿ØÖÆµÄGPIOÒý½Å*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 /*ÉèÖÃÒý½ÅËÙÂÊÎª50MHz*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);							 /*µ÷ÓÃ¿âº¯Êý£¬³õÊ¼»¯GPIO*/
	GPIO_SetBits(GPIOD, GPIO_Pin_4);


	/*¶¨ÒåINTÒý½Å*/
	//µÍµçÆ½£ºW5500µÄÖÐ¶ÏÉúÐ§
	//¸ßµçÆ½£ºÎÞÖÐ¶Ï
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;						 /*Ñ¡ÔñÒª¿ØÖÆµÄGPIOÒý½Å*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 /*ÉèÖÃÒý½ÅËÙÂÊÎª50MHz */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;				 /*ÉèÖÃÒý½ÅÄ£Ê½ÎªÍ¨ÓÃÍÆÍìÄ£ÄâÉÏÀ­ÊäÈë*/
	GPIO_Init(GPIOD, &GPIO_InitStructure);

}



//专门做GPRS接收处理
/*****************************************************************************
* Function     : HardReset_W5500
* Description  : 硬件复位w5500
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月16日
*****************************************************************************/
void HardReset_W5500(void)
{
	gpio_for_w5500_config();			/*初始化MCU相关引脚*/
	reset_w5500();						/*硬复位W5500*/
	//set_w5500_mac();					/*配置MAC地址*/
	//set_w5500_ip();					/*配置IP地址*/
	//socket_buf_init(txsize, rxsize);	/*初始化8个Socket的发送接收缓存大小*/
}





typedef struct
{
	uint8_t WorkMode;
	uint8_t  ServerIp1_Addr[4];
	uint32_t PortNum1;
} _SYS_DATA;

_SYS_DATA SYSData;



//extern _SYS_DATA SYSData;
//注册TCP通信相关的回调函数
//进入临界区
void user_app_w5500_enter_cris(void)
{
	__set_PRIMASK(1);
}
//退出临界区
void user_app_w5500_exit_cris(void)
{
	__set_PRIMASK(0);
}
//SPI片选
void user_app_w5500_cs_select(void)
{
	GPIO_ResetBits(W5500_CS_GPIO,W5500_CS_PIN);
}
//SPI取消片选
void user_app_w5500_cs_deselect(void)
{
	GPIO_SetBits(W5500_CS_GPIO,W5500_CS_PIN);
}

uint8_t user_dev_spi4_read_write_byte(uint8_t TxData)
{
	uint8_t status;
	//return (SPI2_SendRcvByte(TxData,&status));
	return (SPI1_SendRcvByte(TxData,&status));
	//return 0;
}

void user_app_w5500_callback_func_register(void)
{
	reg_wizchip_cris_cbfunc(user_app_w5500_enter_cris, user_app_w5500_exit_cris);    //注册临界区回调函数
	reg_wizchip_cs_cbfunc(user_app_w5500_cs_select, user_app_w5500_cs_deselect); //注册SPI片选信号函数
	reg_wizchip_spi_cbfunc(user_dev_spi4_read_write_byte, (void*)user_dev_spi4_read_write_byte);    //注册SPI读写函数
}
//初始化芯片参数
void user_app_w5500_chip_para_config(void)
{
	uint8_t tmp;
	uint8_t memsize[4][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
	static uint32_t pth_Link_check_cnt = 0;
	printf("Device_is_initializing\r\n");
	if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)//初始化chip   //初始化收发缓冲区，失败重启
	{
		//print_debug("	w5500:初始化缓存区失败!!!\r\n");
		printf("初始化缓存区失败,准备重启\r\n");
		NVIC_SystemReset();
		return;
	}
	//delay_ms(4000);
	printf("W5500_Init_successfully\r\n");
	do
	{
		pth_Link_check_cnt++;//防止卡死在这
		if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
		{
			printf("w5500:硬件链路故障! *********************************\r\n");
		}
		if(tmp == PHY_LINK_OFF)
			if(pth_Link_check_cnt >= 500)
				//printf("	w5500:网线未连接\r\n");
		OSTimeDly(SYS_DELAY_5ms);
	}
	while((tmp == PHY_LINK_OFF)&&(pth_Link_check_cnt < 1000)); //100000
}


#define SOCK_DHCP			0     					 	//开启DHCP的端口（W5500端口0-7共计8个）  
#define SOCK_http			4   //单独的http端口

//uint8_t gDATABUF[1024];
uint8_t	gDATABUF[1024];

wiz_NetInfo netinfo;
void network_init(void)
{
	uint8_t tmpstr[6] = {0};


	//设置网络参数
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);

	//读取网络参数
	ctlnetwork(CN_GET_NETINFO, (void*)&netinfo);

	//读取芯片ID
	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	//打印网络参数
	if(netinfo.dhcp == NETINFO_DHCP)
		printf("\r\n=== %s NET CONF : DHCP ===\r\n",(char*)tmpstr);
	else
		printf("\r\n=== %s NET CONF : Static ===\r\n",(char*)tmpstr);

	printf("===========================\r\n");
	printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],netinfo.mac[3],netinfo.mac[4],netinfo.mac[5]);
	printf("本地IP: %d.%d.%d.%d\r\n", netinfo.ip[0],netinfo.ip[1],netinfo.ip[2],netinfo.ip[3]);
	printf("网关: %d.%d.%d.%d\r\n", netinfo.gw[0],netinfo.gw[1],netinfo.gw[2],netinfo.gw[3]);
	printf("子网掩码: %d.%d.%d.%d\r\n", netinfo.sn[0],netinfo.sn[1],netinfo.sn[2],netinfo.sn[3]);
	printf("DNS: %d.%d.%d.%d\r\n", netinfo.dns[0],netinfo.dns[1],netinfo.dns[2],netinfo.dns[3]);
	printf("===========================\r\n");
}

//获取IP的回调函数
void my_ip_assign(void)
{
	getIPfromDHCP(gWIZNETINFO.ip);
	getGWfromDHCP(gWIZNETINFO.gw);
	getSNfromDHCP(gWIZNETINFO.sn);
	getDNSfromDHCP(gWIZNETINFO.dns);
	//gWIZNETINFO.dhcp = NETINFO_DHCP;
	gWIZNETINFO.dhcp = NETINFO_STATIC;  //静态IP
	network_init();
	printf("DHCP LEASED TIME : %d Sec.\r\n", getDHCPLeasetime());
	ip_dhcp_get_flag = 1;
}

//获取IP的失败函数
void my_ip_conflict(void)
{
	printf("获取IP失败，准备重启\r\n");
	NVIC_SystemReset();
}

/*读取芯片ID  96位的出厂编号*/
void Get_CPU_ID(uint8_t *buf)
{
	uint8_t i = 0;
	for(i = 0; i<12; i++)
	{
		buf[i] = *(__IO uint8_t *)(0x1fff7a10+i);
	}
}

void user_app_w5500_init_DHCP(void)
{
	uint8_t dhcp_cpu_id[12] = {0};
	uint8_t mac[6];

	reset_w5500();												/*硬复位W5500*/
	user_app_w5500_callback_func_register();	//注册TCP通信相关的回调函数
	user_app_w5500_chip_para_config();		//初始化芯片参数

	Get_CPU_ID(dhcp_cpu_id);/*获取cpu id后五个字节 作为唯一的mac后五字节*/
	gWIZNETINFO.mac[0] = 0x0c;
	memcpy(&gWIZNETINFO.mac[1],&dhcp_cpu_id[0],5);  //mac获取

	setSHAR(gWIZNETINFO.mac);         //设置MAC地址
	DHCP_init(SOCK_DHCP, gDATABUF);   //初始化DHCP

	OSTimeDly(SYS_DELAY_3s);  //为什么要延迟后才可以连上网？？？

	reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);//注册回调函数
	printf("W5500-chushihua\r\n"); //初始化完成
}

uint8_t databuffer[4096];
uint8_t DHCP_STATE = 0;
INT8U NetState;
extern OS_EVENT *GPRSRECVTaskEvent;
/*****************************************************************************
* Function     : TaskW5500Connect
* Description  : 以太网连接
* Input        : void
* Output       : None
* Return       :
* Note(s)      :接收数据轮询在此任务中进行，此任务一个循环时间不能太长
* Contributor  : 2020年12月14日
*****************************************************************************/
void TaskW5500Connect(void *pdata)
{
	static uint8_t FTPFist = 0;
	INT16U count = 0;
	uint8_t socketStatus = 0;
	static uint8_t oldSocketStatus = 0;
	static uint8_t oldLinkStatus = 0;
	INT8U rst ;
	INT8U err;
	_BSP_MESSAGE *pMsg;
	NetState = W5500_WORK_STEP_FAULT;
	uint8_t linkStatus = 0;
	uint16_t	revLenth = 0;
	pdata =  pdata;
	ETHMainTaskEvent = OSQCreate(ETHMAINOSQ, ETHMAIN_Q_LEN);//
	if (ETHMainTaskEvent == NULL)
	{
		OSTaskSuspend(OS_PRIO_SELF);    //挂起当前任务
		return;
	}
	_BSP_MESSAGE send_message;
	gpio_for_w5500_config();		/*初始化MCU相关引脚*/
	HardReset_W5500();			  //硬件复位w5500
	user_app_w5500_init_DHCP();  //初始化一下

	uint8_t  remote_ip[4]= {116,62,125,35};
	uint16_t remote_port=8000;
	uint16_t local_port=8000;

	//网关、IP地址赋值
	memcpy(gWIZNETINFO.ip,SYSSet.DHCPip,sizeof(SYSSet.DHCPip));  //自动获取的IP地址
	memcpy(gWIZNETINFO.sn,SYSSet.DHCPZWIP,sizeof(SYSSet.DHCPZWIP));  //子网：255.255.255
	memcpy(gWIZNETINFO.gw,SYSSet.gateway,sizeof(SYSSet.gateway));  //网关
	memcpy(gWIZNETINFO.dns,gWIZNETINFO.gw,sizeof(gWIZNETINFO.gw));  //DNS和网关相同

	while(1)
	{

		if(SYSSet.Net4GorEternet == NET_Ethernet)   //以太网时才会运行
		{
//			uint8_t status = SOCK_CLOSED;
//			switch(getSn_SR(1))								  				         /*»ñÈ¡socketµÄ×´Ì¬*/
//			{
//				case SOCK_CLOSED:											        		         /*socket´¦ÓÚ¹Ø±Õ×´Ì¬*/
//				  printf("123456\r\n");
//					socket(1,Sn_MR_TCP,local_port,Sn_MR_ND);
//					status = SOCK_CLOSED;
//					break;

//				case SOCK_INIT:													        	         /*socket´¦ÓÚ³õÊ¼»¯×´Ì¬*/
//				  printf("987654\r\n");
//					connect(1,remote_ip,remote_port);                /*socketÁ¬½Ó·þÎñÆ÷*/
//					status = SOCK_INIT;
//					break;
//			}


			//====读取寄存器的值：确定是不是物理连接正常与否？？？
			linkStatus = (IINCHIP_READ(PHYCFGR)&0x01);
			if(oldLinkStatus != linkStatus)
			{
				if(linkStatus == 0)  //如果等于0时，就是突然断开连接
				{
					APP_SetModuleConnectState(SOCK_DHCP,STATE_4G_ERR);   //断开连接
					ip_dhcp_get_flag = 0;   //自动获取的标志位
					NetState = W5500_WORK_STEP_FAULT;

					APP_SetNetNotConect(0); //全部断开连接
				}

//				if(linkStatus == 1)
//				{
//					user_app_w5500_init_DHCP(); //初始化一下
//				}
				oldLinkStatus = linkStatus;
				printf("w5500:PHY-state:%x \r\n",oldLinkStatus);
			}


			//====一直读取当前状态连接是否正常？？？
			//===如果没有网络就是socketStatus = 0
			socketStatus = getSn_SR(0);

			if(oldSocketStatus != socketStatus)
			{
				oldSocketStatus = socketStatus;
				printf("w5500:socket0-state:%x \r\n",oldSocketStatus);
			}

			//在正常的收发中，一直判断是不是正常的收发状态，如果不是，立即初始化
			if((NetState == W5500_WORK_STEP_WORKING) && (APP_GetModuleConnectState(0) != STATE_OK))
			{
				if(socketStatus != SOCK_ESTABLISHED)  //SOCK_ESTABLISHED   0x17
				{
					printf("w5500:SOCKwork-kai:%x \r\n",socketStatus);
					ip_dhcp_get_flag = 1;  // 这个是下面的标志位
					NetState = W5500_WORK_STEP_FAULT;
					APP_SetNetNotConect(0); //全部断开连接
				}
			}



			//自动获取DHCP----IP地址或
			if(((ip_dhcp_get_flag == 0)||(oldSocketStatus == SOCK_UDP))&&(linkStatus == 1))
			{
//				printf("zidonghuqu:%d\r\n",oldSocketStatus);
//				DHCP_run();  //自动获取IP    分为静态 NETINFO_STATIC = 1,    动态（自动获取） NETINFO_DHCP=2

				network_init();	   // 这个是固定IP
				ip_dhcp_get_flag = 1;  // 这个是下面的标志位
			}


			if(((ip_dhcp_get_flag)||(oldSocketStatus == SOCK_UDP)) &&(linkStatus == 1))
			{
				switch(NetState)
				{
					case W5500_WORK_STEP_INTI:
						if(APP_GetSIM7600Mode() == MODE_HTTP)
						{
							rst = socket(SOCK_http,	Sn_MR_TCP,9000,0x00);  //创建套字节编号0-7  共8个
						}
						else
						{
							rst = socket(SOCK_DHCP,	Sn_MR_TCP,9000,0x00);  //创建套字节编号0-7  共8个
						}

						if((rst == SOCK_DHCP) || (rst == SOCK_http))
						{
							printf("w5500:Socket%d 创建成功!!! 	\r\n",SOCK_DHCP);
							DHCP_STATE = 1;
							NetState = W5500_WORK_STEP_CONNECT;
							APP_SetSIM7600Status(STATE_OK);    //设置模块存在 ====(可以是4g模块  也可以是以太网模块)
						}
						break;


					case W5500_WORK_STEP_CONNECT:
						if(APP_GetSIM7600Mode() == MODE_DATA)
						{
							//ModuleSIM7600_ConnectServer(count,(INT8U*)NetConfigInfo[SYSSet.NetYXSelct].pIp,NetConfigInfo[SYSSet.NetYXSelct].port);
							static uint16_t SOCKNUM = 0;
							if(connect(SOCK_DHCP,(INT8U*)SYSSet.IP,SYSSet.Port) == SOCK_OK)
							{
								NetState = W5500_WORK_STEP_WORKING;
								printf("w5500111:服务器连接成功!\r\n");
								APP_SetModuleConnectState(SOCK_DHCP,STATE_OK);   //已经连接上了第几个平台的服务器
								SOCKNUM = 0;
							}
							if(++SOCKNUM > 500)
							{
								NetState = W5500_WORK_STEP_FAULT;   //强制一下复位
								SOCKNUM = 0;
								printf("w5500111:456789!\r\n");
							}
						}
						break;

					case W5500_WORK_STEP_WORKING:
						if(APP_GetSIM7600Mode() == MODE_DATA)
						{
							revLenth = recv(SOCK_DHCP,gDATABUF,1024);
							if(revLenth > 0)//接收TCP服务器发送的数据
							{
								//send_message.MsgID = (_BSP_MSGID)BSP_MSGID_UART_RXOVER;
								send_message.MsgID = (_BSP_MSGID)BSP_MSGID_Ethernet_RXOVER;
								send_message.DataLen = revLenth;
								send_message.pData = gDATABUF;
//								uint16_t len = 0;
//								printf("\r\ninter4g---Recv:");
//								for(len = 0; len < revLenth; len++)
//								{
//									printf("%02x",gDATABUF[len]);
//								}

								static uint8_t SNUM = 1;
								if(SNUM)
								{
									OSQPost(GPRSRECVTaskEvent, &send_message);  //发送到4g接收---直接是数据
								}
							}
						}
						break;

					case W5500_WORK_STEP_FAULT:

						printf("w5500--qiangzhi\r\n");   //强制进行硬件重启并初始化!
						HardReset_W5500();			//必须硬件复位w5500
						OSTimeDly(SYS_DELAY_2s);    //硬件复位后，尽量延迟，快速联网

						user_app_w5500_init_DHCP();
						//OSTimeDly(SYS_DELAY_100ms);    //硬件复位后，尽量延迟
						ip_dhcp_get_flag = 0;
						NetState = W5500_WORK_STEP_INTI;
						break;

					default:
						break;
				}
			}
			else
			{
				NetState = W5500_WORK_STEP_FAULT; //直接让复位，然后再重联
			}

			if(++count > 180)
			{
				count = 0;
				DHCP_time_handler();
			}


            //是否远程升级??
			INT8U httpbuf[2048];
			if(APP_GetSIM7600Mode() == MODE_FTP)  //FTP升级
			{
				if(FTPFist == 0)   //
				{
					DHCP_STATE = 0;
					NetState = W5500_WORK_STEP_FAULT;
					FTPFist = 1;
				}
				ftpc_run(databuffer);  //文件下载
			}
			
			else
			{
				FTPFist = 0;
			}

		}
		OSTimeDly(SYS_DELAY_5ms);
	}
}


/************************(C)COPYRIGHT 2020 汇誉科技*****END OF FILE*************************/

