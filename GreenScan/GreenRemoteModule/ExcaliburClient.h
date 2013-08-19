#pragma once
#pragma unmanaged
#include "Stdafx.h"

using namespace std;

namespace Green
{
	namespace Remoting
	{
		public class ExcaliburClient
		{
		private:
			static WSADATA WSAData;
			long ProtocolIdentifier;
			SOCKET Socket;
			PADDRINFOW ClientEndPoint;
			HANDLE SendThread, ReceiveThread;
			class Packet
			{
			public:
				enum class Types : byte
				{
					Unknown = 0,
					Text
				} Type;
				UINT Id, Length;
				LPVOID Data;
				Packet(UINT id, Types type, LPVOID data, UINT length)
				{
					Id = id;
					Type = type;
					Data = data;
					Length = length;
				}

				~Packet()
				{
					delete [Length] Data;
				}
			};

			class Response
			{
			public:
				HANDLE ReceivedEvent;
				LPVOID Data;
				UINT Length;
				Response()
				{
					ReceivedEvent = CreateEvent(0, 0, 0, 0);
					Data = 0;
					Length = 0u;
				}

				~Response()
				{
					CloseHandle(ReceivedEvent);
				}
			};

			queue<Packet*>* PacketsToSend;
			unordered_map<int, Response*>* Responses;
			UINT NextPacketId;
			HANDLE SendEvent;
			CRITICAL_SECTION QueueSection, ResponseSection;
			bool CommunicationThreadsEnabled;
			
		public:
			typedef void (*MessageCallback)(void* argument, int id, LPWSTR text);
			MessageCallback OnMessageReceived;
			void* CallbackArgument;

			static int Init()
			{
				int result = WSAStartup(MAKEWORD(2, 2), &WSAData);
				return result;
			}

			static ExcaliburClient* Create(LPWSTR host, LPWSTR port, long protocolIdentifier)
			{
				int result;
				ADDRINFOW *address = 0, hints;
				ZeroMemory(&hints, sizeof(hints));
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				result = GetAddrInfoW(host, port, &hints, &address);
				if(result) return 0;
				SOCKET clientSocket;
				clientSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
				if(clientSocket == INVALID_SOCKET) return 0;

				ExcaliburClient* client = new ExcaliburClient(clientSocket, address, protocolIdentifier);
				return client;
			}

			~ExcaliburClient()
			{
				Packet* packet;
				while(!PacketsToSend->empty())
				{
					packet = PacketsToSend->front();
					PacketsToSend->pop();
					delete packet;
				}
				delete PacketsToSend;
				for(pair<int, Response*> h : (*Responses))
				{
					delete h.second;
				}
				delete Responses;
				DeleteCriticalSection(&QueueSection);
				DeleteCriticalSection(&ResponseSection);
				FreeAddrInfoW(ClientEndPoint);
				CloseHandle(SendEvent);
			}

			LPWSTR SendMessageAndWaitForAnswer(LPWSTR text, DWORD timeout)
			{
				int id = SendMessage(text);
				Response* response = new Response();
				EnterCriticalSection(&ResponseSection);
				Responses->emplace(id, response);
				LeaveCriticalSection(&ResponseSection);

				LPWSTR answer;
				if(WaitForSingleObject(response->ReceivedEvent, timeout) == WAIT_OBJECT_0)
				{
					answer = (LPWSTR)response->Data;
				}
				else
				{
					answer = 0;
				}
				Responses->erase(id);
				delete response;
				return answer;
			}

			int SendMessage(LPWSTR text)
			{
				LPWSTR message = new wchar_t[wcslen(text) + 1];
				wcscpy(message, text);
				Packet* packet = new Packet(NextPacketId++, Packet::Types::Text, message, strlen((char*)message) + 1);
				EnterCriticalSection(&QueueSection);
				PacketsToSend->push(packet);
				LeaveCriticalSection(&QueueSection);
				SetEvent(SendEvent);
			}
		private:
			ExcaliburClient(SOCKET clientSocket, PADDRINFOW address, long protocolIdentifier)
			{
				Socket = clientSocket;
				ProtocolIdentifier = protocolIdentifier;
				ClientEndPoint = address;
				PacketsToSend = new queue<Packet*>();
				Responses = new unordered_map<int, Response*>();
				NextPacketId = 0;
				InitializeCriticalSection(&QueueSection);
				InitializeCriticalSection(&ResponseSection);
				SendEvent = CreateEvent(0, 0, 0, 0);
				CallbackArgument = nullptr;
				OnMessageReceived = nullptr;
				ReceiveThread = CreateThread(0, 0, &ExcaliburClient::ConnectAndReceiveProc, this, 0, 0);
				
			}

			static DWORD WINAPI ConnectAndReceiveProc(_In_ LPVOID parameter)
			{
				ExcaliburClient* client = (ExcaliburClient*)parameter;
				client->ConnectWorker();
			}

			static DWORD WINAPI SendProc(_In_ LPVOID parameter)
			{
				ExcaliburClient* client = (ExcaliburClient*)parameter;
				client->SendWorker();
			}

			void SocketThrow(int result)
			{
				if(result == SOCKET_ERROR) throw 0;
			}

			void ConnectWorker()
			{
				try
				{
					SocketThrow(connect(Socket, ClientEndPoint->ai_addr, ClientEndPoint->ai_addrlen));
					SocketThrow(send(Socket, (char*)&ProtocolIdentifier, 8, 0));

					long remoteProtocolIdentifier;
					SocketThrow(recv(Socket, (char*)&remoteProtocolIdentifier, 8, MSG_WAITALL));

					if(ProtocolIdentifier == remoteProtocolIdentifier)
					{
						CommunicationThreadsEnabled = true;
						ReceiveWorker();
						SendThread = CreateThread(0, 0, &ExcaliburClient::SendProc, this, 0, 0);
					}
					else throw 0;
				}
				catch(int e)
				{
					OnDisconnect(DisconnectTypes::CannotConnect);
				}
			}

			void ReceiveWorker()
			{
				Packet::Types type;
				UINT id, length;
				char* buffer;

				while(CommunicationThreadsEnabled)
				{
					recv(Socket, (char*)&type, sizeof(type), 0);
					recv(Socket, (char*)&id, sizeof(id), 0);
					recv(Socket, (char*)&length, sizeof(length), 0);
					
					buffer = new char[length];
					recv(Socket, buffer, length, 0);

					switch(type)
					{
					case Packet::Types::Text:
						EnterCriticalSection(&ResponseSection);
						if(Responses->count(id) > 0)
						{
							SetEvent(Responses->at(id)->ReceivedEvent);
						}
						LeaveCriticalSection(&ResponseSection);
						if(OnMessageReceived)
						{
							OnMessageReceived(CallbackArgument, id, (LPWSTR)buffer);
						}
						else delete [length] buffer;
						break;
					default:
						delete [length] buffer;
						break;
					}
				}
			}

			void SendWorker()
			{
				Packet* packet;
				while(CommunicationThreadsEnabled)
				{
					EnterCriticalSection(&QueueSection);
					if(PacketsToSend->empty())
						packet = 0;
					else
					{
						packet = PacketsToSend->front;
						PacketsToSend->pop();
					}
					LeaveCriticalSection(&QueueSection);

					if(packet)
					{
						switch (packet->Type)
						{
						default:
							send(Socket, (char*)&packet->Type, sizeof(packet->Type), 0);
							send(Socket, (char*)&packet->Id, sizeof(packet->Type), 0);
							send(Socket, (char*)&packet->Length, sizeof(packet->Length), 0);
							send(Socket, (char*)packet->Data, packet->Length, 0);
							delete packet;
							break;
						}						
					}
					else
					{
						WaitForSingleObject(SendEvent, INFINITE);
					}
				}
			}

			enum class DisconnectTypes
			{
				Unknown,
				CannotConnect
			};

			void OnDisconnect(DisconnectTypes type)
			{
				closesocket(Socket);
				Socket = INVALID_SOCKET;

			}
		};

		
	}
}