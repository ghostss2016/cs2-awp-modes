#ifndef _CS2_AWP_MODES_EVENTS_H_
#define _CS2_AWP_MODES_EVENTS_H_

#include "main.h"
#include <igameevents.h>

// Event listener class (matching K4-Arenas PluginEvents.cs)
class ArenaEventListener : public IGameEventListener2
{
public:
    ArenaEventListener();
    virtual ~ArenaEventListener();

    // IGameEventListener2 interface
    virtual void FireGameEvent(IGameEvent* event) override;

    // Register/unregister events
    void RegisterEvents();
    void UnregisterEvents();

private:
    // Event handlers
    void OnRoundPrestart(IGameEvent* event);
    void OnRoundStart(IGameEvent* event);
    void OnRoundEnd(IGameEvent* event);
    void OnRoundFreezeEnd(IGameEvent* event);
    void OnPlayerSpawn(IGameEvent* event);
    void OnPlayerDeath(IGameEvent* event);
    void OnPlayerHurt(IGameEvent* event);
    void OnPlayerBlind(IGameEvent* event);
    void OnPlayerDisconnect(IGameEvent* event);
    void OnPlayerTeam(IGameEvent* event);

    bool m_bRegistered;
};

// Global event listener
extern ArenaEventListener* g_pEventListener;

// Initialize event system
void InitializeEvents();
void ShutdownEvents();

// Hook handlers
void OnGameFrame(bool simulating, bool bFirstTick, bool bLastTick);
void OnClientPutInServer(int slot);
void OnClientDisconnect(int slot);

#endif // _CS2_AWP_MODES_EVENTS_H_
