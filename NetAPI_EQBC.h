#pragma once

#include "..\MQ2Plugin.h"
#include "z85.h"

typedef unsigned long DWORD;
#define OFFSET_NETBOTS_STRING 0x4A2B4
#define OFFSET_EQBC_INSTANCE 0x5AC58
#define OFFSET_TRANSMIT_FUNC 0x81A0
typedef void(__thiscall *fEQBCTransmit)(void *instConnectionManager, bool bHandleDisconnect, char* szMsg);
//void __thiscall CConnectionMgr::Transmit(CConnectionMgr *this, bool bHandleDisconnect, char *szMsg)

extern void OnIncomingBotPacket(PCHAR from, WORD packetType, WORD payloadSize, ULONGLONG qwExtra, const uint8_t *payloadData);

bool bHooked = false;
ULONGLONG nextHookAttempt = 0;

//PMQPLUGIN FindPlugin(PCHAR PluginName)
//{
//	long Length = strlen(PluginName) + 1;
//	PMQPLUGIN pLook = pPlugins;
//	while (pLook && _strnicmp(PluginName, pLook->szFilename, Length))
//		pLook = pLook->pNext;
//	return pLook;
//}

bool EQBCConnected()
{
	typedef WORD(__cdecl *fEQBCGetConnectionStatus)(VOID);
	if (HMODULE hMod = GetModuleHandle("mq2eqbc"))
	{
		if (fEQBCGetConnectionStatus GetConnectionStatus = (fEQBCGetConnectionStatus)GetProcAddress(hMod, "isConnected"))
		{
			if (GetConnectionStatus()) return true;
		}
	}
	return false;
}

bool EQBCHooked()
{
	return bHooked;
}

void EQBCTransmit(char* szMsg)
{
	if (HMODULE hMod = GetModuleHandle("mq2eqbc"))
	{
		void* instance = (void*)(DWORD(hMod) + OFFSET_EQBC_INSTANCE);
		if (instance)
		{
			fEQBCTransmit RawEQBCTransmit = (fEQBCTransmit)(DWORD(hMod) + OFFSET_TRANSMIT_FUNC);
			if (RawEQBCTransmit)
			{
				RawEQBCTransmit(instance, true, szMsg);
			}
		}
	}
}

bool HookNetBotsCallback()
{
	if (bHooked)
		return true;
	//bHooked = false;
	//hook the callback
	if (HMODULE hMod = GetModuleHandle("mq2eqbc"))
	{
		PCHAR szNetBots = (PCHAR)(DWORD(hMod) + OFFSET_NETBOTS_STRING);
		if (szNetBots && !_stricmp(szNetBots, "mq2netbots"))
		{
			DebugSpewAlways("::MQ2BOT:: Hijacking mq2netbots callback");
			DWORD oldperm, tmp, bytesWritten;
			bool rc;
			char szBuff[11] = "mq2bot";
			VirtualProtectEx(GetCurrentProcess(), (LPVOID)szNetBots, 11, PAGE_EXECUTE_READWRITE, &oldperm);
			rc = WriteProcessMemory(GetCurrentProcess(), (LPVOID)szNetBots, (LPVOID)&szBuff, 11, &bytesWritten);
			VirtualProtectEx(GetCurrentProcess(), (LPVOID)szNetBots, 11, oldperm, &tmp);
			bHooked = (rc && bytesWritten > 0);
			return bHooked;
		}
	}
	return false;
}

bool UnhookNetBotsCallback()
{
	if (!bHooked)
		return true;
	//bHooked will be false no matter, so we stop trying to process messages
	//the return of this function will be whether unhooking was a success,
	//for cleanup purposes
	bHooked = false;
	//unhook the callback
	if (HMODULE hMod = GetModuleHandle("mq2eqbc"))
	{
		PCHAR szNetBots = (PCHAR)(DWORD(hMod) + OFFSET_NETBOTS_STRING);
		if (szNetBots && !_stricmp(szNetBots, "mq2bot"))
		{
			DebugSpewAlways("::MQ2BOT:: Removing hijack on mq2netbots callback");
			DWORD oldperm, tmp, bytesWritten;
			bool rc;
			char szBuff[11] = "mq2netbots";
			VirtualProtectEx(GetCurrentProcess(), (LPVOID)szNetBots, 11, PAGE_EXECUTE_READWRITE, &oldperm);
			rc = WriteProcessMemory(GetCurrentProcess(), (LPVOID)szNetBots, (LPVOID)&szBuff, 11, &bytesWritten);
			VirtualProtectEx(GetCurrentProcess(), (LPVOID)szNetBots, 11, oldperm, &tmp);
			return (rc && bytesWritten > 0);
		}
	}
	return false;
}

typedef struct _PACKET_HEADER
{
	WORD wPacketType;
	WORD wPayloadSize;
	ULONGLONG qwExtra;
} BotPacketHeader, *PBotPacketHeader;

//let's limit the max packet size for now, since eqbc chokes if it gets too high
#define BOT_MAX_PACKET_SIZE 768
void EQBCSendPacket(WORD packetType, WORD payloadSize, ULONGLONG qwExtra, LPCVOID payloadData)
{
	//send packets when not in game?
	if (gGameState != GAMESTATE_INGAME)
		return;
	if (payloadSize > BOT_MAX_PACKET_SIZE)
	{
		WriteChatf("\ayNetAPI: Payload exceeds the maximum packet size!");
		return;
	}
	if (EQBCConnected())
	{
		constexpr auto bufsize = 1024; //this is not the payload size
		char szBuffer1[bufsize] = { 0 };
		char szBuffer2[961] = { 0 };
		char szTemp[32] = { 0 };
		BotPacketHeader hdr = { 0 };

		//source size needs to be divisible by four, so pad if not
		DWORD srcSize = payloadSize;
		if (srcSize % 4)
			srcSize += 4 - (srcSize % 4);

		//dest size needs to be 5/4ths src, + 5 (for header), + 1 for null term
		//DWORD dstSize = ((srcSize * 5) / 4) + 5 + 1; //skip this for now

		//build the packet
		hdr.wPacketType = packetType;
		hdr.wPayloadSize = payloadSize;
		hdr.qwExtra = qwExtra;
		zmq_z85_encode(szTemp, (byte*)&hdr, sizeof(hdr));

		zmq_z85_encode(szBuffer2, (byte*)payloadData, srcSize);

		strcpy_s(szBuffer1, bufsize, "[NB]|");
		strcat_s(szBuffer1, bufsize, szTemp);
		strcat_s(szBuffer1, bufsize, "|");
		strcat_s(szBuffer1, bufsize, szBuffer2);
		strcat_s(szBuffer1, bufsize, "|[NB]");

		strcpy_s(szTemp, 7, "\tNBMSG");
		EQBCTransmit(szTemp);
		EQBCTransmit(szBuffer1);
	}
}

PLUGIN_API VOID OnNetBotMSG(PCHAR Name, PCHAR Msg) {
	DebugSpewAlways("::NETBOT_MSG:: (%s) %s", Name, Msg);

	if (strlen(Msg) > 1023)
	{
		WriteChatf("\ayNetAPI: Message appears malformed. Size is greater than 1023 bytes.");
		return;
	}

	//we don't want to report messages we sent do we?
	//disable for now for testing
	//if (GetCharInfo() && !_stricmp(Name, GetCharInfo()->Name))
	//	return;

	constexpr auto bufsize = 1024; //this is not the payload size
	char szBuffer1[bufsize] = { 0 };
	byte szBuffer2[bufsize] = { 0 };
	BotPacketHeader hdr = { 0 };

	//char *pStart = Msg;
	//char *pHdr = strtok(Msg, "|");
	//*pHdr = '\0';
	//pHdr++;
	//char *pData = strtok(pHdr, "|");
	//*pData = '\0';
	//pData++;
	//char *pEnd = strtok(pData, "|");
	//*pEnd = '\0';
	//pEnd++;
	//if (!_stricmp(pStart, "[NB]") && !_stricmp(pEnd, "[NB]"))
	//{
	//	//message looks valid
	//}

	//copy Msg
	strcpy_s(szBuffer1, bufsize, Msg);
	
	char *pNextToken = NULL;
	char *p = strtok_s(szBuffer1, "|", &pNextToken);
	if (!_stricmp(p, "[NB]"))
	{
		//header
		p = strtok_s(NULL, "|", &pNextToken);
		zmq_z85_decode((byte*)&hdr, p);

		//payload
		p = strtok_s(NULL, "|", &pNextToken);
		//was using dynamic buffer, but crashed from time to time, will revisit later
		//byte *dataBuffer;
		//dataBuffer = new byte[hdr.wPayloadSize]; //dynamic memory, don't forget to free
		zmq_z85_decode((byte*)&szBuffer2, p);

		p = strtok_s(NULL, "|", &pNextToken);
		if (!_stricmp(p, "[NB]"))
		{
			//closing [NB] tag, message looks good
			OnIncomingBotPacket(Name, hdr.wPacketType, hdr.wPayloadSize, hdr.qwExtra, (byte*)&szBuffer2);
		}
		else { DebugSpewAlways("Invalid msg? p = %s", p); }

		//free memory
		//delete[] dataBuffer;

	}
}

//void EQBCBroadCast(PCHAR Buffer) {
//	typedef VOID(__cdecl *fEqbcNetBotSendMsg)(PCHAR);
//	if (strlen(Buffer) > 9) {
//		PMQPLUGIN pLook = pPlugins;
//		while (pLook && _strnicmp(pLook->szFilename, "mq2eqbc", 8)) pLook = pLook->pNext;
//		if (pLook)
//			if (fEqbcNetBotSendMsg requestf = (fEqbcNetBotSendMsg)GetProcAddress(pLook->hModule, "NetBotSendMsg")) {
//#if    DEBUGGING>1
//				DebugSpewAlways("%s->BroadCasting(%s)", PLUGIN_NAME, Buffer);
//#endif DEBUGGING
//				requestf(Buffer);
//			}
//	}
//}