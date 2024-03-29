// ConsoleApplication5.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
using namespace std;
#include "stdio.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#pragma comment(lib,"ws2_32.lib")
typedef struct _IP_HEADER //定义IP头
{
	union
	{
		BYTE Version; //版本（前4位）
		BYTE HdrLen; //IHL（后4位），标头长&首部长度
	};
	BYTE ServiceType; //服务类型
	WORD TotalLen; //16位总长度
	WORD ID; //16位标识
	union
	{
		WORD Flags; //3位标志
		WORD FragOff; //分段偏移（后13位）
	};
	BYTE TimeToLive; //生命期
	BYTE Protocol; //协议
	WORD HdrChksum; //头校验和
	DWORD SrcAddr; //源地址
	DWORD DstAddr; //目的地址
	BYTE Options; //选项
}IP_HEADER;
//逐位解析IP头中的信息
void getVersion(BYTE b, BYTE &version)
{
	version = b >> 4; //右移4位,获取版本字段
}
void getIHL(BYTE b, BYTE &result)
{
	result = (b & 0x0f) * 4; //获取头部长度字段
}
const char * parseServiceType_getProcedence(BYTE b)
{
	switch (b >> 5) //获取服务类型字段中优先级子域
	{
	case 7:
		return "Network Control";
		break;
	case 6:
		return "Internet work Control";
		break;
	case 5:
		return "CRITIC/ECP";
		break;
	case 4:
		return "Flash Override";
		break;
	case 3:
		return "Flsah";
		break;
	case 2:
		return "Immediate";
		break;
	case 1:
		return "Priority";
		break;
	case 0:
		return "Routine";
		break;
	default:
		return "Unknow";
		break;
	}
}
const char * parseServiceType_getTOS(BYTE b)
{
	b = (b >> 1) & 0x0f; //获取服务类型字段中的TOS子域
	switch (b)
	{
	case 0:
		return "Normal service";
		break;
	case 1:
		return "Minimize monetary cost";
		break;
	case 2:
		return "Maximize reliability";
		break;
	case 4:
		return "Maximize throughput";
		break;
	case 8:
		return "Minimize delay";
		break;
	case 15:
		return "Maximize security";
		break;
	default:
		return "Unknow";
	}
}
void getFlags(WORD w, BYTE &DF, BYTE &MF) //解析标志字段
{
	DF = (w >> 14) & 0x01;
	MF = (w >> 13) & 0x01;
}
void getFragOff(WORD w, WORD &fragOff) //获取分段偏移字段
{
	fragOff = w & 0x1fff;
}
const char * getProtocol(BYTE Protocol) //获取协议字段共8位
{
	switch (Protocol) //以下为协议号说明：
	{
	case 1:
		return "ICMP";
	case 2:
		return "IGMP";
	case 4:
		return "IP in IP";
	case 6:
		return "TCP";
	case 8:
		return "EGP";
	case 17:
		return "UDP";
	case 41:
		return "IPv6";
	case 46:
		return "RSVP";
	case 89:
		return "OSPF";
	default:
		return "UNKNOW";
	}
}
void ipparse(FILE* file, char* buffer)
{
	IP_HEADER ip = *(IP_HEADER*)buffer; //通过指针把缓冲区的内容强制转化为IP_HEADER数据结构
	fseek(file, 0, SEEK_END);
	BYTE Version;
	getVersion(ip.Version, Version);
	fprintf(file, "版本号=%d\r\n", Version);
	BYTE headerLen;
	getIHL(ip.HdrLen, headerLen);
	fprintf(file, "报头标长=%d (BYTE)\r\n", headerLen);
	fprintf(file, "服务类型=%s,%s\r\n", parseServiceType_getProcedence(ip.ServiceType), parseServiceType_getTOS(ip.ServiceType));
	fprintf(file, "总长度=%d (BYTE)\r\n", ip.TotalLen);
	fprintf(file, "标识=%d\r\n", ip.ID);
	BYTE DF, MF;
	getFlags(ip.Flags, DF, MF);
	fprintf(file, "标志 DF=%d,MF=%d\r\n", DF, MF);
	WORD fragOff;
	getFragOff(ip.FragOff, fragOff);
	fprintf(file, "分段偏移值=%d\r\n", fragOff);
	fprintf(file, "生存期=%d (hopes)\r\n", ip.TimeToLive);
	fprintf(file, "协议=%s\r\n", getProtocol(ip.Protocol));
	fprintf(file, "头校验和=0x%0x\r\n", ip.HdrChksum);
	fprintf(file, "源IP地址=%s\r\n", inet_ntoa(*(in_addr*)&ip.SrcAddr));
	fprintf(file, "目的IP地址=%s\r\n", inet_ntoa(*(in_addr*)&ip.DstAddr));
	fprintf(file, "---------------------------------------------\r\n");
}
int main()
{
	int nRetCode = 0;
	// initialize MFC and print and error on failure
	{
		// TODO: code your application's behavior here.
		FILE * file;
		if ((file = fopen("history.txt", "wb+")) == NULL)
		{
			printf("fail to open file %s");
			return -1;
		}
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);
		//建立套接字
		SOCKET sock;
		sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
		BOOL flag = TRUE;
		//设置IP头操作选项，用户可对IP头处理
		setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag));
		char hostName[128];
		gethostname(hostName, 100);
		//获取本地地址
		hostent * pHostIP;
		pHostIP = gethostbyname(hostName);
		//填充SOCKADDR_IN结构
		sockaddr_in addr_in;
		addr_in.sin_addr = *(in_addr*)pHostIP->h_addr_list[0];
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(6000);
		bind(sock, (PSOCKADDR)&addr_in, sizeof(addr_in)); //把socket绑定到本地网卡
		DWORD dwValue = 1;
		//设置SOCK_RAW为SIO_RCVALL,能接收所有IP包
#define IO_RCVALL _WSAIOW(IOC_VENDOR,1)
		DWORD dwBufferLen[10];
		DWORD dwBufferInLen = 1;
		DWORD dwBytesReturned = 0;
		WSAIoctl(sock, IO_RCVALL, &dwBufferInLen, sizeof(dwBufferInLen), &dwBufferLen, sizeof(dwBufferLen), &dwBytesReturned, NULL, NULL);
		//设置接受数据包缓冲区长度
#define BUFFER_SIZE 65535
		char buffer[BUFFER_SIZE];
		//监听网卡
		printf("开始解析YZF的电脑:\n");
		char inkey;
		do
		{
			int size = recv(sock, buffer, BUFFER_SIZE, 0);
			if (size>0)
				ipparse(stdout, buffer);
			if (size>0)
				ipparse(file, buffer);
			cout << "是否继续?(y/n)";
			cin >> inkey;
			cout << inkey;
		} while (inkey == 'y' || inkey == 'Y');
		fclose(file);
		return 0;
	}
	return nRetCode;
}