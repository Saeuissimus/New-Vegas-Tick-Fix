#pragma once

#include "internal/Common/CommonUtils.hpp"
#include <vector>
#include "nvse/PluginAPI.h"
#include <algorithm>

class SRWSharedLock {
public:
	SRWSharedLock(PSRWLOCK apLock) : pLock(apLock) { AcquireSRWLockShared(pLock); }
	SRWSharedLock(SRWLOCK& arLock) : pLock(&arLock) { AcquireSRWLockShared(pLock); }
	~SRWSharedLock() { ReleaseSRWLockShared(pLock); }
private:
	PSRWLOCK pLock;
};

class SRWUniqueLock {
public:
	SRWUniqueLock(PSRWLOCK apLock) : pLock(apLock) { AcquireSRWLockExclusive(pLock); }
	SRWUniqueLock(SRWLOCK& arLock) : pLock(&arLock) { AcquireSRWLockExclusive(pLock); }
	~SRWUniqueLock() { ReleaseSRWLockExclusive(pLock); }
private:
	PSRWLOCK pLock;
};

namespace CommonUtils
{
	class UtilsManager
	{
	private:
		std::vector<bool(__cdecl*)()> m_Callbacks;
		SRWLOCK srwlock = SRWLOCK_INIT;

	public:
		bool RegisterMainLoopFunction(bool (__cdecl *pToRegister)()) {
			SRWUniqueLock lock(srwlock);
			auto it = std::find(m_Callbacks.begin(), m_Callbacks.end(), pToRegister);
			if (it == m_Callbacks.end())
				m_Callbacks.push_back(pToRegister);
			return true;
		};
		bool UnregisterMainLoopFunction(bool (__cdecl *pToRegister)()) {
			SRWUniqueLock lock(srwlock);
			m_Callbacks.erase(std::remove(m_Callbacks.begin(), m_Callbacks.end(), pToRegister), m_Callbacks.end());
			return true;
		};

		std::vector<bool(__cdecl*)()> copyCallbacks() {
			SRWSharedLock lock(srwlock);
			return m_Callbacks;
		}
	};

	inline UtilsManager* GetUtilsManager()
	{
		static UtilsManager g_Utils;
		return &g_Utils;
	}

	inline void NVSEMessageHandler(void* receivedMsg)
	{
		auto msg = static_cast<NVSEMessagingInterface::Message*>(receivedMsg);
		switch (msg->type)
		{
		case NVSEMessagingInterface::kMessage_MainGameLoop:
			{
				auto lUtilsMan = GetUtilsManager();
				auto copiedCallbacks = lUtilsMan->copyCallbacks();

				for (auto callback : copiedCallbacks)
				{
					callback();
				}
			}
			break;
		default:
			break;
		}
	}
}