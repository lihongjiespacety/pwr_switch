#include <stdio.h>
#include <Windows.h>
#include <process.h>

static HANDLE portHandle = INVALID_HANDLE_VALUE;
static HANDLE hMutex = 0;

int uart_init(void) 
{
	if(hMutex ==0)
	{
		hMutex = CreateMutex(NULL, FALSE, NULL);
	}
	if(hMutex == 0)	
	{
		return 1;
	}
	else
	{
	    return 0;
	}

}


int uart_open(const char *name) 
{
	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 1;
	}
	if(portHandle != INVALID_HANDLE_VALUE)
	{
        CloseHandle(portHandle);
		portHandle = INVALID_HANDLE_VALUE;
	}
    portHandle = CreateFileA(name, 
        GENERIC_READ|GENERIC_WRITE,  // ע�⴮�ں��������COM9Ӧ����ǰ�����\\.\������COM10��ʾΪ"\\\\.\\COM10"
        0,                           //�������д  
        NULL,                        //��ռ��ʽ  
        OPEN_EXISTING,               //�򿪶����Ǵ���  
        0,                           //0ͬ����ʽ ; FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED�첽��ʽ
        NULL);

    if( portHandle == INVALID_HANDLE_VALUE ) 
	{
        DWORD errorCode = GetLastError();
        if( errorCode == ERROR_FILE_NOT_FOUND ) 
		{
            //���ڲ�����
        }
        else
		{
            //���ڴ��ڴ�ʧ��
		}
		ReleaseMutex(hMutex);
        return errorCode;
    }
	ReleaseMutex(hMutex);
    return 0;
}

int uart_config(DWORD baudrate,BYTE bytesize, BYTE parity, BYTE stopbits,UINT timeout)
{
	/*1.��ȡ���ڵĳ�ʼ����,���޸�ָ������*/
	DCB dcb; 
	COMMTIMEOUTS TimeOuts;


	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 1;
	}

	if(portHandle == INVALID_HANDLE_VALUE)
	{
		ReleaseMutex(hMutex);
        return 1;
	}
	GetCommState(portHandle,(LPDCB)&dcb); 
	dcb.BaudRate=baudrate;     
	dcb.ByteSize=bytesize; 
	dcb.Parity=parity; 
	dcb.StopBits=stopbits; 
	if(0 == SetCommState(portHandle,&dcb))
	{
		ReleaseMutex(hMutex);
        return 1;
	}
	/*2.���û�������С*/
	if(0 == SetupComm(portHandle,4096*2,4096*2))//���뻺����������������Ĵ�С����1024
	{
		ReleaseMutex(hMutex);
        return 1;
	}
	/*3.���ó�ʱ*/
	//GetCommTimeouts(portHandle,&TimeOuts);
	TimeOuts.ReadIntervalTimeout=0x7FFFFFFF;         //���ַ������ʱʱ��  ���ReadIntervalTimeout=MAXDWORD����ReadTotalTimeoutMultiplier=0��ReadTotalTimeoutConstant=0 �򲻵ȴ��������ء� 
	TimeOuts.ReadTotalTimeoutMultiplier=0;   //���ܳ�ʱ��ReadTotalTimeoutMultiplier���ֽ�����ReadTotalTimeoutConstant
	TimeOuts.ReadTotalTimeoutConstant=timeout; 
	TimeOuts.WriteTotalTimeoutMultiplier=0;  //д�ܳ�ʱ 
	TimeOuts.WriteTotalTimeoutConstant=timeout; 
	SetCommTimeouts(portHandle,&TimeOuts);  
    /*6.��ջ�����*/
	PurgeComm(portHandle,PURGE_TXCLEAR | PURGE_RXCLEAR); 
	ReleaseMutex(hMutex);
	return 0;
}

int uart_flush(void)
{
	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 1;
	}
	PurgeComm(portHandle,PURGE_TXCLEAR | PURGE_RXCLEAR);
	ReleaseMutex(hMutex);
	return 0;
}

int uart_close(const char *name) 
{
	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 1;
	}
	if(INVALID_HANDLE_VALUE != portHandle)
	{
		CloseHandle(portHandle);
		portHandle = INVALID_HANDLE_VALUE;
	}
	ReleaseMutex(hMutex);
	return 0;
}


DWORD uart_read(char* buff, DWORD len, BOOL* readstat)
{
	/*ͬ����*/
	DWORD wCount;     //��ȡ���ֽ��� 
	BOOL bReadStat; 

	DWORD dwErrorFlags; 
	COMSTAT ComStat; 

	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 1;
	}

	if(portHandle == INVALID_HANDLE_VALUE)
	{
		ReleaseMutex(hMutex);
	    return 0;
	}
	ClearCommError(portHandle,&dwErrorFlags,&ComStat); 

	bReadStat=ReadFile(portHandle,buff,len,&wCount,NULL); 
	*readstat = bReadStat;
	ReleaseMutex(hMutex);
	return wCount;
}

int uart_send(char* buff, DWORD len, BOOL* writestat)
{
	BOOL bWriteStat; 
	DWORD dwBytesWrite; 

	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 1;
	}

	if(portHandle == INVALID_HANDLE_VALUE)
	{
		ReleaseMutex(hMutex);
	    return 0;
	}
	//PurgeComm(portHandle, PURGE_TXABORT| PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR); 
	bWriteStat=WriteFile(portHandle,buff,len,&dwBytesWrite,NULL); 
	*writestat = bWriteStat;
	ReleaseMutex(hMutex);
	return dwBytesWrite;
}

int uart_trans(BYTE* outbuff, UINT outlen, BYTE* inbuff, UINT inlen,UINT delay)
{
	BOOL writestat;
	DWORD writebytes;
	BOOL readstat;
	DWORD readbytes;
	uart_flush();

	if(WaitForSingleObject(hMutex, 1000) == WAIT_FAILED)
	{
		return 0;
	}
	PurgeComm(portHandle,PURGE_TXCLEAR | PURGE_RXCLEAR); 
	writebytes = uart_send(outbuff, outlen, &writestat);
	if(portHandle == INVALID_HANDLE_VALUE)
	{
		ReleaseMutex(hMutex);
	    return 0;
	}
	if((writebytes != outlen) || (writestat != TRUE))
	{
		ReleaseMutex(hMutex);
		return 0;
	}
	else
	{
		Sleep(delay);
		readbytes = uart_read(inbuff, inlen, &readstat);
		if((readbytes != inlen) || (readstat != TRUE))
		{
			ReleaseMutex(hMutex);
			return 0;
		}
		else
		{
			ReleaseMutex(hMutex);
			return 1;
		}
	}
}