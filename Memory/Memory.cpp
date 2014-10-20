// Memory.cpp : Defines the entry point for the console application.
//

#include "tchar.h"
#include "windows.h"
#include "crtdbg.h"
#include "stdio.h"

namespace Memory
{
	LONG __GetPageSize__() {
		SYSTEM_INFO si = { 0 };
		GetSystemInfo(&si);
		return si.dwPageSize;
	}

	const DWORD PAGE_SIZE = __GetPageSize__();

	struct __SAFE_HEAP_POINTER {
		PBYTE	pAddr;
		ULONG	ulSize;
	};

	ULONG __cdecl _msize(IN void* pPtr) {
		PBYTE pVirtualAddr = (PBYTE)pPtr;
		__SAFE_HEAP_POINTER* pSafePtr = (__SAFE_HEAP_POINTER*)(pVirtualAddr - sizeof(__SAFE_HEAP_POINTER));
		ULONG_PTR			 rvaOld		= (ULONG_PTR)(pSafePtr + 1) - (ULONG_PTR)pSafePtr->pAddr;
		return (ULONG)(pSafePtr->ulSize - PAGE_SIZE - rvaOld);
	}

	void __cdecl free(IN void* pPtr) {
		if(0 == pPtr)
			return;
		//return free(pPtr);
		PBYTE pVirtualAddr = (PBYTE)pPtr;
		__SAFE_HEAP_POINTER* pSafePtr = (__SAFE_HEAP_POINTER*)(pVirtualAddr - sizeof(__SAFE_HEAP_POINTER));
		ULONG ulOldProtect;
		if(FALSE == VirtualProtect(pSafePtr->pAddr + pSafePtr->ulSize - PAGE_SIZE, PAGE_SIZE, PAGE_READWRITE, &ulOldProtect)) {
			// This should never really happen
			_ASSERT_EXPR(FALSE, L"Memory::free, VirtualProtect failed");
		#ifndef _DEBUG
			DebugBreak();		
		#endif
		}
		_aligned_free(pSafePtr->pAddr);
	}

	void* __cdecl malloc(IN size_t dwBytes) {
		DWORD dwTotalBytes			= dwBytes + sizeof(__SAFE_HEAP_POINTER);
		DWORD dwPages				= (dwTotalBytes / PAGE_SIZE) + 1;
		DWORD dwAlignedBytesCount	= (dwPages + 1) * PAGE_SIZE;
		PBYTE pPtr					= (PBYTE)_aligned_malloc(dwAlignedBytesCount, PAGE_SIZE);
		if(0 == pPtr)
			return 0;
		ZeroMemory(pPtr, dwAlignedBytesCount);
		PBYTE pLastPageStart = pPtr + dwPages * PAGE_SIZE;
		ULONG ulOldProtect;
		PBYTE pBlock = (PBYTE)(pLastPageStart - dwBytes);
		if(FALSE == VirtualProtect(pLastPageStart, PAGE_SIZE, PAGE_READWRITE | PAGE_GUARD, &ulOldProtect)) {
			_ASSERT(FALSE);
			_aligned_free(pPtr);
			return 0;
		}
		__SAFE_HEAP_POINTER* pSafePtr = (__SAFE_HEAP_POINTER*)(pBlock - sizeof(__SAFE_HEAP_POINTER));
		pSafePtr->pAddr		= pPtr;
		pSafePtr->ulSize	= dwAlignedBytesCount;
		return pBlock;
	}

	void* __cdecl calloc(IN size_t dwElements, IN size_t dwElementSize) {
		PVOID pPtr = malloc(dwElements * dwElementSize);
		if(0 != pPtr)
			ZeroMemory(pPtr, dwElements * dwElementSize);
		return pPtr;
	}

	void* __cdecl realloc(IN void* pPtr, IN size_t dwBytes) {
		if(0 == pPtr)
			return malloc(dwBytes);
		PBYTE pVirtualAddr = (PBYTE)pPtr;
		__SAFE_HEAP_POINTER* pSafePtr	= (__SAFE_HEAP_POINTER*)(pVirtualAddr - sizeof(__SAFE_HEAP_POINTER));
		__SAFE_HEAP_POINTER  oldPtr		= *pSafePtr;
		ULONG				 ulPrevSize	= _msize(pPtr);
		if(ulPrevSize == dwBytes)
			return pPtr;

		// Start working on the addresses
		DWORD dwTotalBytes			= dwBytes + sizeof(__SAFE_HEAP_POINTER);
		DWORD dwNewPages			= (dwTotalBytes / PAGE_SIZE) + 1;
		DWORD dwAlignedBytesCount	= (dwNewPages + 1) * PAGE_SIZE;
		PBYTE pBlock				= 0;
		PBYTE pLastPageStart		= 0;
		if((dwAlignedBytesCount <= oldPtr.ulSize) && (dwAlignedBytesCount + PAGE_SIZE >= oldPtr.ulSize)) {
			// No need to reallocate memory, the allocated pages R enough
			pLastPageStart	= pSafePtr->pAddr + dwNewPages * PAGE_SIZE;
			pBlock			= (pLastPageStart - dwBytes);
			MoveMemory(pBlock, pPtr, min(ulPrevSize, dwBytes));
			pSafePtr = (__SAFE_HEAP_POINTER*)(pBlock - sizeof(__SAFE_HEAP_POINTER));
			*pSafePtr= oldPtr;
			return pBlock;
		}

		// Buffer was enlarged or reduced by more than PAGE_SIZE
		PBYTE pNew = (PBYTE)malloc(dwBytes);
		CopyMemory(pNew, pPtr, min(ulPrevSize, dwBytes));
		free(pPtr);
		return pNew;
	}
};

inline void *__cdecl operator new(size_t _Size) {
	return Memory::malloc(_Size);
}

inline void *__cdecl operator new[](size_t _Size) {
	return Memory::malloc(_Size);
}

inline void __cdecl operator delete(void *p) {
	Memory::free(p);
}

inline void __cdecl operator delete[](void *p) {
	Memory::free(p);
}


class CTest
{
public:
	CTest() { printf("CTest::CTest()\r\n"); }
	~CTest() { printf("CTest::~CTest()\r\n"); }
};

int _tmain(int argc, _TCHAR* argv[])
{
	// Sample usage of the overloaded operators
	CTest* pTest = new CTest();
	delete pTest;
	char* pPtr = new char[Memory::PAGE_SIZE * 2 + 1];
	delete [] pPtr;

	pPtr = (char*)Memory::malloc(Memory::PAGE_SIZE * 2 + 1);
	memset(pPtr, 1, Memory::_msize(pPtr));
	pPtr = (char*)Memory::realloc(pPtr, 500);
	pPtr[500] = 0;// This is where the memory overrun happen, will raise a PAGE_GUARD exception that brings in the debugger
	memset(pPtr, 2, Memory::_msize(pPtr));
	pPtr = (char*)Memory::realloc(pPtr, 1000);
	memset(pPtr, 3, Memory::_msize(pPtr));
	Memory::free(pPtr);
	return 0;
}

