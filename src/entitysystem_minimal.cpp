/*
 * Minimal CEntitySystem::GetEntityIdentity implementations.
 * Only the two overloads our plugin needs, avoiding the full
 * entitysystem.cpp which pulls in many unresolved dependencies.
 *
 * IMPORTANT: All functions here use __attribute__((visibility("hidden")))
 * to prevent ODR (One Definition Rule) violations when multiple plugins
 * define the same symbols (e.g. skybox.so and cs2-aimesp.so both define
 * CEntitySystem::GetEntityIdentity).
 */
#include "entity2/entitysystem.h"
#include <stdint.h>

// Make symbols local to this shared object to avoid ODR conflicts
#define HIDDEN_FUNC __attribute__((visibility("hidden")))

// Use ISmmPlugin for logging (defined in main.cpp)
#include <ISmmPlugin.h>
extern ISmmAPI* g_SMAPI;

// Validate EntitySystem 'this' pointer is in valid memory range
static inline bool IsValidEntitySystem(const void* ptr)
{
	if (!ptr)
		return false;
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	// Must be 8-byte aligned and in valid Linux heap/data range
	if (addr & 0x7)
		return false;
	if (addr < 0x100000000ULL || addr > 0x7FFFFFFFFFFFULL)
		return false;
	return true;
}

HIDDEN_FUNC CEntityIdentity* CEntitySystem::GetEntityIdentity(CEntityIndex entnum)
{
	// Validate 'this' pointer before any member access
	if (!IsValidEntitySystem(this))
	{
		// Don't spam - this is a critical error
		static int s_nWarnCount = 0;
		if (s_nWarnCount++ < 5 && g_SMAPI)
			g_SMAPI->ConPrintf("[AimESP] CRITICAL: GetEntityIdentity(CEntityIndex) called with invalid this=%p\n", this);
		return nullptr;
	}

	if (entnum.Get() <= -1 || entnum.Get() >= (MAX_TOTAL_ENTITIES - 1))
		return nullptr;

	// Safety check: validate m_pIdentityChunks array exists
	int chunkIndex = entnum.Get() / MAX_ENTITIES_IN_LIST;
	if (chunkIndex < 0 || chunkIndex >= 64)  // Max 64 chunks
		return nullptr;

	// Check if chunks array pointer is accessible
	if (!m_EntityList.m_pIdentityChunks)
		return nullptr;

	CEntityIdentity* pChunkToUse = m_EntityList.m_pIdentityChunks[chunkIndex];
	if (!pChunkToUse)
		return nullptr;

	CEntityIdentity* pIdentity = &pChunkToUse[entnum.Get() % MAX_ENTITIES_IN_LIST];
	if (!pIdentity)
		return nullptr;

	if (pIdentity->GetEntityIndex() != entnum)
		return nullptr;

	return pIdentity;
}

HIDDEN_FUNC CEntityIdentity* CEntitySystem::GetEntityIdentity(const CEntityHandle& hEnt)
{
	// Validate 'this' pointer before any member access
	if (!IsValidEntitySystem(this))
	{
		static int s_nWarnCount2 = 0;
		if (s_nWarnCount2++ < 5 && g_SMAPI)
			g_SMAPI->ConPrintf("[AimESP] CRITICAL: GetEntityIdentity(CEntityHandle) called with invalid this=%p\n", this);
		return nullptr;
	}

	if (!hEnt.IsValid())
		return nullptr;

	// Safety check: validate m_pIdentityChunks array exists
	int chunkIndex = hEnt.GetEntryIndex() / MAX_ENTITIES_IN_LIST;
	if (chunkIndex < 0 || chunkIndex >= 64)  // Max 64 chunks
		return nullptr;

	// Check if chunks array pointer is accessible
	if (!m_EntityList.m_pIdentityChunks)
		return nullptr;

	CEntityIdentity* pChunkToUse = m_EntityList.m_pIdentityChunks[chunkIndex];
	if (!pChunkToUse)
		return nullptr;

	CEntityIdentity* pIdentity = &pChunkToUse[hEnt.GetEntryIndex() % MAX_ENTITIES_IN_LIST];
	if (!pIdentity)
		return nullptr;

	if (pIdentity->GetRefEHandle() != hEnt)
		return nullptr;

	return pIdentity;
}
