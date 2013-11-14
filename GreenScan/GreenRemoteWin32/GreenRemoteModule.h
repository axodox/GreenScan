// GreenRemoteModule.h
#pragma once
#include "ExcaliburClient.h"
using namespace Green::Remoting;

typedef void(_stdcall *SettingValueReceivedCallback)(void* argument, char* name, char* value);
typedef void(_stdcall *CommandDescriptionReceivedCallback)(void* argument, char* name, char* description);
typedef void(_stdcall *Callback)(void* argument);

#define REMOTEPROTOCOL 101

namespace Green
{
	namespace Remoting
	{
		class RemoteControl
		{
		private:
			ExcaliburClient* EC;
			ExcaliburSeeker* ES;
			unsigned short Port;
			bool IsDisposed;

			static void MessageCallback(void* argument, int id, LPSTR text)
			{
				RemoteControl* rc = (RemoteControl*)argument;
				switch (text[0])
				{
				case 'c':
				case 's':
					if (text[1] == '!' && (rc->OnSettingValueReceived || rc->OnCommandDescriptionReceived))
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
						case 'c':
							if (rc->OnCommandDescriptionReceived) rc->OnCommandDescriptionReceived(rc->CallbackArgument, name, value);
							break;
						case 's':
							if (rc->OnSettingValueReceived) rc->OnSettingValueReceived(rc->CallbackArgument, name, value);
							break;
						}
						delete[namelen + 1] name;
						delete[valuelen + 1] value;
					}
					break;
				}
			}

			static void DisconnectCallback(void* argument, ExcaliburClient::DisconnectTypes reason)
			{
				RemoteControl* rc = (RemoteControl*)argument;
				rc->DestroyClient();
				if (!rc->IsDisposed) rc->SetUpSeeker();
			}

			char* CreateMessage(char* prefix, char* command, char* argument, unsigned &len)
			{
				len = strlen(command) + strlen(argument) + 4;
				char* line = new char[len];
				strcpy(line, prefix);
				strcat(line, command);
				strcat(line, "=");
				strcat(line, argument);
				return line;
			}
		public:
			void* CallbackArgument;
			SettingValueReceivedCallback OnSettingValueReceived;
			CommandDescriptionReceivedCallback OnCommandDescriptionReceived;
			Callback OnDisconnected, OnConnected;

			static void OnEndPointFound(void* argument, sockaddr_in address)
			{
				RemoteControl* rc = (RemoteControl*)argument;
				rc->DestroySeeker();
				rc->SetUpClient(address);
			}

			RemoteControl(unsigned short port)
			{
				IsDisposed = false;
				CallbackArgument = nullptr;
				OnSettingValueReceived = nullptr;
				OnCommandDescriptionReceived = nullptr;
				OnConnected = nullptr;
				OnDisconnected = nullptr;

				Port = port;
				ES = nullptr;
				EC = nullptr;
				SetUpSeeker();
			}

			void SetUpClient(sockaddr_in address)
			{
				EC = new ExcaliburClient(address, REMOTEPROTOCOL);
				EC->CallbackArgument = this;
				EC->OnMessageReceived = &MessageCallback;
				EC->OnDisconnected = &DisconnectCallback;
				if (OnConnected) OnConnected(CallbackArgument);
			}

			void DestroyClient()
			{
				if (!EC) return;
				delete EC;
				EC = nullptr;
				if (OnDisconnected) OnDisconnected(CallbackArgument);
			}

			void SetUpSeeker()
			{
				if (ES) return;
				ES = new ExcaliburSeeker(Port, 101);
				ES->CallbackArgument = this;
				ES->OnEndPointFound = &OnEndPointFound;
			}

			void DestroySeeker()
			{
				if (!ES) return;
				delete ES;
				ES = nullptr;
			}

			void SetOptionAsync(char* option, char* value)
			{
				if (!EC) return;
				unsigned len;
				char* line = CreateMessage("s/", option, value, len);
				EC->SendText(line);
				delete[len] line;
			}

			void ExecuteCommandAsync(char* command, char* argument)
			{
				if (!EC) return;
				unsigned len;
				char* line = CreateMessage("c/", command, argument, len);
				EC->SendText(line);
				delete[len] line;
			}

			bool ExecuteCommand(char* command, char* argument, unsigned long timeout)
			{
				if (!EC) return false;
				unsigned len;
				char* line = CreateMessage("c/", command, argument, len);
				char* answer = EC->SendMessageAndWaitForAnswer(line, timeout);
				bool success = answer && answer[2] == 's';
				delete[len] line;
				LPSTRDelete(answer);
				return answer;
			}

			~RemoteControl()
			{
				IsDisposed = true;
				if (EC)
				{
					EC->Disconnect();
				}
				if (ES)
				{
					delete ES;
				}
			}
		};
	}
}

void _stdcall RemoteInit()
{
	WSAData WSAInitData;
	WSAStartup(MAKEWORD(2, 2), &WSAInitData);
}

void _stdcall RemoteShutdown()
{
	WSACleanup();
}

void* _stdcall CreateRemote(unsigned short port)
{
	return new RemoteControl(port);
}

void _stdcall DestroyRemote(void* remote)
{
	RemoteControl* rc = (RemoteControl*)remote;
	delete rc;
}

void _stdcall SetRemoteCallbackArgument(void* remote, void* argument)
{
	RemoteControl* rc = (RemoteControl*)remote;
	rc->CallbackArgument = argument;
}

void _stdcall SetRemoteConnectionCallbacks(void* remote, Callback onConnected, Callback onDisconnected)
{
	RemoteControl* rc = (RemoteControl*)remote;
	rc->OnConnected = onConnected;
	rc->OnDisconnected = onDisconnected;
}

void _stdcall SetRemoteMessageReceivedCallbacks(void* remote,
	SettingValueReceivedCallback onSettingValueReceived,
	CommandDescriptionReceivedCallback onCommandDescriptionReceived)
{
	RemoteControl* rc = (RemoteControl*)remote;
	rc->OnSettingValueReceived = onSettingValueReceived;
	rc->OnCommandDescriptionReceived = onCommandDescriptionReceived;
}

void _stdcall RemoteSetOptionAsync(void* remote, char* option, char* value)
{
	RemoteControl* rc = (RemoteControl*)remote;
	rc->SetOptionAsync(option, value);
}

void _stdcall RemoteExecuteCommandAsync(void* remote, char* command, char* argument)
{
	RemoteControl* rc = (RemoteControl*)remote;
	rc->ExecuteCommandAsync(command, argument);
}

bool _stdcall RemoteExecuteCommand(void* remote, char* command, char* argument, unsigned long timeout)
{
	RemoteControl* rc = (RemoteControl*)remote;
	return rc->ExecuteCommand(command, argument, timeout);
}

void* _stdcall CreateExcaliburSeeker(unsigned short port, long long protocolIdentifier)
{
	return new ExcaliburSeeker(port, protocolIdentifier);
}

void _stdcall DestroyExcaliburSeeker(void* seeker)
{
	ExcaliburSeeker* es = (ExcaliburSeeker*)seeker;
	delete es;
}

void _stdcall SetExcaliburSeekerCallback(void* seeker, void* argument, EndPointFoundCallback onEndPointFound)
{
	ExcaliburSeeker* es = (ExcaliburSeeker*)seeker;
	es->CallbackArgument = argument;
	es->OnEndPointFound = onEndPointFound;
}

