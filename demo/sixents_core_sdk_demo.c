#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "getserialport.h"
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
// sixents head files
#include "sixents_sdk.h"
#include "sixents_types.h"

#define TEST_HTTP 1
#define TEST_ROOTCA 0

#define BAUDRATE        B115200
#define UART_DEVICE     "/dev/ttyUSB0"
#define FALSE  -1
#define TRUE   0

#define STATE_INIT    0
#define STATE_START   1
#define STATE_RUNNING 2
#define SIX_GPGGA ','


// 模拟GGA数据（实际集成时以定位模组或芯片输出的GGA数据为准）
// const char gga[] = "$GPGGA,000001,3959.776019,N,11602.363141,E,1,8,1,100.000,M,0,M,3,0*46\r\n";
// int num = 0;

// 模拟GGA数据（实际集成时以定位模组或芯片输出的GGA数据为准）
sixents_char gga[1024] = {0} ;
sixents_char buf[1024] = {0} ;
static int fd;
// static pthread_mutex_t g_mutex_recv_info = PTHREAD_MUTEX_INITIALIZER;
// static pthread_cond_t gCond_recv_DATA = PTHREAD_COND_INITIALIZER;         //用于 的同步信号量
//static recv_buf recvbuf;
//static int recv_done = FALSE;
/**
*@brief  设置串口通信速率
*@param  fd     类型 int  打开串口的文件句柄
*@param  speed  类型 int  串口速度
*@return  void
*/
const int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
          		   B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
const int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200,  300, 
		  		  115200, 38400, 19200, 9600, 4800, 2400, 1200,  300, };

void set_speed(int fd, int speed)
{
  int   i; 
  int   status; 
  struct termios   Opt;
  tcgetattr(fd, &Opt); //读取终端参数
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) { 
    if  (speed == name_arr[i]) {     
      tcflush(fd, TCIOFLUSH);     
      cfsetispeed(&Opt, speed_arr[i]); //设置输入波特率 
      cfsetospeed(&Opt, speed_arr[i]); //设置输出波特率  
      status = tcsetattr(fd, TCSANOW, &Opt);  //设置终端参数
      if  (status != 0) {        
        perror("tcsetattr fd1");  
        return;     
      }    
      tcflush(fd,TCIOFLUSH);   
    }  
  }
}

/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄
*@param  databits 类型  int 数据位   取值 为 7 或者8
*@param  stopbits 类型  int 停止位   取值为 1 或者2
*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
int set_parity(int fd,int databits,int stopbits,int parity)
{ 
	struct termios options; 
	if  ( tcgetattr( fd,&options)  !=  0) { 
		perror("SetupSerial 1");     
		return(FALSE);  
	}
    

	options.c_cflag &= ~CSIZE; 
	switch (databits) /*设置数据位数*/
	{   
	case 7:		
		options.c_cflag |= CS7; 
		break;
	case 8:     
		options.c_cflag |= CS8;
		break;   
	default:    
		fprintf(stderr,"Unsupported data size\n"); return (FALSE);  
	}
	switch (parity) 
	{   
		case 'n':
		case 'N':    
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */ 
			break;  
		case 'o':   
		case 'O':     
			options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/  
			options.c_iflag |= INPCK;             /* Disnable parity checking */ 
			break;  
		case 'e':  
		case 'E':   
			options.c_cflag |= PARENB;     /* Enable parity */    
			options.c_cflag &= ~PARODD;   /* 转换为偶效验*/     
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S': 
		case 's':  /*as no parity*/   
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;break;  
		default:   
			fprintf(stderr,"Unsupported parity\n");    
			return (FALSE);  
		}  
	/* 设置停止位*/  
	switch (stopbits)
	{   
		case 1:    
			options.c_cflag &= ~CSTOPB;  
			break;  
		case 2:    
			options.c_cflag |= CSTOPB;  
		   break;
		default:    
			 fprintf(stderr,"Unsupported stop bits\n");  
			 return (FALSE); 
	} 
	/* Set input parity option */ 
	if (parity != 'n')   
		options.c_iflag |= INPCK; 
	tcflush(fd,TCIFLUSH);

    //时间控制，原始模式有效
	options.c_cc[VTIME] = 10*10; /* 设置超时10 seconds(单位1/10s 100ms)*/   
	options.c_cc[VMIN] = 0; 

//     options.c_cc[VMIN] = FRAME_MAXSIZE;   //阻塞条件下有效
    //如果不是开发终端之类的，只是串口传输数据，而不需要串口来处理，那么使用原始模式(Raw Mode)方式来通讯，~ICANON设置串口为原始模式,设置方式如下：
    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    options.c_oflag  &= ~OPOST;   /*Output*/

    /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0)   
	{ 
		perror("SetupSerial 3");   
		return (FALSE);  
	} 

	return (TRUE);  
}

// 获取差分数据的回调函数
void GetDiffData(const sixents_char* buff, sixents_uint32 len)
{
        sixents_uint32 i;
        int res = 0;
        printf("len is: %d\n",len);
    // if (buff == sixents_null || len == 0)
    // {
    //     return;
    // }
    // // ToDo: 以下应该修改为客户自己对差分数据处理的逻辑
    char *str;
    str = (char *)malloc(2*len*sizeof(char)+2);
    for (i = 0; i < len; i++)
    {
      char str_temp[2];
      //printf("%02x ", (sixents_uint8)buff[i]);
      sprintf(str_temp,"%02X",(sixents_uint8)buff[i]);
      strcpy(&str[i*2],str_temp);
      //write(fd,str,sizeof(str));
    }
    str[2*len*sizeof(char)] = '\r';
    str[2*len*sizeof(char)+1] = '\n';
      printf("%s",str);
      //close(fd);
    char *fd_str;
    fd_str = (char *)malloc(len+2);
      
    for(int i=0;i<len;i++)
    {
      fd_str[i] = buff[i];
    }
    fd_str[len] = '\r';
    fd_str[len+1] = '\n';
    
    fd = open(UART_DEVICE, O_RDWR|O_NOCTTY);
    write(fd,fd_str,len+2);
    for (i = 0; i < len; i++)
    {
         //printf("%02X ", ( sixents_uint8 )buff[i]);
         //if ((i + 1) % 80u == 0)
         //{
             //printf("\n");
         //}
    }
     printf("\n");
     //printf("\r\n");

}

// 获取网络、鉴权状态的回调函数
void GetStatus(sixents_uint32 status)
{
    // ToDo: 可根据客户自己的需求进行开发
    printf("%s| Current SDK Status:%d.\n", __FUNCTION__, status);
}

// 入口函数
int main(int argc, char* argv[])
{
    sixents_int32 retVal = 0;
    sixents_uint32 count = 0;
    const sixents_char* curVer;
    int csdk_run_step = 0;
    sixents_sdkConf param; // SDK初始化配置参数

#if TEST_HTTP
    const sixents_char* const curAK = "******";//以下四行内容需要填入六分服务提供商提供的权鉴信息
    const sixents_char* const curAS = "******";
    const sixents_char* const curDevID = "*****";
    const sixents_char* const curDevType = "******";
#else
    const sixents_char* const curAK = "6378599618";
    const sixents_char* const curAS = "kd7v0ayh076zsuefiixqspnd84bgmadxghnj7eckwiwmjjm2nz5dq4ppzrpctcte";
    const sixents_char* const curDevID = "lfuat008";
    const sixents_char* const curDevType = "j1b0qy";
    const sixents_char* const curAuthServ = "lfycuat.sixents.com";
    const sixents_uint16 curAuthPort = 443;
    const sixents_char* const curRtcmServ = "openapitest.sixents.com";
    const sixents_uint16 curRtcmPort = 8001;
    // 测试ECC证书：testca.crt
#if TEST_ROOTCA
    const char* const curRootCA = "-----BEGIN CERTIFICATE-----\r\n"
                                  "MIICXjCCAgSgAwIBAgIUfYrn/H2mM8icrUYjaofO+RWMNpUwCgYIKoZIzj0EAwIw"
                                  "gYUxCzAJBgNVBAYTAkNOMQswCQYDVQQIDAJCSjELMAkGA1UEBwwCQkoxEDAOBgNV"
                                  "BAoMB3NpeGVudHMxEDAOBgNVBAsMB3Nka2F1dGgxFDASBgNVBAMMC3NpeGVudHMu"
                                  "Y29tMSIwIAYJKoZIhvcNAQkBFhNzaXhlbnRzQHNpeGVudHMuY29tMB4XDTIwMTIw"
                                  "NzExNTA0MFoXDTMwMTIwNTExNTA0MFowgYUxCzAJBgNVBAYTAkNOMQswCQYDVQQI"
                                  "DAJCSjELMAkGA1UEBwwCQkoxEDAOBgNVBAoMB3NpeGVudHMxEDAOBgNVBAsMB3Nk"
                                  "a2F1dGgxFDASBgNVBAMMC3NpeGVudHMuY29tMSIwIAYJKoZIhvcNAQkBFhNzaXhl"
                                  "bnRzQHNpeGVudHMuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEwJApbfBH"
                                  "+QA00+2CRXzu1mc/X1q8DUyKGGEm3VOpotI7Slsrcw9ITCbF70Vj2NNGWw18NMcU"
                                  "24s8QNJ+JY6206NQME4wHQYDVR0OBBYEFNW2sbUU3OT17rErMhOiF0xA1jeVMB8G"
                                  "A1UdIwQYMBaAFNW2sbUU3OT17rErMhOiF0xA1jeVMAwGA1UdEwQFMAMBAf8wCgYI"
                                  "KoZIzj0EAwIDSAAwRQIhAKDsXeWFPcPJTR54BWlZh6P42IxAEOnhMIVGGinMDPHI"
                                  "AiBq4U0wxXIiCiXHhzu0E21IxVn17meI24IZ9shflOAO6A=="
                                  "-----END CERTIFICATE-----";
#else
/*    // 六分证书: root.crt
    const char* const curRootCA = "-----BEGIN CERTIFICATE-----\r\n"
                                  "MIIEMjCCAxqgAwIBAgIBATANBgkqhkiG9w0BAQUFADB7MQswCQYDVQQGEwJHQjEb\r\n"
                                  "MBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYDVQQHDAdTYWxmb3JkMRow\r\n"
                                  "GAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UEAwwYQUFBIENlcnRpZmlj\r\n"
                                  "YXRlIFNlcnZpY2VzMB4XDTA0MDEwMTAwMDAwMFoXDTI4MTIzMTIzNTk1OVowezEL\r\n"
                                  "MAkGA1UEBhMCR0IxGzAZBgNVBAgMEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UE\r\n"
                                  "BwwHU2FsZm9yZDEaMBgGA1UECgwRQ29tb2RvIENBIExpbWl0ZWQxITAfBgNVBAMM\r\n"
                                  "GEFBQSBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczCCASIwDQYJKoZIhvcNAQEBBQADggEP\r\n"
                                  "ADCCAQoCggEBAL5AnfRu4ep2hxxNRUSOvkbIgwadwSr+GB+O5AL686tdUIoWMQua\r\n"
                                  "BtDFcCLNSS1UY8y2bmhGC1Pqy0wkwLxyTurxFa70VJoSCsN6sjNg4tqJVfMiWPPe\r\n"
                                  "3M/vg4aijJRPn2jymJBGhCfHdr/jzDUsi14HZGWCwEiwqJH5YZ92IFCokcdmtet4\r\n"
                                  "YgNW8IoaE+oxox6gmf049vYnMlhvB/VruPsUK6+3qszWY19zjNoFmag4qMsXeDZR\r\n"
                                  "rOme9Hg6jc8P2ULimAyrL58OAd7vn5lJ8S3frHRNG5i1R8XlKdH5kBjHYpy+g8cm\r\n"
                                  "ez6KJcfA3Z3mNWgQIJ2P2N7Sw4ScDV7oL8kCAwEAAaOBwDCBvTAdBgNVHQ4EFgQU\r\n"
                                  "oBEKIz6W8Qfs4q8p74Klf9AwpLQwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQF\r\n"
                                  "MAMBAf8wewYDVR0fBHQwcjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5jb20v\r\n"
                                  "QUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNqA0oDKGMGh0dHA6Ly9jcmwuY29t\r\n"
                                  "b2RvLm5ldC9BQUFDZXJ0aWZpY2F0ZVNlcnZpY2VzLmNybDANBgkqhkiG9w0BAQUF\r\n"
                                  "AAOCAQEACFb8AvCb6P+k+tZ7xkSAzk/ExfYAWMymtrwUSWgEdujm7l3sAg9g1o1Q\r\n"
                                  "GE8mTgHj5rCl7r+8dFRBv/38ErjHT1r0iWAFf2C3BUrz9vHCv8S5dIa2LX1rzNLz\r\n"
                                  "Rt0vxuBqw8M0Ayx9lt1awg6nCpnBBYurDC/zXDrPbDdVCYfeU0BsWO/8tqtlbgT2\r\n"
                                  "G9w84FoVxp7Z8VlIMCFlA2zs6SFz7JsDoeA3raAVGI/6ugLOpyypEBMs1OUIJqsi\r\n"
                                  "l2D4kF501KKaU73yqWjgom7C12yxow+ev+to51byrvLjKzg6CYG1a4XXvi3tPxq3\r\n"
                                  "smPi9WIsgtRqAEFQ8TmDn5XpNpaYbg==\r\n"
                                  "-----END CERTIFICATE-----\r\n";
*/
   const char* const curRootCA = "-----BEGIN CERTIFICATE-----\r\n"
"MIIDsjCCApoCCQDYCJMo4+B1PzANBgkqhkiG9w0BAQsFADCBmTELMAkGA1UEBhMC"
"Q04xEDAOBgNVBAgMB0JlaUppbmcxEDAOBgNVBAcMB0JlaUppbmcxLTArBgNVBAoM"
"JEJlaWppbmcgU2l4ZW50cyBUZWNobm9sb2d5IENvLiwgTHRkLjEVMBMGA1UECwwM"
"U2l4ZW50cyBHTlNTMSAwHgYDVQQDDBdTaXhlbnRzIHlpbmdjaGUgcm9vdCBjYTAg"
"Fw0yMTAzMDQwOTI5NTdaGA8yMTIxMDIwODA5Mjk1N1owgZkxCzAJBgNVBAYTAkNO"
"MRAwDgYDVQQIDAdCZWlKaW5nMRAwDgYDVQQHDAdCZWlKaW5nMS0wKwYDVQQKDCRC"
"ZWlqaW5nIFNpeGVudHMgVGVjaG5vbG9neSBDby4sIEx0ZC4xFTATBgNVBAsMDFNp"
"eGVudHMgR05TUzEgMB4GA1UEAwwXU2l4ZW50cyB5aW5nY2hlIHJvb3QgY2EwggEi"
"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCwzivmwxeX5GyRIJfeO3TkMool"
"ptEl6XSe8YZ0vHpdIocQnhVCCGC1Cs2+VCNoRc5F+XF2PvaUSd+P5uGGiO4kx+NA"
"aGE4JmqNwoAMjRj9qzImXy58PgEGVuKuEMIUf4yN09g5Sd2awfZYzBOy18ouYhAn"
"VNQvV/QX5UzwGhBs+lBCvrnFNyGaHwVPS2My8L55oI4An92CWzRLJLVGs+tL8e5Y"
"9E9Zwv8yaT93omjODFN5UOdiT2aslluF+7g3UG169fPXR+P1RduZO/gnggpzcLFP"
"sOJ1+oRk0wgxiNWLN5MJgywE7PBpI13+IMT0a0P3NuwWuoXZGhz3OE7u5i5pAgMB"
"AAEwDQYJKoZIhvcNAQELBQADggEBAGTHNso050vTMTGJazQWY/m/yQYMUZaN1jQw"
"K+ho7FXBNme1Yx1AvSKuqrikec8LE2aRzLeuOzWUNSEQphT4Cw9jTUXG0GG6ZC0s"
"mCOld6RLyy9BUVjHxET9ZzFcQ5AVeI0/mCFfnNo6GyWmKMhYctToxms2/nLQzbfS"
"PUYPBNNRKQw6rE7AD84/DhoiJXz7VuSL+YcPYu7ljfNlVmywQPDJeVVFKym9YuCi"
"gWE3DpIkeRxNvbVFg/fD3O3wvUzxwZewv1mk1cECUCfln+HoiOFlUGH2ZzXAp8qu"
"zmQ7q4lRW76FcUf1c3GdMeCeszJVE4A5n5qhl8ec1OZykaVxP1A="
"-----END CERTIFICATE-----";
#endif
#endif

    // 获取SDK 版本号
    curVer = sixents_sdkGetVer();
    if (curVer != sixents_null)
    {
        printf("%s| Current SDK Version:%s.\n", __FUNCTION__, curVer);
    }

    // step 0: 初始化配置参数
    memset(&param, 0, sizeof(sixents_sdkConf));
    // 鉴权信息
    param.keyType = SIXENTS_KEY_TYPE_AK;                   // 鉴权方式为AK/AS方式
    memcpy(param.key, curAK, strlen(curAK));               // 设置您购买服务实例的AK
    memcpy(param.secret, curAS, strlen(curAS));            // 设置您购买服务实例的AS
    memcpy(param.devID, curDevID, strlen(curDevID));       // 设置您的设备ID或SN
    memcpy(param.devType, curDevType, strlen(curDevType)); // 设置您的设备类型

    param.logPrintLevel = SIXENTS_LL_DEBUG; // 关闭日志打印，打开日志可以用于调试
    param.timeout = 10u;                    // 网络超时时长，10s
    param.sockIOFlag = SIXENTS_SOCK_IOFLAG_NOBLOCK;

#if TEST_HTTP
    //param.pid = SIXENTS_PT_HTTP;
    //param.rootCA = sixents_null;
#else
    param.pid = SIXENTS_PT_HTTPS;
    memcpy(param.openApiHost, curAuthServ, strlen(curAuthServ));
    param.openApiPort = curAuthPort;
    memcpy(param.serverHost, curRtcmServ, strlen(curRtcmServ));
    param.serverPort = curRtcmPort;
    param.rootCA = curRootCA;
#endif

    // 回调函数配置
    param.cbGetDiffData = &GetDiffData; // 获取差分数据
    param.cbGetStatus = &GetStatus;     // 获取网络、鉴权等状态

    // 判断物理网络是否可用，如果可用，则设置当前网络状态设置为SIXENTS_NWSTATUS_ON
    sixents_sdkSetNwStatus(SIXENTS_NWSTATUS_ON);

    int   c=0, res;
    
	printf("uart Start...\n");
    fd = open(UART_DEVICE, O_RDWR|O_NOCTTY);
 
    if (fd < 0) {
        perror(UART_DEVICE);
        exit(1);
    }
 
    printf("uart Open...\n");
    set_speed(fd,115200);
	if (set_parity(fd,8,1,'N') == FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}


    while (SIXENTS_TRUE)
    {
        usleep(200 * 1000); // 200ms
        /****************************************
         * csdk_run_step: 0 sdk init
         *                1 sdk start
         *                2 work
         ****************************************/
        switch (csdk_run_step)
        {
            case 0:
            {
                retVal = sixents_sdkInit(&param);
                if (retVal != SIXENTS_RET_OK)
                {
                    printf("---MAIN case 0| sixents_sdkInit failed! retVal:%d.\r\n", retVal);
                    sixents_sdkFinal();
                }
                else
                {
                    retVal = sixents_sdkSetBuff(SIXENTS_BUFF_BIG);
                    printf("---MAIN case 0| sixents_sdkInit success---\r\n");
                    csdk_run_step = 1;
                }
            }
            break;
            case 1:
            {
                printf("---MAIN case 1| sixents_sdkStart start---\r\n");
                retVal = sixents_sdkStart();
                if (retVal != SIXENTS_RET_OK)
                {
                    printf("---MAIN case 1| sixents_sdkStart failed! retVal:%d ---\r\n", retVal);
                    usleep(1000 * 1000); // 0.3s
                }
                else
                {
                    printf("---MAIN case 1| sixents_sdkStart success---\r\n");
                    csdk_run_step = 2;
                }
            }
            break;
            case 2:
            {
                //if ((count < 11 && (count % 5) == 0)  || (count > 320 && (count % 5) == 0)) // 模拟60s不发送gga断开连接
                if ((count % 5) == 0) // 1s发送一次GGA
                {
                    res = read(fd, buf, 90);         
                    if(res==0)
                     continue;
                    buf[res]=0;
                    printf("send gga is: %s\n",buf);
                   
                    //sixents_char gga_temp[3];
                    printf("buf_len %d\n",strlen(buf));
                    retVal = sixents_sdkSendGGAStr(buf, strlen(buf));
                    if (retVal != SIXENTS_RET_OK)
                    {
                        printf("%s| Send GGA failed with error: %d.\n", __FUNCTION__, retVal);
                    }
                }

                retVal = sixents_sdkTick();
                if (retVal != SIXENTS_RET_OK)
                {
                    printf("%s| sdkTick failed with error: %d.\n", __FUNCTION__, retVal);
                }
                count++;
            }
            break;
            default:
                printf("[six-log]:switch nothing\r\n");
                break;
        }
        // check suspend flag and suspend task
        // if (Six_RTK_State_Flag == 0)
        //{
        //    csdk_run_step = 0; // state to 0
        //    // suspend csdk task and printf log
        //    sixents_sdkFinal();
        //    printf("[six-log]:*4-suspend sixents_task_CSDK !\r\n");
        //}
    } // while

    return retVal;
}
