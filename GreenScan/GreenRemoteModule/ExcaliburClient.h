#pragma once
#pragma unmanaged
#include "Stdafx.h"
#define FRAMEDELIMITER 170
using namespace std;


namespace Green
{
	namespace Remoting
	{
		public class ExcaliburClient
		{
		private:
			long long ProtocolIdentifier;
			
			SOCKET Socket;
			PADDRINFOW ClientEndPoint;
			HANDLE SendThread, ReceiveThread;
			class Packet
			{
			public:
				enum class Types : char
				{
					Unknown = 0,
					KeepAlive = 3,
					Text = 32,
					Close = 127
				} Type;
				int Id, Length;
				LPVOID Data;
				Packet(int id, Types type, LPVOID data, int length)
				{
					Id = id;
					Type = type;
					Data = data;
					Length = length;
				}

				~Packet()
				{
					if(Length > 0)
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
			bool CommunicationThreadsEnabled, IsConnected;
		public:
			enum class DisconnectTypes
			{
				Unknown,
				CannotConnect,
				Direct,
				Indirect
			} DisconnectType;

			typedef void (*MessageCallback)(void* argument, int id, LPSTR text);
			typedef void (*DisconnectedCallback)(void* argument, DisconnectTypes reason);
			MessageCallback OnMessageReceived;
			DisconnectedCallback OnDisconnected;
			void* CallbackArgument;
			DWORD KeepAliveTimeout; 			

			static int Init()
			{
				WSAData WSAInitData;
				int result = WSAStartup(MAKEWORD(2, 2), (LPWSADATA)&WSAInitData);
				return result;
			}

			static int Shutdown()
			{
				return WSACleanup();
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
				if(clientSocket == INVALID_SOCKET)
				{
					FreeAddrInfoW(address);
					return 0;
				}

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

			LPSTR SendMessageAndWaitForAnswer(LPSTR text, DWORD timeout)
			{
				if(!IsConnected) return 0;
				int id = SendMessage(text);
				Response* response = new Response();
				EnterCriticalSection(&ResponseSection);
				Responses->emplace(id, response);
				LeaveCriticalSection(&ResponseSection);

				LPSTR answer;
				if(WaitForSingleObject(response->ReceivedEvent, timeout) == WAIT_OBJECT_0)
				{
					answer = (LPSTR)response->Data;
				}
				else
				{
					answer = 0;
				}
				Responses->erase(id);
				delete response;
				return answer;
			}

			int SendMessage(LPSTR text)
			{
				if(!IsConnected) return -1;
				unsigned textlen = strlen(text), lenlen;
				LPSTR message = new char[textlen + 2];
				EncodeULEB128(textlen, message, lenlen);
				memcpy(message + lenlen, text, textlen);
				Packet* packet = new Packet(NextPacketId++, Packet::Types::Text, message, textlen + 2);
				EnterCriticalSection(&QueueSection);
				PacketsToSend->push(packet);
				LeaveCriticalSection(&QueueSection);
				SetEvent(SendEvent);	
				return packet->Id;
			}

			void Disconnect()
			{
				if(IsConnected)
				{
					IsConnected = false;
					DisconnectType = DisconnectTypes::Direct;
					SetEvent(SendEvent);
					WaitForSingleObject(SendThread, INFINITE);
					WaitForSingleObject(ReceiveThread, INFINITE);
				}
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
				OnDisconnected = nullptr;
				ReceiveThread = CreateThread(0, 0, &ExcaliburClient::ConnectAndReceiveProc, this, 0, 0);
				KeepAliveTimeout = 500;			
			}

			static DWORD WINAPI ConnectAndReceiveProc(_In_ LPVOID parameter)
			{
				ExcaliburClient* client = (ExcaliburClient*)parameter;
				client->ConnectWorker();
				return 0;
			}

			static DWORD WINAPI SendProc(_In_ LPVOID parameter)
			{
				ExcaliburClient* client = (ExcaliburClient*)parameter;
				client->SendWorker();
				return 0;
			}

			inline void SocketThrow(int result)
			{
				if(result == SOCKET_ERROR) throw 0;
			}

			void ConnectWorker()
			{
				try
				{
					SocketThrow(connect(Socket, ClientEndPoint->ai_addr, ClientEndPoint->ai_addrlen));
					SocketThrow(send(Socket, (char*)&ProtocolIdentifier, 8, 0));
					
					long long remoteProtocolIdentifier;
					SocketThrow(recv(Socket, (char*)&remoteProtocolIdentifier, 8, MSG_WAITALL));

					if(ProtocolIdentifier == remoteProtocolIdentifier)
					{
						IsConnected = true;
						CommunicationThreadsEnabled = true;
						SendThread = CreateThread(0, 0, &ExcaliburClient::SendProc, this, 0, 0);
						ReceiveWorker();						
					}
					else throw 0;
				}
				catch(int e)
				{
					DisconnectType = DisconnectTypes::CannotConnect;
					IsConnected = false;
					closesocket(Socket);	
					Socket = INVALID_SOCKET;
					if(OnDisconnected) OnDisconnected(CallbackArgument, DisconnectType);
				}
			}

			unsigned DecodeULEB128(const char *data, unsigned &length) 
			{
				const char *start = data;
				unsigned value = 0;
				unsigned shift = 0;
				do {
				  value += (*data & 0x7f) << shift;
				  shift += 7;
				} while (*data++ >= 128);
				length = (unsigned)(data - start);
				return value;
			}

			void EncodeULEB128(unsigned value, char *data, unsigned &length)
			{
				char* output = data;
				char byte;
				length = 0;
				do 
				{
					byte = value & 0x7f;
					value >>= 7;
					if (value != 0)
						byte |= 0x80;
					*output++ = byte;
					length++;
				} 
				while (value != 0);
			}

			LPSTR ReadDodNetString(char* data)
			{
				unsigned lenlen, bytes = DecodeULEB128(data, lenlen);
				LPSTR text = new char[bytes + 1];
				memcpy(text, data + lenlen, bytes);
				text[bytes] = 0;
				return text;
			}
			
			void ReceiveWorker()
			{
				Packet::Types type;
				UINT id, length;
				char* buffer, delimiter;

				try
				{
					while(CommunicationThreadsEnabled)
					{
						buffer = 0;
						length = 0;
						SocketThrow(recv(Socket, (char*)&delimiter, sizeof(delimiter), 0));
						if((BYTE)delimiter != FRAMEDELIMITER) throw 1;
						SocketThrow(recv(Socket, (char*)&length, sizeof(length), 0));
						length -= sizeof(type) + sizeof(id);
						SocketThrow(recv(Socket, (char*)&type, sizeof(type), 0));
						SocketThrow(recv(Socket, (char*)&id, sizeof(id), 0));
					
						if(length > 0)
						{
							buffer = new char[length];
							SocketThrow(recv(Socket, buffer, length, 0));
						}

						switch(type)
						{
						case Packet::Types::Text:
							{
								LPSTR text = ReadDodNetString(buffer);

								EnterCriticalSection(&ResponseSection);
								if(Responses->count(id) > 0)
								{
									Response* r = Responses->at(id);
									r->Length = strlen(text) + 1;
									r->Data = new char[r->Length];
									memcpy(r->Data, text, r->Length);
									SetEvent(r->ReceivedEvent);
								}
								LeaveCriticalSection(&ResponseSection);
								
								if(OnMessageReceived) OnMessageReceived(CallbackArgument, id, text);			
								LPSTRDelete(text);
							}
							break;
						case Packet::Types::Close:
							DisconnectType = DisconnectTypes::Indirect;
							IsConnected = false;
							SetEvent(SendEvent);
							break;
						default:
							break;
						}
						delete [length] buffer;
					}
				}
				catch(int e)
				{
					if(buffer) delete [length] buffer;
					DisconnectType = DisconnectTypes::Unknown;
					IsConnected = false;
					SetEvent(SendEvent);
					if(OnDisconnected) OnDisconnected(CallbackArgument, DisconnectType);
				}
			}

			void SendWorker()
			{
				char delimiter = FRAMEDELIMITER;
				int length;
				Packet* packet = 0;
				
				try
				{
					while(CommunicationThreadsEnabled)
					{
						if(!packet || packet->Type != Packet::Types::KeepAlive)
						{
							WaitForSingleObject(SendEvent, KeepAliveTimeout);
						}

						if(!IsConnected && DisconnectType != DisconnectTypes::Direct)
						{
							break;
						}

						if(!IsConnected && DisconnectType == DisconnectTypes::Direct)
						{
							packet = new Packet(-1, Packet::Types::Close, 0, 0);
							CommunicationThreadsEnabled = false;
						}
						else
						{
							packet = 0;
							EnterCriticalSection(&QueueSection);
							if(PacketsToSend->empty())
								packet = 0;
							else
							{
								packet = PacketsToSend->front();
								PacketsToSend->pop();
							}
							LeaveCriticalSection(&QueueSection);
						
							if(!packet)
							{
								packet = new Packet(-1, Packet::Types::KeepAlive, 0, 0);
							}
						}
						
						SocketThrow(send(Socket, &delimiter, sizeof(delimiter), 0));
						length = packet->Length + sizeof(packet->Type) + sizeof(packet->Id);
						SocketThrow(send(Socket, (char*)&length, sizeof(packet->Length), 0));
						SocketThrow(send(Socket, (char*)&packet->Type, sizeof(packet->Type), 0));
						SocketThrow(send(Socket, (char*)&packet->Id, sizeof(packet->Id), 0));
						switch (packet->Type)
						{
						case Packet::Types::Close:
						case Packet::Types::KeepAlive:
							delete packet;
							break;
						default:
							SocketThrow(send(Socket, (char*)packet->Data, packet->Length, 0));
							delete packet;
							break;
						}
					}
				}
				catch(int e)
				{
					if(packet) delete packet;
				}

				//Free resources
				CommunicationThreadsEnabled = false;
				closesocket(Socket);	
				Socket = INVALID_SOCKET;
			}
		};
	}
}