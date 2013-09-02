#pragma once
#include "Stdafx.h"
#define FRAMEDELIMITER 170
using namespace std;
inline void SocketThrow(int result, bool throwOnNull = false)
{
	if(result == SOCKET_ERROR)
		throw WSAGetLastError();
	if(result == 0 && throwOnNull)
		throw 0;
}

namespace Green
{
	namespace Remoting
	{
		typedef void (*EndPointFoundCallback)(void* argument, sockaddr_in address);
		class ExcaliburSeeker
		{
		private:
			HANDLE SeekerThread, DiscoveryThread;
			SOCKET Socket;
			sockaddr_in BroadcastAddress;
			bool ThreadsOn;
			long long ProtocolIdentifier;
			static DWORD WINAPI SeekerProc(_In_ LPVOID parameter)
			{
				ExcaliburSeeker* es = (ExcaliburSeeker*)parameter;
				while(es->ThreadsOn)
				{					
					try
					{
						SocketThrow(sendto(
							es->Socket, 
							(char*)&es->ProtocolIdentifier, 
							sizeof(es->ProtocolIdentifier),
							0,
							(SOCKADDR*)&(es->BroadcastAddress),
							sizeof(SOCKADDR)));
						Sleep(1000);
					}
					catch(int e)
					{
						es->ThreadsOn = false;
					}
				}
				return 0;
			}

			static DWORD WINAPI DiscoveryProc(_In_ LPVOID parameter)
			{
				ExcaliburSeeker* es = (ExcaliburSeeker*)parameter;
				long long remoteProtocolIdentifier = 0;
				sockaddr_in remoteAddr;
				int remoteAddrSize = sizeof(remoteAddr);
				int bytes;
				while(es->ThreadsOn)
				{
					try
					{
						SocketThrow(bytes = recvfrom(es->Socket, (char*)&remoteProtocolIdentifier, sizeof(remoteProtocolIdentifier), 0, (SOCKADDR*) &remoteAddr, &remoteAddrSize)); 
						if(bytes == sizeof(remoteProtocolIdentifier) && es->ProtocolIdentifier == remoteProtocolIdentifier && es->OnEndPointFound)
						{
							es->OnEndPointFound(es->CallbackArgument, remoteAddr);
						}
						Sleep(1000);
					}
					catch(int e)
					{
						es->ThreadsOn = false;
					}
				}
				return 0;
			}
		public:
			void* CallbackArgument;
			EndPointFoundCallback OnEndPointFound;
			ExcaliburSeeker(unsigned short port, long long protocolIdentifier)
			{
				CallbackArgument = nullptr;
				OnEndPointFound = nullptr;
				ProtocolIdentifier = protocolIdentifier;
				Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				char* optbuf = new char[4];
				optbuf[0] = 1;
				SocketThrow(setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, optbuf, 4));
				*((DWORD*)optbuf) = 10;
				SocketThrow(setsockopt(Socket, IPPROTO_IP, IP_TTL, optbuf, 2));
				delete [4] optbuf;
				
				sockaddr_in AnyAddress;
				ZeroMemory(&AnyAddress, sizeof(AnyAddress));
				AnyAddress.sin_family = AF_INET;
				AnyAddress.sin_port = htons(0);
				AnyAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
				SocketThrow(bind(Socket, (sockaddr*)&AnyAddress, sizeof(AnyAddress)));

				ZeroMemory(&BroadcastAddress, sizeof(BroadcastAddress));
				BroadcastAddress.sin_family = AF_INET;
				BroadcastAddress.sin_port = htons(port);
				BroadcastAddress.sin_addr.S_un.S_addr = htonl(INADDR_BROADCAST);

				ThreadsOn = true;
				SeekerThread = CreateThread(0, 0, &SeekerProc, this, 0, 0);
				DiscoveryThread = CreateThread(0, 0, &DiscoveryProc, this, 0, 0);
			}

			~ExcaliburSeeker()
			{
				ThreadsOn = false;
				closesocket(Socket);
				CloseHandle(SeekerThread);
				CloseHandle(DiscoveryThread);
			}
		};

		class ExcaliburClient
		{
		private:
			long long ProtocolIdentifier;
			
			SOCKET Socket;
			sockaddr_in ClientEndPoint;
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

			queue<Packet*> PacketsToSend;
			unordered_map<int, Response*> Responses;
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

			static ExcaliburClient* Create(PCSTR host, PCSTR port, long long protocolIdentifier)
			{
				int result;
				ADDRINFO *address = 0, hints;
				ZeroMemory(&hints, sizeof(hints));
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				result = getaddrinfo(host, port, &hints, &address);
				if(result) return 0;

				sockaddr_in addrin;
				ZeroMemory(&addrin, sizeof(addrin));
				memcpy(&addrin, address, sizeof(addrin));
				freeaddrinfo(address);

				ExcaliburClient* client = new ExcaliburClient(addrin, protocolIdentifier);				
				return client;
			}

			~ExcaliburClient()
			{
				Packet* packet;
				while(!PacketsToSend.empty())
				{
					packet = PacketsToSend.front();
					PacketsToSend.pop();
					delete packet;
				}
				for(pair<int, Response*> h : Responses)
				{
					delete h.second;
				}
				DeleteCriticalSection(&QueueSection);
				DeleteCriticalSection(&ResponseSection);
				CloseHandle(SendEvent);
				CloseHandle(SendThread);
				CloseHandle(ReceiveThread);
			}

			LPSTR SendMessageAndWaitForAnswer(LPSTR text, DWORD timeout)
			{
				if(!IsConnected) return 0;
				int id = SendText(text);
				Response* response = new Response();
				EnterCriticalSection(&ResponseSection);
				Responses.emplace(id, response);
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
				Responses.erase(id);
				delete response;
				return answer;
			}

			int SendText(LPSTR text)
			{
				if(!IsConnected) return -1;
				unsigned textlen = strlen(text), lenlen;
				LPSTR message = new char[textlen + 2];
				EncodeULEB128(textlen, message, lenlen);
				memcpy(message + lenlen, text, textlen);
				Packet* packet = new Packet(NextPacketId++, Packet::Types::Text, message, textlen + 2);
				EnterCriticalSection(&QueueSection);
				PacketsToSend.push(packet);
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
					WaitForSingleObject(ReceiveThread, INFINITE);
					WaitForSingleObject(SendThread, INFINITE);
				}
			}

			ExcaliburClient(sockaddr_in address, long long protocolIdentifier)
			{
				Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				ProtocolIdentifier = protocolIdentifier;
				ClientEndPoint = address;
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
		private:
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

			void ConnectWorker()
			{
				try
				{
					SocketThrow(connect(Socket, (sockaddr*)&ClientEndPoint, sizeof(ClientEndPoint)));
					SocketThrow(send(Socket, (char*)&ProtocolIdentifier, 8, 0));
					
					long long remoteProtocolIdentifier;
					SocketThrow(recv(Socket, (char*)&remoteProtocolIdentifier, 8, MSG_WAITALL), true);

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
				Packet::Types type = Packet::Types::Unknown;
				UINT id = 0, length = 0, bufferlen = 0;
				char* buffer, delimiter = 0;
								
				try
				{
					while(CommunicationThreadsEnabled && IsConnected)
					{
						buffer = 0;
						length = 0;
						SocketThrow(recv(Socket, &delimiter, sizeof(delimiter), MSG_WAITALL), true);
						if((BYTE)delimiter != FRAMEDELIMITER) throw 1;
						SocketThrow(recv(Socket, (char*)&length, sizeof(length), MSG_WAITALL), true);
						length -= sizeof(type) + sizeof(id);
						SocketThrow(recv(Socket, (char*)&type, sizeof(type), MSG_WAITALL), true);
						SocketThrow(recv(Socket, (char*)&id, sizeof(id), MSG_WAITALL), true);

						
						if(length > 0)
						{
							bufferlen = length;
							buffer = new char[bufferlen];
							SocketThrow(recv(Socket, buffer, length, MSG_WAITALL), true);
						}
						
						switch(type)
						{
						case Packet::Types::Text:
							{
								LPSTR text = ReadDodNetString(buffer);

								EnterCriticalSection(&ResponseSection);
								if(Responses.count(id) > 0)
								{
									Response* r = Responses.at(id);
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
						
						if(bufferlen) 
						{
							delete [bufferlen] buffer;
							bufferlen = 0;
						}
					}
				}
				catch(int e)
				{
					if(bufferlen) delete [bufferlen] buffer;
					DisconnectType = DisconnectTypes::Unknown;
					IsConnected = false;
					SetEvent(SendEvent);					
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
							if(PacketsToSend.empty())
								packet = 0;
							else
							{
								packet = PacketsToSend.front();
								PacketsToSend.pop();
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
				shutdown(Socket, SD_BOTH);
				char a;
				recv(Socket, &a, 1, MSG_WAITALL);
				closesocket(Socket);	
				Socket = INVALID_SOCKET;

				if(OnDisconnected) OnDisconnected(CallbackArgument, DisconnectType);
			}
		};
	}
}