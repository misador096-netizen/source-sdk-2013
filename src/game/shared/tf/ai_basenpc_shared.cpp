#include "cbase.h"
#include "tf_gamerules.h"
#include "ai_basenpc_shared.h"
#ifndef CLIENT_DLL
#include "ai_basenpc.h"
#endif
#ifdef CLIENT_DLL
#include "c_ai_basenpc.h"
#endif
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_invis.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_shovel.h"
#include "tf_weapon_sword.h"
#include "tf_weapon_shotgun.h"
#include "in_buttons.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_flaregun.h"
#include "tf_weapon_wrench.h"
#include "econ_wearable.h"
#include "econ_item_system.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_syringegun.h"
#include "tf_weapon_flamethrower.h"
#include "econ_entity_creation.h"
#include "tf_mapinfo.h"
#include "tf_dropped_weapon.h"
#include "tf_weapon_passtime_gun.h"
#include "tf_weapon_rocketpack.h"
#include <functional>

// Client specific.
#ifdef CLIENT_DLL
#include "c_baseviewmodel.h"
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_fx.h"
#include "soundenvelope.h"
#include "c_tf_playerclass.h"
#include "iviewrender.h"
#include "prediction.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "c_tf_weapon_builder.h"
#include "dt_utlvector_recv.h"
#include "recvproxy.h"
#include "c_tf_weapon_builder.h"
#include "c_func_capture_zone.h"
#include "tf_hud_target_id.h"
#include "tempent.h"
#include "cam_thirdperson.h"
#include "vgui/IInput.h"

#define CTFPlayerClass C_TFPlayerClass
#define CCaptureZone C_CaptureZone
#define CRecipientFilter C_RecipientFilter

#include "c_tf_objective_resource.h"
#include "tf_weapon_buff_item.h"
#include "c_tf_passtime_logic.h"

// Server specific.
#else
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "util.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_playerclass.h"
#include "SpriteTrail.h"
#include "tf_weapon_builder.h"
#include "nav_mesh/tf_nav_area.h"
#include "nav_pathfind.h"
#include "tf_obj_dispenser.h"
#include "dt_utlvector_send.h"
#include "tf_item_wearable.h"
#include "NextBotManager.h"
#include "tf_weapon_builder.h"
#include "func_capture_zone.h"
#include "hl2orange.spa.h"
#include "bot/tf_bot.h"
#include "tf_objective_resource.h"
#include "halloween/tf_weapon_spellbook.h"
#include "tf_weapon_buff_item.h"
#include "tf_passtime_logic.h"
#include "tf_weapon_passtime_gun.h"
#include "entity_healthkit.h"
#include "halloween/merasmus/merasmus.h"
#include "tf_weapon_grapplinghook.h"
#include "tf_wearable_levelable_item.h"
#include "tf_weapon_rocketpack.h"
#include "tf_obj_sentrygun.h"
#include "func_respawnroom.h"
#endif

#include "tf_wearable_weapons.h"
#include "tf_weapon_bonesaw.h"

void AI_BaseNPC_Shared::Init(CAI_BaseNPC* pPlayer)
{
	m_pOuter = pPlayer;
}

template < typename tType >
class CConditionVars
{
public:
	template < typename t0, typename t1, typename t2, typename t3, typename t4 >
	CConditionVars(CAI_BaseNPC* pOuter, ETFCond eCond, t0& nPlayerCond, t1& nPlayerCondEx, t2& nPlayerCondEx2, t3& nPlayerCondEx3, t4& nPlayerCondEx4)
	{
		m_pOuter = pOuter;

		if (eCond >= 128)
		{
			Assert(eCond < 128 + 32);
			m_pnCondVar = (tType*)&nPlayerCondEx4;
			m_nCondBit = eCond - 128;
		}
		else if (eCond >= 96)
		{
			Assert(eCond < 96 + 32);
			m_pnCondVar = (tType*)&nPlayerCondEx3;
			m_nCondBit = eCond - 96;
		}
		else if (eCond >= 64)
		{
			Assert(eCond < (64 + 32));
			m_pnCondVar = (tType*)&nPlayerCondEx2;
			m_nCondBit = eCond - 64;
		}
		else if (eCond >= 32)
		{
			Assert(eCond < (32 + 32));
			m_pnCondVar = (tType*)&nPlayerCondEx;
			m_nCondBit = eCond - 32;
		}
		else
		{
			m_pnCondVar = (tType*)&nPlayerCond;
			m_nCondBit = eCond;
		}
	}

	const int& CondVar() const
	{
		return m_pnCondVar->m_Value;
	}

	int& CondVarForModify()
	{
		m_pOuter->NetworkStateChanged(m_pnCondVar);
		return m_pnCondVar->m_Value;
	}

	int CondBit() const
	{
		return 1 << m_nCondBit;
	}

private:
	CAI_BaseNPC* m_pOuter;
	tType* m_pnCondVar;
	int m_nCondBit;
};

#define CONDITION_VARS( name, cond ) \
CConditionVars< decltype( m_nPlayerCond ) > name( m_pOuter, cond, m_nPlayerCond, m_nPlayerCondEx, m_nPlayerCondEx2, m_nPlayerCondEx3, m_nPlayerCondEx4 )

void AI_BaseNPC_Shared::AddCond(ETFCond eCond, float flDuration /* = PERMANENT_CONDITION */, CBaseEntity* pProvider /*= NULL */)
{
	Assert(eCond >= 0 && eCond < TF_COND_LAST);
	Assert(eCond < m_ConditionData.Count());
	
	// If we're dead, don't take on any new conditions
	if (!m_pOuter || !m_pOuter->IsAlive())
	{
		return;
	}

#ifdef CLEINT_DLL
	if (m_pOuter->IsDormant())
	{
		return;
	}
#endif

	// sanity check to prevent servers from adding these conditions when they shouldn't
	if ((eCond == TF_COND_COMPETITIVE_WINNER) || (eCond == TF_COND_COMPETITIVE_LOSER))
	{
		if (TFGameRules() && !TFGameRules()->ShowMatchSummary())
			return;
	}

	// Which bitfield are we tracking this condition variable in? Which bit within
	// that variable will we track it as?
	CONDITION_VARS(cPlayerCond, eCond);

	// See if there is an object representation of the condition.
	bool bAddedToExternalConditionList = m_ConditionList.Add(eCond, flDuration, m_pOuter, pProvider);
	if (!bAddedToExternalConditionList)
	{
		// Set the condition bit for this condition.
		cPlayerCond.CondVarForModify() |= cPlayerCond.CondBit();

		// Flag for gamecode to query
		m_ConditionData[eCond].m_bPrevActive = (m_ConditionData[eCond].m_flExpireTime != 0.f) ? true : false;

		if (flDuration != PERMANENT_CONDITION)
		{
			// if our current condition is permanent or we're trying to set a new
			// time that's less our current time remaining, use our current time instead
			if ((m_ConditionData[eCond].m_flExpireTime == PERMANENT_CONDITION) ||
				(flDuration < m_ConditionData[eCond].m_flExpireTime))
			{
				flDuration = m_ConditionData[eCond].m_flExpireTime;
			}
		}

		m_ConditionData[eCond].m_flExpireTime = flDuration;
		m_ConditionData[eCond].m_pProvider = pProvider;
		m_ConditionData[eCond].m_nPreventedDamageFromCondition = 0;

	//	OnConditionAdded(eCond);
	}
}

bool AI_BaseNPC_Shared::IsCritBoosted(void) const
{
	bool bAllWeaponCritActive = (InCond(TF_COND_CRITBOOSTED) ||
		InCond(TF_COND_CRITBOOSTED_PUMPKIN) ||
		InCond(TF_COND_CRITBOOSTED_USER_BUFF) ||
#ifdef CLIENT_DLL
		InCond(TF_COND_CRITBOOSTED_DEMO_CHARGE) ||
#endif
		InCond(TF_COND_CRITBOOSTED_FIRST_BLOOD) ||
		InCond(TF_COND_CRITBOOSTED_BONUS_TIME) ||
		InCond(TF_COND_CRITBOOSTED_CTF_CAPTURE) ||
		InCond(TF_COND_CRITBOOSTED_ON_KILL) ||
		InCond(TF_COND_CRITBOOSTED_CARD_EFFECT) ||
		InCond(TF_COND_CRITBOOSTED_RUNE_TEMP));

	if (bAllWeaponCritActive)
		return true;


	CTFWeaponBase* pWeapon = dynamic_cast<CTFWeaponBase*>(m_pOuter->GetActiveWeapon());
	if (pWeapon)
	{
		if (InCond(TF_COND_CRITBOOSTED_RAGE_BUFF) && pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_PRIMARY)
		{
			// Only primary weapon can be crit boosted by pyro rage
			return true;
		}

		float flCritHealthPercent = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pWeapon, flCritHealthPercent, mult_crit_when_health_is_below_percent);

		if (flCritHealthPercent < 1.0f && m_pOuter->HealthFraction() < flCritHealthPercent)
		{
			return true;
		}
	}

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AI_BaseNPC_Shared::UpdateCritBoostEffect(ECritBoostUpdateType eUpdateType)
{
	bool bShouldDisplayCritBoostEffect = IsCritBoosted()
		|| InCond(TF_COND_ENERGY_BUFF)
		//|| IsHypeBuffed()
		|| InCond(TF_COND_SNIPERCHARGE_RAGE_BUFF);

	if (m_pOuter->GetActiveWeapon())
	{
		bShouldDisplayCritBoostEffect &= m_pOuter->GetActiveWeapon()->CanBeCritBoosted();
	}

	// Never show crit boost effects when stealthed
	bShouldDisplayCritBoostEffect &= !IsStealthed();

	// Never show crit boost effects when disguised unless we're the local player (so crits show on our viewmodel)
	if (!m_pOuter->IsLocalPlayer())
	{
		bShouldDisplayCritBoostEffect &= !InCond(TF_COND_DISGUISED);
	}

	// Remove our current crit-boosted effect if we're forcing a refresh (in which case we'll
	// regenerate an effect below) or if we aren't supposed to have an effect active.
	if (eUpdateType == kCritBoost_ForceRefresh || !bShouldDisplayCritBoostEffect)
	{
		if (m_pOuter->m_pCritBoostEffect)
		{
			Assert(m_pOuter->m_pCritBoostEffect->IsValid());
			if (m_pOuter->m_pCritBoostEffect->GetOwner())
			{
				m_pOuter->m_pCritBoostEffect->GetOwner()->ParticleProp()->StopEmissionAndDestroyImmediately(m_pOuter->m_pCritBoostEffect);
			}
			else
			{
				m_pOuter->m_pCritBoostEffect->StopEmission();
			}

			m_pOuter->m_pCritBoostEffect = NULL;
		}

#ifdef CLIENT_DLL
		if (m_pCritBoostSoundLoop)
		{
			CSoundEnvelopeController::GetController().SoundDestroy(m_pCritBoostSoundLoop);
			m_pCritBoostSoundLoop = NULL;
		}
#endif
	}

	// Should we have an active crit effect?
	if (bShouldDisplayCritBoostEffect)
	{
		CBaseEntity* pWeapon = NULL;
		// Use GetRenderedWeaponModel() instead?
		if (m_pOuter->IsLocalPlayer())
		{
			pWeapon = m_pOuter->GetViewModel(0);
		}
		else
		{
			// is this player an enemy?
			if (m_pOuter->GetTeamNumber() != GetLocalPlayerTeam())
			{
				// are they a cloaked spy? or disguised as someone who almost assuredly isn't also critboosted?
				if (IsStealthed() || InCond(TF_COND_STEALTHED_BLINK) || InCond(TF_COND_DISGUISED))
					return;
			}

			pWeapon = m_pOuter->GetActiveWeapon();
		}

		if (pWeapon)
		{
			if (!m_pOuter->m_pCritBoostEffect)
			{
				if (InCond(TF_COND_DISGUISED) && !m_pOuter->IsLocalPlayer() && m_pOuter->GetTeamNumber() != GetLocalPlayerTeam())
				{
					const char* pEffectName = (GetDisguiseTeam() == TF_TEAM_RED) ? "critgun_weaponmodel_red" : "critgun_weaponmodel_blu";
					m_pOuter->m_pCritBoostEffect = pWeapon->ParticleProp()->Create(pEffectName, PATTACH_ABSORIGIN_FOLLOW);
				}
				else
				{
					const char* pEffectName = (m_pOuter->GetTeamNumber() == TF_TEAM_RED) ? "critgun_weaponmodel_red" : "critgun_weaponmodel_blu";
					m_pOuter->m_pCritBoostEffect = pWeapon->ParticleProp()->Create(pEffectName, PATTACH_ABSORIGIN_FOLLOW);
				}

				if (m_pOuter->IsLocalPlayer())
				{
					if (m_pOuter->m_pCritBoostEffect)
					{
						ClientLeafSystem()->SetRenderGroup(m_pOuter->m_pCritBoostEffect->RenderHandle(), RENDER_GROUP_VIEW_MODEL_TRANSLUCENT);
					}
				}
			}
			else
			{
				m_pOuter->m_pCritBoostEffect->StartEmission();
			}

			Assert(m_pOuter->m_pCritBoostEffect->IsValid());
		}

#ifdef CLIENT_DLL
		if (m_pOuter->GetActiveTFWeapon() && !m_pCritBoostSoundLoop)
		{
			CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();
			CLocalPlayerFilter filter;
			m_pCritBoostSoundLoop = controller.SoundCreate(filter, m_pOuter->entindex(), "Weapon_General.CritPower");
			controller.Play(m_pCritBoostSoundLoop, 1.0, 100);
		}
#endif
	}
}
#endif