// SPDX-License-Identifier: GPL-3.0-only
#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"

class CBasePlayer {};

#if defined __WIN32__ || defined _WIN32 || defined WIN32
typedef  void* (__thiscall *tCBaseEntity__SetAbsOrigin)(CBaseEntity*,Vector  const*);
#else

typedef void *( *tCBaseEntity__SetAbsOrigin)(CBaseEntity *, const Vector *);

#endif

class DefibFix : public SDKExtension
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);

	virtual void SDK_OnUnload();

public:
#if defined SMEXT_CONF_METAMOD
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late);
#endif
};

#endif
