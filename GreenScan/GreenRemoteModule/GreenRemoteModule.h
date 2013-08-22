// GreenRemoteModule.h
#include "ExcaliburClient.h"
#pragma once
#pragma managed
using namespace System;
using namespace Green::Remoting;

namespace Green
{
	namespace Remoting
	{
		public class RemoteControl
		{
		private:
			ExcaliburClient* EC;

			static void MessageCallback(void* argument, int id, LPSTR text)
			{
				RemoteControl* rc = (RemoteControl*)argument;
				switch(text[0])
				{
				case 'c':
				case 's':
					if(text[1] == '!' && (rc->OnSettingValueReceived || rc->OnCommandDescriptionReceived))
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
						switch(text[0])
						{
						case 'c':
							if(rc->OnCommandDescriptionReceived) rc->OnCommandDescriptionReceived(rc->CallbackArgument, name, value);
							break;
						case 's':
							if(rc->OnSettingValueReceived) rc->OnSettingValueReceived(rc->CallbackArgument, name, value);
							break;
						}
						delete [namelen + 1] name;
						delete [valuelen + 1] value;
					}
					break;
				}
			}

			static void DisconnectCallback(void* argument, ExcaliburClient::DisconnectTypes reason)
			{
				RemoteControl* rc = (RemoteControl*)argument;
				if(rc->OnDisconnected) rc->OnDisconnected(rc->CallbackArgument);				
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
			typedef void (*SettingValueReceivedCallback)(void* argument, char* name, char* value);
			typedef void (*CommandDescriptionReceivedCallback)(void* argument, char* name, char* description);
			typedef void (*Callback)(void* argument);
			SettingValueReceivedCallback OnSettingValueReceived;
			CommandDescriptionReceivedCallback OnCommandDescriptionReceived;
			Callback OnDisconnected;

			RemoteControl()
			{
				CallbackArgument = nullptr;
				OnSettingValueReceived = nullptr;
				OnCommandDescriptionReceived = nullptr;
				OnDisconnected = nullptr;
				EC = ExcaliburClient::Create(L"127.0.0.1", L"5656", 101);
				if(EC)
				{
					EC->CallbackArgument = this;
					EC->OnMessageReceived = &MessageCallback;
					EC->OnDisconnected = &DisconnectCallback;
				}
			}

			void SetOptionAsync(char* option, char* value)
			{
				unsigned len;
				char* line = CreateMessage("s/", option, value, len);
				EC->SendMessage(line);
				delete [len] line;
			}

			void ExecuteCommandAsync(char* command, char* argument)
			{
				unsigned len;
				char* line = CreateMessage("c/", command, argument, len);
				EC->SendMessage(line);
				delete [len] line;
			}

			bool ExecuteCommand(char* command, char* argument, unsigned long timeout)
			{
				unsigned len;
				char* line = CreateMessage("c/", command, argument, len);
				char* answer = EC->SendMessageAndWaitForAnswer(line, timeout);
				bool success = answer && answer[2] == 's';
				delete [len] line;
				LPSTRDelete(answer);
				return answer;
			}

			void Disconnect()
			{
				EC->Disconnect();
			}

			static void Init()
			{
				ExcaliburClient::Init();
			}

			static void Shutdown()
			{
				ExcaliburClient::Shutdown();
			}
		};

		public ref class RemoteControlTester
		{
		private:
			RemoteControl* RC;
		public:
			RemoteControlTester()
			{
				RC = new RemoteControl();
			}

			void Disconnect()
			{
				RC->Disconnect();
			}

			void TestSetting()
			{
				RC->SetOptionAsync("Preprocessing.DepthGaussIterations", "0");	
			}

			void Close()
			{
				RC->ExecuteCommand("Close","",1000);
			}

			static void Init()
			{
				RemoteControl::Init();
			}

			static void Shutdown()
			{
				RemoteControl::Shutdown();
			}
		};
	}
}