#pragma once

#include <windows.h>

#pragma comment(lib, "Synchronization.lib")

// Lock that uses WaitOnAddress
class WaitLock {
public:
	WaitLock() = default;
	WaitLock(const WaitLock&) = delete;
	WaitLock& operator=(const WaitLock&) = delete;

	enum LockState : LONG {
		OPEN	= 0,
		LOCKED	= 1,
	};

	bool TryLock() {
		return InterlockedCompareExchangeAcquire(&lock, LOCKED, OPEN) == OPEN;
	}

	void Lock() {
		LONG ucCmp = LOCKED;
		while (true) {
			if (TryLock())
				break;

			WaitOnAddress(&lock, &ucCmp, sizeof(lock), INFINITE);
		}
	}

	void Unlock() {
		const LONG prev = InterlockedExchange(&lock, OPEN);
		if (prev == LOCKED)
			WakeByAddressSingle(const_cast<LONG*>(&lock));
	}

private:
	alignas(4) volatile LONG lock = OPEN;
};

class WaitLockScope {
public:
	WaitLockScope(WaitLock* apLock) : pLock(apLock)	 { pLock->Lock(); };
	WaitLockScope(WaitLock& arLock) : pLock(&arLock) { pLock->Lock(); };
	~WaitLockScope() { pLock->Unlock(); };

protected:
	WaitLock* pLock = nullptr;
};