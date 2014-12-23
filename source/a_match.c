//-----------------------------------------------------------------------------
// Matchmode related code
//
// $Id: a_match.c,v 1.18 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_match.c,v $
// Revision 1.18  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.17  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.16  2003/02/10 02:12:25  ra
// Zcam fixes, kick crashbug in CTF fixed and some code cleanup.
//
// Revision 1.15  2002/03/28 12:10:11  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.14  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.13  2002/03/25 23:34:06  slicerdw
// Small tweak on var handling ( prevent overflows )
//
// Revision 1.12  2001/12/05 15:27:35  igor_rock
// improved my english (actual -> current :)
//
// Revision 1.11  2001/12/02 16:15:32  igor_rock
// added console messages (for the IRC-Bot) to matchmode
//
// Revision 1.10  2001/11/25 19:09:25  slicerdw
// Fixed Matchtime
//
// Revision 1.9  2001/09/28 15:44:29  ra
// Removing Bill Gate's fingers from a_match.c
//
// Revision 1.8  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.7  2001/06/16 16:47:06  deathwatch
// Matchmode Fixed
//
// Revision 1.6  2001/06/13 15:36:31  deathwatch
// Small fix
//
// Revision 1.5  2001/06/13 07:55:17  igor_rock
// Re-Added a_match.h and a_match.c
// Added CTF Header for a_ctf.h and a_ctf.c
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "a_match.h"

float matchtime = 0;

void SendScores(void)
{
	int mins, secs;

	mins = matchtime / 60;
	secs = (int)matchtime % 60;
	if(use_3teams->value) {
		gi.bprintf(PRINT_HIGH, "��������������������������������������������\n");
		gi.bprintf(PRINT_HIGH, " Team 1 Score - Team 2 Score - Team 3 Score\n");
		gi.bprintf(PRINT_HIGH, "    [%d]           [%d]           [%d]\n", teams[TEAM1].score, teams[TEAM2].score, teams[TEAM3].score);
		gi.bprintf(PRINT_HIGH, " Total Played Time: %d:%02d\n", mins, secs);
		gi.bprintf(PRINT_HIGH, "��������������������������������������������\n");
	} else {
		int team1score = 0, team2score = 0;

		if(ctf->value)
			GetCTFScores(&team1score, &team2score);
		else {
			team1score = teams[TEAM1].score;
			team2score = teams[TEAM2].score;
		}
		gi.bprintf(PRINT_HIGH, "�����������������������������\n");
		gi.bprintf(PRINT_HIGH, " Team 1 Score - Team 2 Score\n");
		gi.bprintf(PRINT_HIGH, "     [%d]           [%d]\n", team1score, team2score);
		gi.bprintf(PRINT_HIGH, " Total Played Time: %d:%02d\n", mins, secs);
		gi.bprintf(PRINT_HIGH, "�����������������������������\n");
	}
	gi.bprintf(PRINT_HIGH, "Match is over, waiting for next map, please vote a new one..\n");
}

void Cmd_Kill_f(edict_t * ent);	// Used for killing people when they sub off
void Cmd_Sub_f(edict_t * ent)
{
	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	if (ent->client->resp.team == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}
	if (!ent->client->resp.subteam) {
		Cmd_Kill_f(ent); // lets kill em.
		gi.bprintf(PRINT_HIGH, "%s is now a substitute for %s\n", ent->client->pers.netname, teams[ent->client->resp.team].name);
		ent->client->resp.subteam = ent->client->resp.team;
		return;
	}

	gi.bprintf(PRINT_HIGH, "%s is no longer a substitute for %s\n", ent->client->pers.netname, teams[ent->client->resp.team].name);
	ent->client->resp.subteam = 0;
	if(team_round_going && (teamdm->value || ctf->value))
	{
		ResetKills (ent);
		//AQ2:TNG Slicer Last Damage Location
		ent->client->resp.last_damaged_part = 0;
		ent->client->resp.last_damaged_players[0] = '\0';
		//AQ2:TNG END
		PutClientInServer (ent);
		AddToTransparentList (ent);
	}
}


/*
==============
MM_SetCaptain
==============
Set ent to be a captain of team, ent can be NULL to remove captain
*/
void MM_SetCaptain( int teamNum, edict_t *ent )
{
	int i;
	edict_t *oldCaptain = teams[teamNum].captain;

	if (teamNum == NOTEAM)
		ent = NULL;

	teams[teamNum].captain = ent;
	if (!ent) {
		if (!team_round_going || (!teamdm->value && !ctf->value)) {
			if (teams[teamNum].ready) {
				char temp[128];
				Com_sprintf( temp, sizeof( temp ), "%s is no longer ready to play!", teams[teamNum].name );
				CenterPrintAll( temp );
			}
			teams[teamNum].ready = 0;
		}
		if (oldCaptain) {
			gi.bprintf( PRINT_HIGH, "%s is no longer %s's captain\n", oldCaptain->client->pers.netname, teams[teamNum].name );
		}
		teams[teamNum].locked = 0;
		return;
	}

	if (ent != oldCaptain) {
		gi.bprintf( PRINT_HIGH, "%s is now %s's captain\n", ent->client->pers.netname, teams[teamNum].name );
		gi.cprintf( ent, PRINT_CHAT, "You are the captain of '%s'\n", teams[teamNum].name );
		gi.sound( &g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex( "misc/comp_up.wav" ), 1.0, ATTN_NONE, 0.0 );

		for (i = TEAM1; i <= teamCount; i++) {
			if (i != teamNum && teams[teamNum].wantReset)
				gi.cprintf( ent, PRINT_HIGH, "Team %i want to reset scores, type 'resetscores' to accept\n", i );
		}
	}
}

void MM_LeftTeam( edict_t *ent )
{
	int teamNum = ent->client->resp.team;

	if (teams[teamNum].captain == ent) {
		MM_SetCaptain( teamNum, NULL );
	}
	ent->client->resp.subteam = 0;
}

int TeamsReady(void)
{
	int i;
	
	for(i = TEAM1; i <= teamCount; i++) {
		if(!teams[i].ready)
			return 0;
	}
	return 1;
}

void Cmd_Captain_f(edict_t * ent)
{
	int teamNum;
	edict_t *oldCaptain;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}

	oldCaptain = teams[teamNum].captain;
	if (oldCaptain == ent) {
		MM_SetCaptain( teamNum, NULL );
		return;
	}

	if (oldCaptain) {
		gi.cprintf( ent, PRINT_HIGH, "Your team already has a Captain\n" );
		return;
	}

	MM_SetCaptain( teamNum, ent );
}

//extern int started; // AQ2:M - Matchmode - Used for ready command
void Cmd_Ready_f(edict_t * ent)
{
	char temp[128];
	int		teamNum;
	team_t	*team;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be on a team for that...\n" );
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be a captain for that\n" );
		return;
	}

	if((teamdm->value || ctf->value) && team_round_going) {
		if(teamdm->value)
			gi.cprintf(ent, PRINT_HIGH, "You cant unready in teamdm, use 'pausegame' instead\n");
		else
			gi.cprintf(ent, PRINT_HIGH, "You cant unready in ctf, use 'pausegame' instead\n");
		return;
	}

	team->ready = !team->ready;
	Com_sprintf( temp, sizeof( temp ), "%s %s ready to play!", team->name, (team->ready) ? "is" : "is no longer" );
	CenterPrintAll( temp );
}

void Cmd_Teamname_f(edict_t * ent)
{
	int i, argc, teamNum;
	char temp[32];
	team_t *team;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	if(ctf->value) {
		gi.cprintf(ent, PRINT_HIGH, "You cant change teamnames in ctf mode\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be on a team for that...\n" );
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be a captain for that\n" );
		return;
	}

	if (team->ready) {
		gi.cprintf( ent, PRINT_HIGH, "You cannot use this while 'Ready'\n" );
		return;
	}

	if (team_round_going || team_game_going) {
		gi.cprintf(ent, PRINT_HIGH, "You cannot use this while playing\n");
		return;
	}

	argc = gi.argc();
	if (argc < 2) {
		gi.cprintf( ent, PRINT_HIGH, "Your Team Name is %s\n", team->name );
		return;
	}

	Q_strncpyz(temp, gi.argv(1), sizeof(temp));
	for (i = 2; i <= argc; i++) {
		Q_strncatz(temp, " ", sizeof(temp));
		Q_strncatz(temp, gi.argv(i), sizeof(temp));
	}
	temp[18] = 0;

	if (!temp[0])
		strcpy( temp, "noname" );

	gi.dprintf("%s (Team %i) is now known as %s\n", team->name, teamNum, temp);
	IRC_printf(IRC_T_GAME, "%n (Team %i) is now known as %n", team->name, teamNum, temp);
	strcpy(team->name, temp);
	gi.cprintf(ent, PRINT_HIGH, "New Team Name: %s\n", team->name);

}

void Cmd_Teamskin_f(edict_t * ent)
{
	char *s;
	int teamNum;
	team_t *team;
/*	int i;
	edict_t *e;*/

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be a captain for that\n");
		return;
	}
	if (team->ready) {
		gi.cprintf(ent, PRINT_HIGH, "You cannot use this while 'Ready'\n");
		return;
	}
	if (team_round_going || team_game_going) {
		gi.cprintf(ent, PRINT_HIGH, "You cannot use this while playing\n");
		return;
	}
	if (gi.argc() < 2) {
		gi.cprintf(ent, PRINT_HIGH, "Your Team Skin is %s\n", team->skin);
		return;
	}

	s = gi.argv(1);
	if(!strcmp(s, team->skin)) {
		gi.cprintf(ent, PRINT_HIGH, "Your Team Skin is Already %s\n", s);
		return;
	}

	Q_strncpyz(team->skin, s, sizeof(team->skin));
	if(ctf->value) {
		s = strchr(team->skin, '/');
		if(s)
			s[1] = 0;
		else
			strcpy(team->skin, "male/");
		Q_strncatz(team->skin, teamNum == 1 ? CTF_TEAM1_SKIN : CTF_TEAM2_SKIN, sizeof(team->skin));
	}

	Com_sprintf(team->skin_index, sizeof(team->skin_index), "../players/%s_i", team->skin );
/*	for (i = 1; i <= game.maxclients; i++) { //lets update players skin
		e = g_edicts + i;
		if (!e->inuse)
			continue;

		if(e->client->resp.team == team)
			AssignSkin(e, teams[team].skin, false);
	}*/
	gi.cprintf(ent, PRINT_HIGH, "New Team Skin: %s\n", team->skin);
}

void Cmd_TeamLock_f(edict_t * ent, int a_switch)
{
	char msg[128];
	int teamNum;
	team_t *team;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	if (!mm_allowlock->value) {
		gi.cprintf(ent, PRINT_HIGH, "Team locking is disabled on this server\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You are not on a team\n");
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf(ent, PRINT_HIGH, "You are not the captain of your team\n");
		return;
	}

	if (a_switch == team->locked) {
		gi.cprintf( ent, PRINT_HIGH, "Your team %s locked\n", (a_switch) ? "is already" : "isn't" );
		return;
	}

	team->locked = a_switch;
	Com_sprintf( msg, sizeof( msg ), "%s is now %s", team->name, (a_switch) ? "locked" : "unlocked" );
	CenterPrintAll(msg);
}

void Cmd_SetAdmin_f (edict_t * ent)
{
	if (ent->client->resp.admin) {
		gi.cprintf( ent, PRINT_HIGH, "You are no longer a match admin.\n" );
		gi.dprintf( "%s is no longer a Match Admin\n", ent->client->pers.netname );
		ent->client->resp.admin = 0;
	}

	if(!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "Matchmode is not enabled on this server.\n");
		return;
	}

	if (strcmp( mm_adminpwd->string, "0" ) == 0) {
		gi.cprintf( ent, PRINT_HIGH, "Admin Mode is not enabled on this server..\n" );
		return;
	}

	if (gi.argc() < 2) {
		gi.cprintf (ent, PRINT_HIGH, "Usage:  matchadmin <password>\n");
		return;
	}

	if (strcmp( mm_adminpwd->string, gi.argv(1) )) {
		gi.cprintf( ent, PRINT_HIGH, "Wrong password\n" );
		return;
	}

	gi.cprintf (ent, PRINT_HIGH, "You are now a match admin.\n");
	gi.dprintf ("%s is now a Match Admin\n", ent->client->pers.netname);
	IRC_printf (IRC_T_GAME, "%n is now a Match Admin", ent->client->pers.netname);
	ent->client->resp.admin = 1;
}

void Cmd_ResetScores_f(edict_t * ent)
{
	int i, teamNum, otherCaptain = 0;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	if(ent->client->resp.admin) //Admins can resetscores
	{
		ResetScores(true);
		gi.bprintf(PRINT_HIGH, "Scores and time was resetted by match admin %s\n", ent->client->pers.netname);
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}
	if (teams[teamNum].captain != ent) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be a captain for that\n");
		return;
	}

	if (teams[teamNum].wantReset)
	{
		teams[teamNum].wantReset = 0;
		for (i = TEAM1; i<teamCount + 1; i++) {
			if (i != teamNum && teams[i].captain) {
				gi.cprintf(teams[i].captain, PRINT_HIGH, "Team %i doesnt want to reset afterall", teamNum);
			}
		}
		gi.cprintf(ent, PRINT_HIGH, "Your score reset request cancelled\n");
		return;
	}

	teams[teamNum].wantReset = 1;
	for(i = TEAM1; i<teamCount+1; i++) {
		if(!teams[i].wantReset)
			break;
	}
	if(i == teamCount+1)
	{
		ResetScores(true);
		gi.bprintf(PRINT_HIGH, "Scores and time was resetted by request of captains\n");
		return;
	}

	for (; i<teamCount + 1; i++) {
		if (!teams[i].wantReset && teams[i].captain) {
			gi.cprintf(teams[i].captain, PRINT_HIGH, "Team %i want to reset scores, type 'resetscores' to accept\n", teamNum);
			otherCaptain = 1;
		}
	}
	if(otherCaptain)
		gi.cprintf(ent, PRINT_HIGH, "Your score reset request was send to other team captain\n");
	else
		gi.cprintf(ent, PRINT_HIGH, "Other team need captain and his accept to reset the scores\n");

}

void Cmd_TogglePause_f(edict_t * ent, qboolean pause)
{
	static int lastPaused = 0;
	int		teamNum;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command need matchmode to be enabled\n");
		return;
	}

	if ((int)mm_pausecount->value < 1) {
		gi.cprintf(ent, PRINT_HIGH, "Pause is disabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}

	if (!team_round_going) {
		gi.cprintf(ent, PRINT_HIGH, "No match going so why pause?\n");
		//return;
	}

	if (ent->client->resp.subteam) {
		gi.cprintf(ent, PRINT_HIGH, "You cant pause when substitute\n");
		return;
	}

	if(pause)
	{
		if(level.pauseFrames > 0)
		{
			gi.cprintf(ent, PRINT_HIGH, "Game is already paused you silly\n", time);
			return;
		}
		if (level.intermissiontime) {
			gi.cprintf(ent, PRINT_HIGH, "Can't pause in an intermission.\n");
			return;
		}
		if(teams[teamNum].pauses_used >= (int)mm_pausecount->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Your team doesn't have any pauses left.\n");
			return;
		}
		teams[teamNum].pauses_used++;

		CenterPrintAll (va("Game paused by %s\nTeam %i has %i pauses left", ent->client->pers.netname, ent->client->resp.team, (int)mm_pausecount->value - teams[ent->client->resp.team].pauses_used));
		level.pauseFrames = (unsigned int)(mm_pausetime->value * 60.0f * HZ);
		lastPaused = teamNum;
	}
	else
	{
		if (!level.pauseFrames)
		{
			gi.cprintf(ent, PRINT_HIGH, "Game is not paused\n", time);
			return;
		}
		if(!lastPaused)
		{
			gi.cprintf(ent, PRINT_HIGH, "Already unpausing\n");
			return;
		}
		if(lastPaused != teamNum)
		{
			gi.cprintf(ent, PRINT_HIGH, "You cannot unpause when pause is made by other team\n");
			return;
		}
		level.pauseFrames = 10 * HZ;
		lastPaused = 0;
	}
}

