extern cvar_t *use_grapple;

typedef enum {
	CTF_GRAPPLE_STATE_FLY,
	CTF_GRAPPLE_STATE_PULL,
	CTF_GRAPPLE_STATE_HANG
} ctfgrapplestate_t;

#define CTF_GRAPPLE_SPEED		650 // speed of grapple in flight by default
#define CTF_GRAPPLE_PULL_SPEED	650 // speed player is pulled at by default
#define TP_GRAPPLE_SPEED		950 // speed of grapple in flight in teamplay
#define TP_GRAPPLE_PULL_SPEED	650 // speed of grapple in flight in teamplay

// GRAPPLE
void CTFWeapon_Grapple (edict_t *ent);
void CTFWeapon_Grapple_Fire(edict_t* ent);
void CTFPlayerResetGrapple(edict_t *ent);
void CTFGrapplePull(edict_t *self);
void CTFResetGrapple(edict_t *self);
