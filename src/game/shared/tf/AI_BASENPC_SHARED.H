#ifndef AI_BASENPC_SHARED_H
#define AI_BASENPC_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"
#include "SpriteTrail.h"
#include "tf_condition.h"

// Client specific.
#ifdef CLIENT_DLL
#include "baseobject_shared.h"
#include "c_tf_weapon_builder.h"
class C_AI_BaseNPC;
// Server specific.
#else
#include "entity_currencypack.h"
#include "tf_weapon_builder.h"
class CAI_BaseNPC;
#endif

#define PERMANENT_CONDITION_NPC		-1

class AI_BaseNPC_Shared : public CGameEventListener
{
public:

	// Client specific.
#ifdef CLIENT_DLL

	friend class C_AI_BaseNPC;
	typedef C_AI_BaseNPC OuterClass;
	DECLARE_PREDICTABLE();

	// Server specific.
#else

	friend class CAI_BaseNPC;
	typedef CAI_BaseNPC OuterClass;

#endif

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE(AI_BaseNPC_Shared);

	int		GetNumHealers(void) { return m_nNumHealers; }

private:
	CNetworkVar(int, m_nNumHealers);
	CUtlVector< condition_source_t > m_ConditionData;
	// Vars that are not networked.
	OuterClass* m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).
	CNetworkVar(int, m_nPlayerState);			// Player state.
	CNetworkVar(int, m_nPlayerCond);			// Player condition flags.
	CNetworkVar(int, m_nPlayerCondEx);		// Player condition flags (extended -- we overflowed 32 bits).
	CNetworkVar(int, m_nPlayerCondEx2);		// Player condition flags (extended -- we overflowed 64 bits).
	CNetworkVar(int, m_nPlayerCondEx3);		// Player condition flags (extended -- we overflowed 96 bits).
	CNetworkVar(int, m_nPlayerCondEx4);		// Player condition flags (extended -- we overflowed 128 bits).

	CNetworkVarEmbedded(NPCConditionList, m_ConditionList);
public:
#ifdef CLIENT_DLL
	enum ECritBoostUpdateType { kCritBoost_Ignore, kCritBoost_ForceRefresh };
	void	UpdateCritBoostEffect(ECritBoostUpdateType eUpdateType = kCritBoost_Ignore);
#endif
	bool	IsCritBoosted(void) const;
	void	FireGameEvent(IGameEvent* event);
	void Init(OuterClass* pOuter);
	// Condition (TF_COND_*).
	void	AddCond(ETFCond eCond, float flDuration = PERMANENT_CONDITION_NPC, CBaseEntity* pProvider = NULL);
	void	RemoveCond(ETFCond eCond, bool ignore_duration = false);
	bool	InCond(ETFCond eCond) const;
	bool	WasInCond(ETFCond eCond) const;
	void	ForceRecondNextSync(ETFCond eCond);
	void	RemoveAllCond();
	void	OnConditionAdded(ETFCond eCond);
	void	OnConditionRemoved(ETFCond eCond);
	void	ConditionThink(void);
	float	GetConditionDuration(ETFCond eCond) const;
	void	SetConditionDuration(ETFCond eCond, float flNewDur)
	{
		Assert(eCond < m_ConditionData.Count());
		m_ConditionData[eCond].m_flExpireTime = flNewDur;
	}

};

#endif // AI_BASENPC_SHARED_H