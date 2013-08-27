// GreenRemoteTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//Callbacks
typedef void (_stdcall *SimpleCallback)(void* argument);
typedef void (_stdcall *SettingValueReceivedCallback)(void* argument, char* name, char* value);
typedef void (_stdcall *CommandDescriptionReceivedCallback)(void* argument, char* name, char* description);

//Functions
typedef void (_stdcall *SimpleFunction)();
typedef void* (_stdcall *CreateRemoteFunction)(unsigned short port);
typedef void (_stdcall *ObjectFunction)(void* object);
typedef void (_stdcall *SetRemoteCallbackArgumentFunction)(void* remote, void* argument);
typedef void (_stdcall *SetRemoteConnectionCallbacksFunction)(void* remote, SimpleCallback onConnected, SimpleCallback onDisconnected);
typedef void (_stdcall *SetRemoteMessageReceivedCallbacksFunction)(void* remote, SettingValueReceivedCallback onSettingValueReceived, CommandDescriptionReceivedCallback onCommandDescriptionReceived);
typedef void (_stdcall *RemoteSendAsyncFunction)(void* remote, char* name, char* param);
typedef void (_stdcall *RemoteSendFunction)(void* remote, char* name, char* param, unsigned long timeout);

using namespace std;

void _stdcall OnRemoteConnected(void* argument)
{
	printf("Remote connected!\r\n");
}

void _stdcall OnRemoteDisconnected(void* argument)
{
	printf("Remote disconnected!\r\n");
}

void _stdcall OnSettingValueReceived(void* argument, char* name, char* value)
{
	printf("%s=%s\r\n", name, value);
}

void _stdcall OnCommandDescriptionReceived(void* argument, char* name, char* description)
{
	printf("%s=%s\r\n", name, description);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//Load library
	HMODULE remoteLib = LoadLibrary(L"GreenRemoteWin32.dll");
	if(!remoteLib)
	{
		printf("DLL not found!\r\n");
		return -1;
	}

	SimpleFunction RemoteInit = (SimpleFunction)GetProcAddress(remoteLib, "RemoteInit");
	SimpleFunction RemoteShutdown = (SimpleFunction)GetProcAddress(remoteLib, "RemoteShutdown");
	CreateRemoteFunction CreateRemote = (CreateRemoteFunction)GetProcAddress(remoteLib, "CreateRemote");
	ObjectFunction DestroyRemote = (ObjectFunction)GetProcAddress(remoteLib, "DestroyRemote");
	ObjectFunction DisconnectRemote = (ObjectFunction)GetProcAddress(remoteLib, "DisconnectRemote");
	SetRemoteCallbackArgumentFunction SetRemoteCallbackArgument = (SetRemoteCallbackArgumentFunction)GetProcAddress(remoteLib, "SetRemoteCallbackArgument");
	SetRemoteConnectionCallbacksFunction SetRemoteConnectionCallbacks = (SetRemoteConnectionCallbacksFunction)GetProcAddress(remoteLib, "SetRemoteConnectionCallbacks");
	SetRemoteMessageReceivedCallbacksFunction SetRemoteMessageReceivedCallbacks = (SetRemoteMessageReceivedCallbacksFunction)GetProcAddress(remoteLib, "SetRemoteMessageReceivedCallbacks");
	RemoteSendAsyncFunction RemoteSetOptionAsync = (RemoteSendAsyncFunction)GetProcAddress(remoteLib, "RemoteSetOptionAsync");
	RemoteSendAsyncFunction RemoteExecuteCommandAsync = (RemoteSendAsyncFunction)GetProcAddress(remoteLib, "RemoteExecuteCommandAsync");
	RemoteSendFunction RemoteExecuteCommand = (RemoteSendFunction)GetProcAddress(remoteLib, "RemoteExecuteCommand");

	//Init
	printf("Initalizing remoting library...\r\n");
	RemoteInit();
	printf("Searching for target...\r\n");
	void* remote = CreateRemote(5656);
	SetRemoteConnectionCallbacks(remote, &OnRemoteConnected, &OnRemoteDisconnected);
	SetRemoteMessageReceivedCallbacks(remote, &OnSettingValueReceived, &OnCommandDescriptionReceived);
	char text[256];
	while (true)
	{
		gets(text);
		if(text[0] == 'e')
			break;
		if(text[1]!='/')
			printf("Enter commands in format s/Setting=value or c/Command=argument!\r\n");
		switch (text[0])
		{
		case 's':
		case 'c':
			{
				char* sepa = strchr(text, '=');
				int namelen = sepa - text - 2;
				char* name = new char[namelen + 1];
				memcpy(name, text + 2, namelen);
				name[namelen] = 0;
				int valuelen = strlen(text) - namelen - 3;
				char* value = new char[valuelen + 1];
				memcpy(value, sepa + 1, valuelen);
				value[valuelen] = 0;
				switch (text[0])
				{
				case 's':
					RemoteSetOptionAsync(remote, name, value);
					break;
				case 'c':
					RemoteExecuteCommandAsync(remote, name, value);
					break;
				}
			}
			break;
		default:
			printf("Enter commands in format s/Setting=value or c/Command=argument!\r\n");
			break;
		}
	}
	DestroyRemote(remote);
	RemoteShutdown();
	return 0;
}
