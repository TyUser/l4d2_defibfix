#include "extension.h"
#include "CDetour/detours.h"
#include <iplayerinfo.h>

#define GAMEDATA_FILE "defibfix"

CDetour *hg_getPlayer  = NULL;
CDetour *hg_defibStart = NULL;
CDetour *hg_defibEnd   = NULL;
CDetour *hg_deadPlayer = NULL;

tCBaseEntity__SetAbsOrigin CBaseEntity__SetAbsOrigin;

IGameConfig *g_pGameConf = NULL;
IServerGameEnts *gameents = NULL;

DefibFix g_DefibFix;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_DefibFix);

int ig_defib;
int ig_noob;

/**
*	return -1           // Мертвый игрок не найден.
*	return (от 1 до 32) // Номер мертвого игрока.
**/
int HxPlayerDead()
{
	int iDead1 = -1;
	int iDead2 = -1;
	int iDead3 = -1;
	int i = 1;

	while (i <= 32)
	{
		IGamePlayer *player = playerhelpers->GetGamePlayer(i);
		if (player)
		{
			if (player->IsInGame())
			{
				IPlayerInfo *info = player->GetPlayerInfo();
				if (info)
				{
					if (info->GetTeamIndex() == 2)
					{
						if (info->IsObserver())
						{
							g_pSM->LogMessage(myself, "Dead %d [%s]", i, info->GetName());

							if (info->IsFakeClient())
							{
								iDead3 = i;
							}
							else
							{
								iDead2 = i;
								if (ig_noob != i)
								{
									iDead1 = i;
								}
							}
						}

						if (info->IsDead())
						{
							g_pSM->LogMessage(myself, "Dead %d [%s]", i, info->GetName());

							if (info->IsFakeClient())
							{
								iDead3 = i;
							}
							else
							{
								iDead2 = i;
								if (ig_noob != i)
								{
									iDead1 = i;
								}
							}
						}
					}
				}
			}
		}

		i += 1;
	}

	if (iDead1 > 0)
	{
		g_pSM->LogMessage(myself,"Выбран %d", iDead1);
		return iDead1;
	}

	if (iDead2 > 0)
	{
		g_pSM->LogMessage(myself,"Выбран %d", iDead2);
		return iDead2;
	}

	g_pSM->LogMessage(myself,"Выбран %d", iDead3);
	return iDead3;
}

DETOUR_DECL_STATIC1(GetPlayerByCharacter, void *, int, charaster)
{
	if (ig_defib)
	{
		ig_defib = 0;
		int iDead = HxPlayerDead();
		if (iDead > 0)
		{
			ig_noob = iDead;
			g_pSM->LogMessage(myself,"Воскрешается %d", iDead);
			return gamehelpers->ReferenceToEntity(iDead);
		}

		g_pSM->LogMessage(myself,"Ошибка");
		return 0;
	}

	return DETOUR_STATIC_CALL(GetPlayerByCharacter)(charaster);
}

DETOUR_DECL_MEMBER4(DefibrillatorOnStartAction, void *, int,reserved, void*,player, void*,entity, int,reserved2)
{
	edict_t *edict=gameents->BaseEntityToEdict((CBaseEntity*)entity);

	if (edict)
	{
		g_pSM->LogMessage(myself,"Начало дефибрилляции");
		if (HxPlayerDead() == -1)
		{
			g_pSM->LogMessage(myself,"Труп удаляется");
			engine->RemoveEdict(edict);
		}
	}

	return DETOUR_MEMBER_CALL(DefibrillatorOnStartAction)(reserved,player,entity,reserved2);
}

DETOUR_DECL_MEMBER2(DefibrillatorOnActionComplete, void *, void*, player, void*, entity)
{
	edict_t *edict=gameents->BaseEntityToEdict((CBaseEntity*)entity);
	if (edict)
	{
		g_pSM->LogMessage(myself,"Окончание дефибрилляции");
		ig_defib = 1;
	}

	return DETOUR_MEMBER_CALL(DefibrillatorOnActionComplete)(player,entity);
}

DETOUR_DECL_STATIC1(CSurvivorDeathModel__Create, CBaseEntity *, CBasePlayer*, bplayer)
{
	edict_t *pEdict=gameents->BaseEntityToEdict((CBaseEntity*)bplayer);
	int client=gamehelpers->IndexOfEdict(pEdict);

	CBaseEntity * result = DETOUR_STATIC_CALL(CSurvivorDeathModel__Create)(bplayer);
	if (client > 0) 
	{
		Vector PlayerOrigin=pEdict->GetCollideable()->GetCollisionOrigin();
		CBaseEntity__SetAbsOrigin(result,&PlayerOrigin);
	}

	return result;
}

bool DefibFix::SDK_OnMetamodLoad( ISmmAPI *ismm, char *error, size_t maxlength, bool late )
{
	size_t maxlen=maxlength;
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);

	return true;
}

bool DefibFix::SDK_OnLoad( char *error, size_t maxlength, bool late )
{
	ig_defib = 0;
	ig_noob = 0;

	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		snprintf(error, maxlength, "Could not read defibfix.txt: %s", conf_error);
		return false;
	}

	if (!SetupHooks())
	{
		snprintf(error, maxlength, "Cannot SetupHooks or GetOffset.");
		return false;
	}

	return true;
}

void DefibFix::SDK_OnUnload()
{
	DefibFix::RemoveHooks();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool DefibFix::SetupHooks()
{
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	g_pGameConf->GetMemSig("CBaseEntity::SetAbsOrigin",(void **)&CBaseEntity__SetAbsOrigin);

	hg_getPlayer  = DETOUR_CREATE_STATIC(GetPlayerByCharacter, "GetPlayerByCharacter");
	hg_defibStart = DETOUR_CREATE_MEMBER(DefibrillatorOnStartAction, "DefibrillatorOnStartAction");
	hg_defibEnd   = DETOUR_CREATE_MEMBER(DefibrillatorOnActionComplete, "DefibrillatorOnActionComplete");
	hg_deadPlayer = DETOUR_CREATE_STATIC(CSurvivorDeathModel__Create, "CSurvivorDeathModel::Create");

	if ((hg_getPlayer != NULL)
		 && (hg_defibStart != NULL)
		 && (hg_defibEnd != NULL)
		 && (hg_deadPlayer!=NULL)
		 && (CBaseEntity__SetAbsOrigin!=NULL))
	{
		hg_getPlayer->EnableDetour();
		hg_defibStart->EnableDetour();
		hg_defibEnd->EnableDetour();
		hg_deadPlayer->EnableDetour();
	}
	else
	{
		RemoveHooks();
		return false;
	}

	return true;
}

void DefibFix::RemoveHooks()
{
	if (hg_getPlayer != NULL)
	{
		hg_getPlayer->Destroy();
		hg_getPlayer = NULL;
	}

	if (hg_defibStart != NULL)
	{
		hg_defibStart->Destroy();
		hg_defibStart = NULL;
	}

	if (hg_defibEnd != NULL)
	{
		hg_defibEnd->Destroy();
		hg_defibEnd = NULL;
	}

	if (hg_deadPlayer != NULL)
	{
		hg_deadPlayer->Destroy();
		hg_deadPlayer = NULL;
	}
}
