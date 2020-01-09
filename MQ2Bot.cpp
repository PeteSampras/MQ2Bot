/* notes:
Specific notes:
FixNote 20160728.1 - Need to check vSpawn, if not there, insert vAdd to there, also double check the vAdds[i].Add flag.
FixNote 20160728.2 - this check will need readded for AACutOffTime
FixNote 20160731.1  might need to add more members here, but for now just setting ID and spawn

General notes overall:
Safety string and POSIX updates complete.

General notes spells:
Fully flesh out AAs, disc, heals before moving on.  No need to redo all routines 20 times for no reason.

General notes spawns/storage:
vAdds need checked against vSpawn, the Spawns need to be populated, and then both checked/cleared via OnRemoveSpawn
KillTarget needs to get set somehow.  can re-use some/most of mq2bot code GetKillTarget() for that.
Once we have kill target, each detrimental spell needs to have it added to their target struct member.

General notes casting/spell storage:
I think the CastHandle would actually work as is, but it can be more efficient by adding struct members directly (this is sort of done) so that checks dont need made multiple times for unchanging members.


save for later:
// to use discs: pCharData->DoCombatAbility(vMaster[i].ID);
try
{
if(pSpawn)
{
SPAWNINFO tSpawn;
memcpy(&tSpawn,pSpawn,sizeof(SPAWNINFO));
RemoveFromNotify(&tSpawn, true);
}
}
catch(...)
{
DebugSpewAlways("MQ2Bot::LoadZoneTargets() **Exception**");
}

*/
#define MQ2Bot2 1
#ifdef MQ2Bot2
#pragma region Headers
// Can safely ignore these warnings
//#pragma warning ( disable : 4710 4365 4018 4244 4505 4189 4101 4100 4482 )
#pragma warning( push )
#pragma warning ( disable : 4482 )
#pragma warning(disable: 26495)
// includes

#include "../MQ2Plugin.h"
#include "../moveitem2.h"
#include "../Blech/Blech.h"
#include <vector>
#include <list>
#include <map>
#include <iterator>
#include <deque>
#include <Windows.h>
#include <cstdio>
#include <stdio.h>
#include <Shlwapi.h>
using namespace std;
#include <algorithm>
#pragma comment(lib, "Shlwapi.lib")

//char PLUGIN_NAME[MAX_PATH] = "MQ2Bot"; // this needs reenabled when not using mmobugs source code
PreSetup("MQ2Bot");

// defines
#define NewSource 1
#define	TargetIT(X)			*(PSPAWNINFO*)ppTarget=X
#define	ADD_HP vAdds[0]->HPCurrent*100/vAdds[0]->HPMax
#define	ISINDEX() (Index[0])
#define	ISNUMBER() (IsNumber(Index))
#define	GETNUMBER() (atoi(Index))
#define	GETFIRST() Index
#pragma endregion Headers

#pragma region Prototypes
// enums
enum CLASSES { NONE, WAR, CLR, PAL, RNG, SHD, DRU, MNK, BRD, ROG, SHM, NEC, WIZ, MAG, ENC, BST, BER };
enum ATTRIBSLOTS { ATTRIB, BASE, BASE2, CALC, MAX }; // attrib enum
enum OPTIONS { // used to check routines automatically
	ZERO, AA, AGGRO, AURA, BARD, BUFF, CALL, CHARM, CLICKIES, CLICKYBUFFS, ITEMS,
	CLICKYNUKES, DEBUFF, DOT, ENDURANCE, FADE, FIGHTBUFF, GRAB, HEAL, HEALPET, HEALTOT,
	IMHIT, JOLT, KNOCKBACK, LIFETAP, MAINTANKBUFF, MANA, NUKE, NUKETOT, PET, REZ,
	ROOT, SELFBUFF, SNARE, SUMMONITEM, SWARM, DISC, CURE, MEZ
};
enum TARGETTYPE { UNKNOWN, LoS, AE_PCv1, Group_v1, PB_AE, Single, Self, Ignore, Targeted_AE, Animal, Undead, Summoned, Lifetap = 13, 
	Pet=14, Corpse, Plant, UberGiants, UberDragons, Targeted_AE_Tap=20, AE_Undead=24, AE_Summoned=25, Hatelist=32, Hatelist2=33, Chest=34,
	SpecialMuramites=35, Caster_PB_PC=36,Caster_PB_NPC=37,Pet2=38,NoPets=39,AE_PCv2=40,Group_v2=41,Directional_AE=42,SingleInGroup=43,
	Beam=44,FreeTarget=45,TargetOfTarget=46,PetOwner=47, Target_AE_No_Players_Pets=50
};

enum BUFFTYPE { NOTYPE, Charge, Aego, Skin, Focus, Regen, Symbol, Clarity, Pred, Strength, Brells, SV, SE, HybridHP, 
	Growth, Rune, Rune2, Shining, SpellHaste, MeleeProc, SpellProc, Invis, IVU, Levitate, MoveSpeed, Haste,
	DS1, DS2, DS3, DS4, DS5, DS6, DS7, DS8, DS9, DS10, DS11, DS12, Tash, Malo, Snare, Root, Slow, Mez, Cripple, Fero, Retort, KillShotProc, Unity, DefensiveProc
};
enum SKILLTYPES { TYPE_SPELL = 1, TYPE_AA = 2, TYPE_ITEM = 3, TYPE_DISC = 4 }; // struct for spell data, how to use the spell, and how the spell was used

// structs
typedef struct _Spawns
{
	//vector<_BotSpell>	vSpellList;					// list of what they have? was going to use as bufflist but maybe it is deprecated
	//vector<_Buff>		vBuffList;					// tracks buffs on this character (mine or others)
	char				SpawnBuffList[MAX_STRING];	// might want string version? idk maybe not
	char				Class[MAX_STRING];			// class short name for buff purposes.
	DWORD				ID;							// ID
	int					PetID;						// ID of any pet
	bool				Add;						// is this a confirmed add?
	bool				Friendly;					// is this a friendly?
	PSPAWNINFO			Spawn;						// actual spawn entry, might dump this due to crashes in original bot
	SPAWNINFO			SpawnCopy;					// use this copy instead maybe
	int					Priority;					// priority of spawn. can assign for adds or for spells id think.
	bool				NeedsCheck;					// do i need to check this spawn for buffs/mez/whatever
	ULONGLONG			LastChecked;				// timestamp of last check
	ULONGLONG			Slow;						//mq2 time stamp of when slow should last until, repeat et al for rest
	ULONGLONG			Malo;
	ULONGLONG			Tash;
	ULONGLONG			Haste;
	ULONGLONG			Root;
	ULONGLONG			Snare;
	ULONGLONG			Mez;
	ULONGLONG			DS;
	ULONGLONG			RevDS;
	ULONGLONG			Cripple;
	ULONGLONG			Charge;
	ULONGLONG			Concuss;
	ULONGLONG			MindFroze;
	ULONGLONG			Charm;
	ULONGLONG			Aego;
	ULONGLONG			Skin;
	ULONGLONG			Focus;
	ULONGLONG			Regen;
	ULONGLONG			Symbol;
	ULONGLONG			Clarity;
	ULONGLONG			Pred;
	ULONGLONG			Strength;
	ULONGLONG			Brells;
	ULONGLONG			SV;
	ULONGLONG			SE;
	ULONGLONG			HybridHP;
	ULONGLONG			Growth;
	ULONGLONG			Shining;
	ULONGLONG			DeepSleep;
	ULONGLONG			SpellHaste; // TODO: Code this in if valid requirement
	ULONGLONG			MeleeProc; // TODO: Code this in if valid requirement
	ULONGLONG			SpellProc; // TODO: Code this in if valid requirement
	ULONGLONG			Hot;
	ULONGLONG			Fero;
	ULONGLONG			DelayedHeal;// TODO: Code this in if valid requirement
	int					PoisonCounters;
	int					DiseaseCounters;
	int					CorruptedCounters;
	int					CurseCounters;
	int					DetrimentalCounters;
	BYTE				State;
} Spawns, * PSpawns, SpawnCopy;

typedef struct _BotSpell // has to be before the FunctionDeclarations because a bunch use it
{
	PSPELL				Spell;					// store the spell itself so I can access all of its members
	char				Name[MAX_STRING];		// Check ini to see if they want to use a custom spell, if not then store the name of the spell/AA/Disc/item we actually want to /cast since that is often different
	bool				CanIReprioritize;		// if a specific custom spell or priority was set, then i want to ignore reprioritizing it
	DWORD				SpellIcon;				// store the name of the icon in case it is special like mana reiteration buff icon
	char				Gem[MAX_STRING];		// in case they want to use a custom gem, otherwise this will be alt, disc, item, or default gem
	char				If[MAX_STRING];			// is there a custom if statement in the ini?
	_Spawns				MyTarget;					// My Target for this spell
	char				SpellCat[MAX_STRING];	// what SpellType() is this, so i dont have to check over and over
	char				SpellType[MAX_STRING] = { 0 };	// this will be a custom spell type for each routine.  Ie. Fast Heal, Normal Heal, Fire, Cold, Tash, because I will use this info later on checks and prioritization
	OPTIONS				SpellTypeOption;		// int conversion from spell type
	ULONGLONG			Duration;				// my actual duration, will use GetDuration2() and store it so i dont have to calculate every cast.
	int					UseOnce;				// only use this spell once per mob? this will be an option ini entry
	int					ForceCast;				// should i force memorize this spell if it is not already memmed?  ini entry, will be 1 on buffs, optional on rest
	int					Use;					// should i use this spell at all?  this will allow people to memorize but not use spells in case they want manual control
	int					StartAt;				// individual control of when to start using this spell on friend or foe
	int					StopAt;					// individual control of when to stop using this spell on friend or foe
	int					NamedOnly;				// ini entry to let user determine if they only want to use this on a named.  could use it as a level to say only use it on named above this level to avoid wasting it on easy named. int for now
	int					Priority;				// ini entry that will default to 0, but we can set this priority internally for things like heals and nukes to determine order of cast.  higher number = higher priority
	ULONGLONG			Recast;					// ini entry that will give us a timestamp to reuse a spell if different from it's fastest refresh.  I may only want to cast swarm pets once every 30 seconds even though i could every 12.
	ULONGLONG			LastCast;				// timestamp of last time i cast this spell
	int					LastTargetID;			// last target id that i cast this spell on.
	char				Color[10];				// color to use when cast a spell
	DWORD				ID = 0;					// ID, for Alt ability for now, maybe others
	PALTABILITY			AA;						// AA if this is an AA
	DWORD				PreviousID;				// Needed for my current memmed spells to see if something changed
	int					Type;					// TYPE_AA, TYPE_SPELL, TYPE_DISC, TYPE_ITEM
	int					CastTime;				// Casting time
	DWORD				TargetID;				// Id of target i want to cast on
	int					IniMatch;				// what spell number is this in the ini, if any.
	bool				Prestige;				// if this is an item, is it prestige?
	int					RequiredLevel;			// required level for this
	int					UseInCombat;			// use the skill in combat? default yes combat skill/heals/fightbuffs, default no on buffs, pets, etc.
	char				Classes[MAX_STRING] = { 0 }; // for buffs only. what classes should i buff this one?											
	DWORD				GroupID;				// what is the group ID for ez access
	BUFFTYPE			BuffType =::BUFFTYPE::NOTYPE;	// what BUFFTYPE enum is this, if any
	PSPELL				UnitySpell;				// whats my unified spell for this, if any
												// save this just in case:  void(*CheckFunc)(std::vector<_BotSpell> &, int);
	void(*CheckFunc)(int);
} BotSpell, *PBotSpell;

typedef struct _BotItem // going to need this eventually
{

} BotItem, *PBotItem;

typedef struct _Buff
{
	BUFFTYPE			Type;					// type of buff
	PALTABILITY			Unity;					// special spell for unity
	PALTABILITY			UnityBeza;				// special spell for unity beza
	BotSpell			UnitySpell;				// special spell for unity
	BotSpell			UnityBezaSpell;			// special spell for unity beza
	BotSpell			Self;
	BotSpell			Single;
	BotSpell			Group;
	BotSpell			Pet;
} Buff, *PBuff;

typedef struct _Buffs
{
	Buff				DS1;
	Buff				DS2;
	Buff				DS3;
	Buff				DS4;
	Buff				DS5;
	Buff				DS6;
	Buff				DS7;
	Buff				DS8;
	Buff				DS9;
	Buff				DS10;
	Buff				DS11;
	Buff				DS12;
	Buff				Charge;
	Buff				Aego;
	Buff				Haste;
	Buff				Skin;
	Buff				Focus;
	Buff				Regen;
	Buff				Symbol;
	Buff				Clarity;
	Buff				Pred;
	Buff				Strength;
	Buff				Brells;
	Buff				SV;
	Buff				SE;
	Buff				HybridHP;
	Buff				Growth;
	Buff				Rune;
	Buff				Rune2;
	Buff				Shining;
	Buff				Fero;
	Buff				Retort;
	Buff				SpellHaste; // TODO: Code this in if valid requirement
	Buff				MeleeProc; // TODO: Code this in if valid requirement
	Buff				SpellProc; // TODO: Code this in if valid requirement
	Buff				KillShotProc; // TODO: Code this in if valid requirement
	Buff				Invis; // TODO: Code this in if valid requirement
	Buff				IVU; // TODO: Code this in if valid requirement
	Buff				Levitate; // TODO: Code this in if valid requirement
	Buff				MoveSpeed; // TODO: Code this in if valid requirement
	Buff				DefensiveProc; // TODO:
} Buffs, *PBuffs;

typedef struct _Debuffs
{
	BotSpell			Malo;
	BotSpell			Tash;
	BotSpell			Haste;
	BotSpell			Root;
	BotSpell			Snare;
	BotSpell			Mez;
	BotSpell			DS;
	BotSpell			RevDS;
	BotSpell			Cripple;
	BotSpell			Charge;
	BotSpell			Concuss;
	BotSpell			MindFroze;
	BotSpell			Charm;
	BotSpell			Aego;
	BotSpell			Skin;
	BotSpell			Focus;
	BotSpell			Regen;
	BotSpell			Symbol;
	BotSpell			Clarity;
	BotSpell			Pred;
	BotSpell			Strength;
	BotSpell			Brells;
	BotSpell			SV;
	BotSpell			SE;
	BotSpell			HybridHP;
	BotSpell			Growth;
	BotSpell			Shining;
	BotSpell			DeepSleep;
	BotSpell			SpellHaste; // TODO: Code this in if valid requirement
	BotSpell			MeleeProc; // TODO: Code this in if valid requirement
	BotSpell			SpellProc; // TODO: Code this in if valid requirement
	BotSpell			KillShotProc; // TODO: Code this in if valid requirement
} Debuffs, *PDebuffs;



#pragma region FunctionDeclarations
// declaration of create functions
void CreateAA();
void CreateDisc();
void CreateHeal();

// declaration of check routines
void CheckAA(int spell);
void CheckAggro(int spell);
void CheckAura(int spell);
void CheckBard(int spell);
void CheckBuff(int spell);
void CheckCall(int spell);
void CheckCharm(int spell);
void CheckClickies(int item);
void CheckClickyBuffs(int item);
void CheckClickyNukes(int item);
void CheckCure(int spell);
void CheckDebuff(int spell);
void CheckDisc(int spell);
void CheckDot(int spell);
void CheckEndurance(int spell);
void CheckFade(int spell);
void CheckFightBuff(int spell);
void CheckGrab(int spell);
void CheckHeal(int spell);
void CheckHealPet(int spell);
void CheckHealToT(int spell);
void CheckImHit(int spell);
void CheckItems(int item);
void CheckJolt(int spell);
void CheckKnockback(int spell);
void CheckLifetap(int spell);
void CheckMainTankBuff(int spell);
void CheckMana(int spell);
void CheckMez(int spell);
void CheckNuke(int spell);
void CheckNukeToT(int spell);
void CheckPet(int spell);
void CheckRez(int spell);
void CheckRoot(int spell);
void CheckSelfBuff(int spell);
void CheckSnare(int spell);
void CheckSummonItem(int spell);
void CheckSwarm(int spell);

// declaration of general functions
int			CalcDuration(PSPELL pSpell);
void		BotCastCommand(_BotSpell &spell);
VOID		DebugWrite(PCHAR szFormat, ...);
void		EQBCSwap(char startColor[MAX_STRING]);
double		botround(double d);

// declaration of spawn functions
void		CheckGroupPets(int i);
int			PctEnd(PSPAWNINFO pSpawn);
int			PctHP(PSPAWNINFO pSpawn);
int			PctMana(PSPAWNINFO pSpawn);

// declaration of spell functions
void		DiscCategory(PSPELL pSpell);
DWORD		GetSpellDuration2(PSPELL pSpell);
void		PopulateIni(vector<_BotSpell> &v, char VectorName[MAX_STRING]);
void		SortSpellVector(vector<_BotSpell> &v);
void		SpellCategory(PSPELL pSpell);
void		SkillType(char szName[MAX_STRING]);
void		SpellType(PSPELL pSpell);
bool		ValidDet(PSPELL pSpell, PSPAWNINFO Target);
bool		ValidBen(PSPELL pSpell, PSPAWNINFO Target);
#pragma endregion FunctionDeclarations

#pragma region VariableDefinitions

// function declare
void(*spellFunc)(int);
// bool declares
bool	BardClass = false, ConfigureLoaded = false, InCombat = false, summoned = false, UseManualAssist = false, UseNetBots = false, storePrestige = false, bGoldStatus = false;

// char declares
char	AddList[MAX_STRING] = { 0 }, AssistName[MAX_STRING] = { 0 }, BodyTypeFix[MAX_STRING] = { 0 }, CurrentRoutine[MAX_STRING] = { 0 }, EQBCColor[MAX_STRING] = { 0 },
INISection[MAX_STRING] = { 0 }, NetBotsName[MAX_STRING] = "NULL", spellCat[MAX_STRING] = { 0 }, spellType[MAX_STRING] = { 0 }, storeGem[10] = { 0 }, storeName[MAX_STRING] = { 0 },
conColor[MAX_STRING] = { 0 }, DEBUG_DUMPFILE[MAX_STRING]={0};

char DSClasses[MAX_STRING], HasteClasses[MAX_STRING], AegoClasses[MAX_STRING], SkinClasses[MAX_STRING],
SymbolClasses[MAX_STRING], FocusClasses[MAX_STRING], RegenClasses[MAX_STRING], ClarityClasses[MAX_STRING],
PredClasses[MAX_STRING], StrengthClasses[MAX_STRING], BrellsClasses[MAX_STRING], SVClasses[MAX_STRING],
SEClasses[MAX_STRING], FeroClasses[MAX_STRING], SpellHasteClasses[MAX_STRING], ACClasses[MAX_STRING];

char BuffClasses[MAX_STRING] = "|DS|Haste|Aego|Skin|Symbol|Focus|Regen|Clarity|Pred|Strength|Brells|SV|SE|Fero|SpellHaste|AC|";

// Long declares
long MyClass;

// DWORD declares
DWORD LastBodyID, storeID;

// int declares
int		AnnounceAdds = 1, AnnounceEcho = 1, AnnounceEQBC = 0, AssistAt = 100, AssistRange = 100, DefaultGem = 1, dotExtend = 0, hotExtend = 0, storeType = 0, storeRequiredLevel = 0,
mezExtend = 0;

BUFFTYPE buffType = ::BUFFTYPE::NOTYPE;

// float declares
float	benDurExtend = 0.00, benRngExtend = 1.00, detDurExtend = 0.00, detRngExtend = 1.00, fCampX, fCampY, fCampZ, FightX = 0, FightY = 0, FightZ = 0,
reinforcement = 0.00, WarpDistance = 0.00;

// ULONGLONG declares
ULONGLONG AssistTimer = 0, LastAnnouncedSpell = 0, SpellTimer = 0;

// PSPELL declares
PSPELL storeSpell, UnitySpell;

//vector _BotSpell declares
//vector<_BotSpell> vMaster, vMemmedSpells, vTemp;
vector<_BotSpell> vMaster;
vector<_BotSpell> vMemmedSpells(NUM_SPELL_GEMS - 1, _BotSpell());
vector<_BotSpell> vTemp;
vector<_BotSpell> vCustomSpells;
vector<_Buff> vBuff;

//vector int declares
vector<int> vClickyPrestige;

// vector PSPELL declares
vector<PSPELL> vClickySpell;

// vector PSPAWNINFO declares
_Spawns KillTarget, xNotTargetingMe, xTargetingMe; // we always store the current kill target here
vector<_Spawns> vGroup, vXTarget, vSpawns, vPets; // manage all the various spawns, and if something is mezzed, dont remove it unless it is banished.

// buffs
_Buffs MyBuffs;
_Debuffs MyDebuffs;

												  // vector string declares
vector<string> vClicky;

// map declares
std::map<string, string> SpellIf;
std::map<string, string>::iterator SpellIt;

// deque declares
deque<_Spawns>	vAdds;

// buff spells for hardcoding
// unified lines: symbol, aego, bst SV/SE unity
BotSpell
//long duration buffs
buffSelfSpellResist,buffSingleSpellResist,buffGroupSpellResist, // might need to break this out by class or resist type since there's so many.
buffSingleResistCorruption,buffGroupResistCorruption, //maybe?
buffSelfHybridHP, // pal/sk self hp buffs
buffSelfConvert, // SK/nec mana conversion
buffSelfAttack, // SK attack buff
buffSingleAego, buffGroupAego, buffSingleUnifiedAego, buffGroupUnifiedAego, buffGroupSpellHaste, buffSingleAC, buffGroupAC,//clr
buffSingleSymbol, buffGroupSymbol, buffSingleUnifiedSymbol, buffGroupUnifiedSymbol, buffSingleSpellHaste, //clr .. 
buffSingleHaste, buffGrouphaste, buffAAHaste, // shm, enc, bst (single)
buffSingleSkin, buffGroupSkin, //dru /rng (single only rng)
buffSelfFocus, //clr/casters
buffSingleFocus, buffGroupFocus, // shm / bst (group only except for one single spell at level 67)
buffSingleRegen, buffGroupRegen, // shm/dru rang(single)
buffSelfClarity, buffSingleClarity, buffGroupClarity, // rng, dru self . enc single/group
buffSelfPred, buffGroupPred, buffGroupStrength, // rng
buffGroupBrells, //pal only
buffGroupSV, buffGroupSE, //bst only
//HybridHP, //not sure if i need this atm
buffSelfDS, buffSingleDS, buffGroupDS, buffPetDS,
buffSingleFerocity, buffGroupFerocity, // dont use single if you have group
// there are a ton of different lines for procs on hit or cast. many classes get 2, sometimes 3
buffSelfMeleeProc, buffSingleMeleeProc, buffGroupMeleeProc,
buffSelfMeleeProc2,
buffSelfSpellProc, buffSingleSpellProc, buffGroupSpellProc,
buffSelfKillShotProc,
// misc buffs common across classes
buffSelfInvis, buffSingleInvis, buffGroupInvis,
buffSelfIVU, buffSingleIVU, buffGroupIVU,
buffSelfLevitate, buffSingleLevitate, buffGroupLevitate,
buffSingleMoveSpeed, buffGroupMoveSpeed,
//things i might only use if memmed, usually short duration and require getting hit to matter
buffSelfDefensiveProc, buffSingleDefensiveProc,
buffSelfRune, buffSingleRune, buffGroupRune,
buffSelfRune2, buffSingleRune2, buffGroupRune2,
buffDivineIntervention, //clr
//things i should only use if memmed.. high reuse timer or extremely short duration or both
buffSingleRetort, buffSelfDivineAura, buffSingleDivineAura, buffSelfDivineAura2, buffYaulp, buffSingleShining, //clr.. ok clerics.. we get it. you are special
buffSelfWard, buffSingleWard,
buffSingleGrowth;// dru/shm

// PCHAR declares
PCHAR ChatColors[] = { "\ay", "\a-y", "\ao", "\a-o", "\ag", "\a-g", "\ab", "\a-b", "\ar", "\a-r",
"\at", "\a-t", "\am", "\a-m", "\ap", "\a-p", "\aw", "\a-w", "\ax", "\a-x", "\au", "\a-u", NULL };
PCHAR EQBCColors[] = { "[+y+]", "[+Y+]", "[+o+]", "[+O+]", "[+g+]", "[+G+]", "[+b+]", "[+B+]", "[+r+]", "[+R+]", "[+t+]",
"[+T+]", "[+m+]", "[+M+]", "[+p+]", "[+P+]", "[+w+]", "[+W+]", "[+x+]", "[+X+]", "[+u+]", "[+U+]", NULL };
PCHAR cast_status[] = { "SUCCESS","INTERRUPTED","RESIST","COLLAPSE","RECOVER","FIZZLE","STANDING","STUNNED","INVISIBLE","NOTREADY",
"OUTOFMANA","OUTOFRANGE","NOTARGET","CANNOTSEE","COMPONENTS","OUTDOORS","TAKEHOLD","IMMUNE","DISTRACTED","ABORTED",	"UNKNOWN",	"IMMUNE_RUN", "IMMUNE_MEZ","IMMUNE_SLOW",NULL };
PCHAR DefaultSection[] = { "AA", "Aggro","Aura","Bard","Buff","Call","Charm","Clickies", "ClickyBuffs","Items",
"ClickyNukes","Debuff","Dot","Endurance","Fade","FightBuff","Grab","Heal","HealPet","HealToT",
"ImHit","Jolt","KnockBack","Lifetap","MainTankBuff","Mana","Nuke","NukeToT","Pet","Rez",
"Root","SelfBuff","Snare","SummonItem","Swarm", "Disc","Cure", "Mez", NULL }; // 10 per line
PCHAR DefaultUseInCombat[] = { "1","1","0","1","0","1","1","1","0","1",
"1","1","1","0","1","1","1","1","1","1", 
"1","1","1","1","1","0","1","1","0","1",
"1","0","1","0","1","1","1","1",NULL }; // 10 per line
PCHAR DefaultColor[] = { "\ap", "\a-r","\a-m","\a-t","\aw","\ao","\am","\a-o","\a-o","\a-o",
"\a-o","\ay","\ay","\ag","\a-w","\am","\a-g","\at","\a-t","\a-t",
"\ay","\at","\a-g","\ag","\ag","\ao","\ar","\a-t","\a-t","\a-o",
"\a-g","\aw","\a-g","\a-o","\a-r", "\a-y","\ag","\a-m", NULL };// 10 per line
PCHAR DefaultPriority[] = { "300","900", "0", "0", "0", "0", "0", "0", "0", "0",
"00","800", "700", "0", "950", "0", "0", "990", "850", "0",
"980","1000", "0", "910", "0", "0", "450", "0", "0", "0",
"500","0", "750", "0", "720","600","0", "1000", NULL };// 10 per line
PCHAR DefaultStartAt[] = { "99","100", "100", "100", "100", "100", "100", "100", "100", "100",
"99","100", "99", "17", "50", "100", "100", "80", "80", "80",
"80","70", "100", "80", "100", "80", "99", "99", "100", "100",
"100","100", "100", "100", "99","99", "99", "100", NULL };// 10 per line
PCHAR DefaultStopAt[] = { "0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0","0", "0","0",NULL };// 10 per line
PCHAR DefaultUse[] = { "1","1", "1", "1", "1", "1", "1", "1", "1", "1",
"1","1", "1", "1", "1", "1", "1", "1", "1", "1",
"1","1", "1", "1", "1", "1", "1", "1", "1", "1",
"1","1", "1", "1", "1","1","1","1", NULL };// 10 per line
PCHAR DefaultForceCast[] = { "0","0", "0", "0", "1", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "1", "0", "1",
"0","1", "0", "1", "0", "0","0","0", NULL };// 10 per line
PCHAR DefaultNamedOnly[] = { "0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0","0","0", NULL };// 10 per line
PCHAR DefaultUseOnce[] = { "0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0", "0", "0", "0", "0",
"0","0", "0", "0", "0", "0","0","0", NULL };// 10 per line

											// special declares
OPTIONS	spellTypeOption;
_BotSpell EndRegenSpell;
#pragma endregion VariableDefinitions
#pragma endregion Prototypes

#pragma region Inlines
// Returns TRUE if character is in game and has valid character data structures
inline bool InGameOK()
{
	return(GetGameState() == GAMESTATE_INGAME && GetCharInfo() && GetCharInfo()->pSpawn && GetCharInfo2());
}

// Returns TRUE if the specified UI window is visible
static inline BOOL WinState(CXWnd *Wnd)
{
	return (Wnd && ((PCSIDLWND)Wnd)->IsVisible());
}

static inline LONG GetSpellAttribX(PSPELL spell, int slot) {
	return slot < GetSpellNumEffects(spell) ? GetSpellAttrib(spell, slot) : 254;
}
static inline LONG GetSpellBaseX(PSPELL spell, int slot) {
	return slot < GetSpellNumEffects(spell) ? GetSpellBase(spell, slot) : 0;
}
static inline LONG GetSpellBase2X(PSPELL spell, int slot) {
	return slot < GetSpellNumEffects(spell) ? GetSpellBase2(spell, slot) : 0;
}
static inline LONG GetSpellMaxX(PSPELL spell, int slot) {
	return slot < GetSpellNumEffects(spell) ? GetSpellMax(spell, slot) : 0;
}
static inline LONG GetSpellCalcX(PSPELL spell, int slot) {
	return slot < GetSpellNumEffects(spell) ? GetSpellCalc(spell, slot) : 0;
}
#pragma endregion Inlines

#pragma region GeneralFunctionDefinitions
void Announce(_BotSpell &spell)
{
	if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(spell.TargetID))
	{
		if (AnnounceEcho && pSpawn && LastAnnouncedSpell != spell.Spell->ID)
		{
			WriteChatf("%s%s \aw--> %s", spell.Color, spell.Name, pSpawn->DisplayedName);
		}
		if (AnnounceEQBC && pSpawn && LastAnnouncedSpell != spell.Spell->ID)
		{
			EQBCSwap(spell.Color);
			char castLine[MAX_STRING] = { 0 };
			sprintf_s(castLine, "/bc %s%s [+x+]--> %s", EQBCColor, spell.Name, pSpawn->DisplayedName);
			HideDoCommand(GetCharInfo()->pSpawn, castLine, FromPlugin);
		}
		LastAnnouncedSpell = spell.Spell->ID;
	}
}

DWORD ChkCreateDir(char* pszDir)
{
	DWORD dwAtt = 0;
	dwAtt = GetFileAttributes(pszDir);
	if (FILE_ATTRIBUTE_DIRECTORY == dwAtt) return(dwAtt);
	char  acDir[MAX_PATH];
	strcpy_s(acDir, pszDir);
	bool bCreateFolder = true;
	static bool bIsFile = false;
	char* pc = NULL;
	pc = strrchr(acDir, '\\');

	if (pc) {
		if (!bIsFile) {
			bIsFile = true;
			bCreateFolder = false;
		}
		*pc = (char)0;
	}
	else
		return(0xffffffff);

	dwAtt = ChkCreateDir(acDir);

	if (FILE_ATTRIBUTE_DIRECTORY == dwAtt && bCreateFolder) {
		if (!CreateDirectory(pszDir, NULL))
			return GetLastError();

		return(dwAtt);
	}

	return(0);
}

VOID DebugWrite(PCHAR szFormat, ...)
{
	if (_stricmp(DEBUG_DUMPFILE,"all") && _stricmp(DEBUG_DUMPFILE,CurrentRoutine))
		return;
	try
	{
		va_list vaList;
		va_start(vaList, szFormat);
		int len = _vscprintf(szFormat, vaList) + 1;
		if (char *szOutput = (char *)LocalAlloc(LPTR, len + 32))
		{
			vsprintf_s(szOutput, len + 32, szFormat, vaList);
			FILE *fOut = NULL;
			CHAR Filename[MAX_STRING] = { 0 };
			CHAR szBuffer[MAX_STRING] = { 0 };
			DWORD i;
			sprintf_s(Filename, "%s\\MQ2Bot_Debug.log", gszLogPath);
			ChkCreateDir(gszLogPath);
			for (i = 0; i < strlen(Filename); i++)
			{
				if (Filename[i] == '\\')
				{
					strncpy_s(szBuffer, Filename, i);
					szBuffer[i] = 0;
				}
			}
			fOut = _fsopen(Filename, "at", SH_DENYNO);
			if (!fOut)
			{
				return;
			}
			char tmpbuf[128];
			struct tm today;
			time_t tm;
			tm = time(NULL);
			localtime_s(&today, &tm);
			strftime(tmpbuf, 128, "%Y/%m/%d %H:%M:%S", &today);
			CHAR PlainText[MAX_STRING] = { 0 };
			StripMQChat(szOutput, PlainText);
			sprintf_s(szBuffer, "[%s] %s", tmpbuf, PlainText);
			for (unsigned int i = 0; i < strlen(szBuffer); i++) {
				if (szBuffer[i] == 0x07) {
					if ((i + 2) < strlen(szBuffer))
						strcpy_s(&szBuffer[i], MAX_STRING, &szBuffer[i + 2]);
					else
						szBuffer[i] = 0;
				}
			}
			fprintf(fOut, "%s\n", szBuffer);
			fclose(fOut);
			LocalFree(szOutput);
		}
	}
	catch (exception &e)
	{
		sprintf_s(DEBUG_DUMPFILE, NULL);
		FatalError("[%s::%s] Severe Error: %s : %s", PLUGIN_NAME, __FUNCTION__, e.what(), typeid(e).name());
	}
	catch (...)
	{
		sprintf_s(DEBUG_DUMPFILE, NULL);
		FatalError("[%s::%s] Severe Error: Unknown", PLUGIN_NAME, __FUNCTION__);
	}
}

void DurationSetup()
{
	if (!InGameOK())
		return;
	PCHARINFO2 pChar2 = GetCharInfo2();
	int ben = 0, det = 0, mez = 0;
	//remove char itemName[MAX_STRING], spellName[MAX_STRING], itemlevel[MAX_STRING];
	if (pChar2 && pChar2->pInventoryArray && pChar2->pInventoryArray->InventoryArray)
	{
		for (int i = 1; i < NUM_INV_SLOTS; i++)
		{
			if (PCONTENTS Cont = GetCharInfo2()->pInventoryArray->InventoryArray[i])
				if (PSPELL pFocus = GetSpellByID(GetItemFromContents(Cont)->Focus.SpellID))
				{
					if (GetSpellAttribX(pFocus, 0) == 128 && GetSpellBaseX(pFocus, 2) == 1)
						benDurExtend = (float)GetSpellBaseX(pFocus, 0) / 100;
					if (GetSpellAttribX(pFocus, 0) == 128 && GetSpellBaseX(pFocus, 2) == 0)
						detDurExtend = (float)GetSpellBaseX(pFocus, 0) / 100;
					if (GetSpellAttribX(pFocus, 0) == 129 && GetSpellBaseX(pFocus, 3) == 1)
						benRngExtend = (float)GetSpellBaseX(pFocus, 0) / 100 + 1;
					if (GetSpellAttribX(pFocus, 0) == 129 && GetSpellBaseX(pFocus, 3) == 0)
						detRngExtend = (float)GetSpellBaseX(pFocus, 0) / 100 + 1;
				}
		}
		if (reinforcement < 0.01 && GetCharInfo()->pSpawn->Level>55) {
			long Index = GetAAIndexByName("Spell Casting Reinforcement");
			if (Index > 0)
			{
				if (PlayerHasAAAbility(Index))
					for (int j = 0; j < AA_CHAR_MAX_REAL; j++) {
						if (pPCData->GetAlternateAbilityId(j) == Index)
							reinforcement = (float)(0.025*GetCharInfo2()->AAList[j].PointsSpent);
						if (reinforcement > 0.01 || !pPCData->GetAlternateAbilityId(j))
							break;
					}
			}
			Index = GetAAIndexByName("Prolonged Salve");
			if (Index > 0)
			{
				if (PlayerHasAAAbility(Index))
					for (int j = 0; j < AA_CHAR_MAX_REAL; j++) {
						if (pPCData->GetAlternateAbilityId(j) == Index)
							hotExtend = (int)botround(GetCharInfo2()->AAList[j].PointsSpent / 12);
						if (hotExtend > 0.01 || !pPCData->GetAlternateAbilityId(j))
							break;
					}
			}
			Index = GetAAIndexByName("Powerful Elixirs");
			if (Index > 0)
			{
				if (PlayerHasAAAbility(Index))
					for (int j = 0; j < AA_CHAR_MAX_REAL; j++) {
						if (pPCData->GetAlternateAbilityId(j) == Index)
							hotExtend = (int)botround(GetCharInfo2()->AAList[j].PointsSpent / 12);
						if (hotExtend > 0.01 || !pPCData->GetAlternateAbilityId(j))
							break;
					}
			}
			if (PlayerHasAAAbility(GetAAIndexByName("Mesmerization Mastery"))) {
				for (unsigned int j = 0; j < AA_CHAR_MAX_REAL; j++) {
					if (pPCData->GetAlternateAbilityId(j)) {
						if (pPCData->GetAlternateAbilityId(j) == GetAAIndexByName("Mesmerization Mastery")) {
							mez = GetCharInfo2()->AAList[j].PointsSpent;
							if (mez >= 28) // level 3+ (add more checks before this if expansions add more ranks)
								mezExtend += 3;
							else if (mez >= 20) // level 2
								mezExtend += 2;
							else if (mez >= 12) // level 1
								mezExtend += 1;
							break;
						}
					}
				}
			}
			Index = GetAAIndexByName("Lingering Death");
			if (Index > 0)
			{
				if (PlayerHasAAAbility(Index))
					for (int j = 0; j < AA_CHAR_MAX_REAL; j++) {
						if (pPCData->GetAlternateAbilityId(j) == Index)
							dotExtend = (int)botround(GetCharInfo2()->AAList[j].PointsSpent / 13);
						if (dotExtend > 0.01 || !pPCData->GetAlternateAbilityId(j))
							break;
					}
			}
			Index = GetAAIndexByName("Extended Pestilence");
			if (Index > 0)
			{
				if (PlayerHasAAAbility(Index))
					for (int j = 0; j < AA_CHAR_MAX_REAL; j++) {
						if (pPCData->GetAlternateAbilityId(j) == Index)
							if (GetCharInfo2()->AAList[j].PointsSpent >= 48)
								dotExtend = 3;
						if (dotExtend > 0.01 || !pPCData->GetAlternateAbilityId(j))
							break;
					}
			}

		}
	}
}

void EQBCSwap(char startColor[MAX_STRING])
{
	strcpy_s(EQBCColor, "[+x+]");
	for (int i = 0; ChatColors[i] && EQBCColors[i]; i++)
	{
		if (strstr(startColor, ChatColors[i]))
		{
			strcpy_s(EQBCColor, EQBCColors[i]);
			return;
		}
	}
}

bool FindPlugins(PCHAR PluginName)
{
	PMQPLUGIN pPlugin = pPlugins;
	while (pPlugin)
	{
		if (!_stricmp(pPlugin->szFilename, PluginName))
			return true;
		pPlugin = pPlugin->pNext;
	}
	return false;
}

int FindSpellAttrib(PSPELL pSpell, int attrib)
{
	for (int slot = 0; slot < GetSpellNumEffects(pSpell); ++slot)
	{
		if (GetSpellAttrib(pSpell, slot) == attrib)
			return slot;
	}
	return -1; // not found
}

bool HasSpellAttrib(PSPELL pSpell, int attrib)
{
	return FindSpellAttrib(pSpell, attrib) != -1;
}

PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
{
	if (_stricmp(DEBUG_DUMPFILE, "all") && _stricmp(DEBUG_DUMPFILE, CurrentRoutine))
		DebugWrite(Line);
	return 0;
}

int PctEnd(PSPAWNINFO pSpawn)
{
	if (pSpawn)
	{
		if (pSpawn->GetMaxEndurance() > 0)
			return ((pSpawn->GetCurrentEndurance() * 100) / pSpawn->GetMaxEndurance());
		else
			return 0;
	}
	return 0;
}

int PctHP(PSPAWNINFO pSpawn)
{
	if (pSpawn)
	{
		if (pSpawn->HPMax > 0)
			return (unsigned int)((pSpawn->HPCurrent * 100) / pSpawn->HPMax);
		else
			return 0;
	}
	return 0;
}

int PctMana(PSPAWNINFO pSpawn)
{
	if (pSpawn)
	{
		if (pSpawn->GetMaxMana() > 0)
			return ((pSpawn->GetCurrentMana() * 100) / pSpawn->GetMaxMana());
		else
			return 0;
	}
	return 0;
}

void PopulateIni(vector<_BotSpell> &v, char VectorName[MAX_STRING])
{
	char szTemp[MAX_STRING] = { 0 }, szSpell[MAX_STRING];
	int vSize = v.size();
	for (int x = 0; x < vSize; x++)
	{
		int i = x + 1;
		sprintf_s(szSpell, "%sTotal", VectorName);
		_itoa_s(v.size(), szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dName", VectorName, i);
		WritePrivateProfileString(INISection, szSpell, v[x].Name, INIFileName);
		//sprintf_s(szSpell, "%sSpellIconName%d", VectorName, i);
		//WritePrivateProfileString(INISection, szSpell, v[x].SpellIconName, INIFileName);
		sprintf_s(szSpell, "%s%dIf", VectorName, i);
		if (IsNumber(v[x].If))
		{
			_itoa_s(atoi(v[x].If), szTemp, 10);
			WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		}
		else
			WritePrivateProfileString(INISection, szSpell, v[x].If, INIFileName);
		sprintf_s(szSpell, "%s%dGem", VectorName, i);
		if (IsNumber(szSpell))
		{
			_itoa_s(atoi(v[x].Gem), szTemp, 10);
			WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		}
		else
			WritePrivateProfileString(INISection, szSpell, v[x].Gem, INIFileName);
		sprintf_s(szSpell, "%s%dUseOnce", VectorName, i);
		_itoa_s(v[x].UseOnce, szTemp, 10);
		sprintf_s(szSpell, "%s%dForceCast", VectorName, i);
		_itoa_s(v[x].ForceCast, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dUse", VectorName, i);
		_itoa_s(v[x].Use, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dStartAt", VectorName, i);
		_itoa_s(v[x].StartAt, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dStopAt", VectorName, i);
		_itoa_s(v[x].StopAt, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dNamedOnly", VectorName, i);
		_itoa_s(v[x].NamedOnly, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dPriority", VectorName, i);
		_itoa_s(v[x].Priority, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
		sprintf_s(szSpell, "%s%dUseInCombat", VectorName, i);
		_itoa_s(v[x].UseInCombat, szTemp, 10);
		WritePrivateProfileString(INISection, szSpell, szTemp, INIFileName);
	}
}

double botround(double d)
{
	return floor(d + 0.5);
}

bool SpellStacks(PSPELL pSpell)
{
	if (!InGameOK())
		return true;
	unsigned long nBuff;
	PCHARINFO2 pChar = GetCharInfo2();
	bool stacks = true;
	//remove char cTest[MAX_STRING];
	PSPELL tmpSpell2;
	// Check Buffs
	for (nBuff = 0; nBuff < NUM_LONG_BUFFS && pChar; nBuff++)
	{
		if (pChar->Buff[nBuff].SpellID > 0 && GetSpellByID(pChar->Buff[nBuff].SpellID)) {
			PSPELL tmpSpell = GetSpellByID(pChar->Buff[nBuff].SpellID);
			if (tmpSpell && pSpell)
			{
				for (int nSlot = 0; nSlot < GetSpellNumEffects(pSpell); nSlot++)
				{
					int attrib = GetSpellAttrib(pSpell, nSlot);

					if (attrib == 374 || attrib == 419 || attrib == 339)
					{
						tmpSpell2 = GetSpellByID(GetSpellBase2(pSpell, nSlot));
						if (tmpSpell2 && !BuffStackTest(tmpSpell, tmpSpell2) || !outdoor && tmpSpell2->ZoneType == 1 || tmpSpell == tmpSpell2 || attrib == 339 && GetSpellAttribX(tmpSpell, nSlot) == 339 || strstr(pSpell->Name, tmpSpell->Name))
						{
							stacks = false;
							return stacks;
						}
					}
					if (attrib == 339 && GetSpellAttribX(tmpSpell, nSlot) == 339 || strstr(pSpell->Name, tmpSpell->Name) || !outdoor && tmpSpell->ZoneType == 1)
					{
						stacks = false;
						return stacks;
					}
				}
				if ((!BuffStackTest(pSpell, tmpSpell) || pSpell == tmpSpell || strstr(pSpell->Name, tmpSpell->Name)) || !outdoor && pSpell->ZoneType == 1)
				{
					stacks = false;
					return stacks;
				}
			}
		}
	}
	// Check Songs
	for (nBuff = 0; nBuff < NUM_SHORT_BUFFS && pChar; nBuff++)
	{
		if (pChar->ShortBuff[nBuff].SpellID > 0) {
			PSPELL tmpSpell = GetSpellByID(pChar->ShortBuff[nBuff].SpellID);
			if (tmpSpell && pSpell && (!(tmpSpell->ClassLevel[Bard] == 255 ? 0 : tmpSpell->ClassLevel[Bard]) && (!(GetSpellAttribX(pSpell, 0) == 58) || !(pSpell->DurationWindow == 1))))
			{
				for (int nSlot = 0; nSlot < GetSpellNumEffects(pSpell); nSlot++)
				{
					int attrib = GetSpellAttrib(pSpell, nSlot);

					if (attrib == 374 || attrib == 419)
					{
						tmpSpell2 = GetSpellByID(GetSpellBase2(pSpell, nSlot));
						if (tmpSpell2 && !BuffStackTest(tmpSpell, tmpSpell2) || pSpell == tmpSpell2 || tmpSpell == tmpSpell2 || strstr(pSpell->Name, tmpSpell->Name))
						{
							stacks = false;
							return stacks;
						}
					}
					if (attrib == 339 && GetSpellAttribX(tmpSpell, nSlot) == 339 || strstr(pSpell->Name, tmpSpell->Name))
					{
						stacks = false;
						return stacks;
					}
				}
				if ((!BuffStackTest(pSpell, tmpSpell) || pSpell == tmpSpell || strstr(pSpell->Name, tmpSpell->Name)))
				{
					stacks = false;
					return stacks;
				}
			}
		}
	}
	//check disc
	if (pCombatAbilityWnd) {
		if (CXWnd *Child = ((CXWnd*)pCombatAbilityWnd)->GetChildItem("CAW_CombatEffectLabel")) {
			CHAR szBuffer[2048] = { 0 };
			if (GetCXStr(Child->CGetWindowText(), szBuffer, 2047) && szBuffer[0] != '\0') {
				PSPELL tmpSpell = GetSpellByName(szBuffer);
				if (pSpell && tmpSpell) {
					if (!BuffStackTest(pSpell, tmpSpell)) {
						stacks = false;
						return stacks;
					}
				}
			}
		}
	}
	return true;
}

#pragma endregion GeneralFunctionDefinitions

#pragma region SpawnFunctionDefinitions
bool sortSpawnByPriority(const Spawns &lhs, const Spawns &rhs) { return lhs.Priority > rhs.Priority; } // Sort spawn vector by priority

void SortSpawnVector(vector<_Spawns> &v)  // the actual sorting of a spawn vector by priority
{
	sort(v.begin(), v.end(), sortSpawnByPriority);
}
void ConColorSwap(PSPAWNINFO pSpawn)
{
	if (!pSpawn)
		return;
	if (ConColor(pSpawn) == CONCOLOR_GREY)
		strcpy_s(conColor, "\a-w");
	if (ConColor(pSpawn) == CONCOLOR_GREEN)
		strcpy_s(conColor, "\ag");
	if (ConColor(pSpawn) == CONCOLOR_LIGHTBLUE)
		strcpy_s(conColor, "\at");
	if (ConColor(pSpawn) == CONCOLOR_BLUE)
		strcpy_s(conColor, "\au");
	if (ConColor(pSpawn) == CONCOLOR_WHITE)
		strcpy_s(conColor, "\aw");
	if (ConColor(pSpawn) == CONCOLOR_YELLOW)
		strcpy_s(conColor, "\ay");
	if (ConColor(pSpawn) == CONCOLOR_RED)
		strcpy_s(conColor, "\ay");
}

void CheckAdds()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s", CurrentRoutine); // test code
	if (!GetCharInfo()->pXTargetMgr)
		return;
	char szXTAR[MAX_STRING];
	char szXTAR2[MAX_STRING];
	char testAddList[MAX_STRING];
	int xtar = 0;
	if (ExtendedTargetList* etl = GetCharInfo()->pXTargetMgr)
	{
		vAdds.clear();
		InCombat = false;
		int npc = 0;
		int leastaggro = 100;
		int mostaggro = 99;
		_Spawns aggro;
		xNotTargetingMe = aggro;
		xTargetingMe = aggro;
		if (((PCPLAYERWND)pPlayerWnd))
			if (((CXWnd*)pPlayerWnd)->GetChildItem("PW_CombatStateAnim"))
				if (((PCPLAYERWND)pPlayerWnd)->CombatState == 0)
					InCombat = true;
		try // access spawn data crashes people when something despawns so we need to try, and we need to use memcpy of the data.
		{
			for (int n = 0; n < etl->XTargetSlots.Count; n++)
			{
				XTARGETSLOT& slot = etl->XTargetSlots[n];
				if (slot.SpawnID > 0)
				{
					if (slot.xTargetType)
						sprintf_s(szXTAR, "%s", GetXtargetType(slot.xTargetType));
					PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(slot.SpawnID);
					if (pSpawn)
					{
						_Spawns add;
						SPAWNINFO tSpawn;
						memcpy(&tSpawn, pSpawn, sizeof(SPAWNINFO));
						if (pSpawn && (strstr(szXTAR, "NPC") || strstr(szXTAR, "Hater") || (strstr(szXTAR, "Target") || strstr(szXTAR, "target") || strstr(szXTAR, "_TARGET") && !strstr(szXTAR, "EMPTY")) && InCombat && tSpawn.Type == SPAWN_NPC && tSpawn.Type != SPAWN_CORPSE))
						{
							add.Spawn = pSpawn;
							add.ID = pSpawn->SpawnID;
							add.Add = true;
							vAdds.push_back(add);
						}
						if (pAggroInfo)
						{
							if (pAggroInfo->aggroData[AD_xTarget1 + n].AggroPct < leastaggro)
							{
								leastaggro = pAggroInfo->aggroData[AD_xTarget1 + n].AggroPct;
								xNotTargetingMe.ID = pSpawn->SpawnID;
							}
							if (pAggroInfo->aggroData[AD_xTarget1 + n].AggroPct > mostaggro && !GetSpawnByID(xTargetingMe.ID))
							{
								mostaggro = pAggroInfo->aggroData[AD_xTarget1 + n].AggroPct;
								xTargetingMe.ID = pSpawn->SpawnID;
							}
						}
						if (pSpawn && (strstr(szXTAR, "NPC") || strstr(szXTAR, "Target") || strstr(szXTAR, "target") || strstr(szXTAR, "_TARGET") && !strstr(szXTAR, "EMPTY")) && !InCombat && tSpawn.Type == SPAWN_NPC && tSpawn.Type != SPAWN_CORPSE)
						{
							add.ID = pSpawn->SpawnID;
							add.Spawn = pSpawn;
							add.Add = false;
							vAdds.push_back(add);
						}
					}
				}
			}
		}
		catch (...)
		{
			DebugSpewAlways("MQ2Bot::CheckAdds() **Exception** 1");
		}
	}

	if (UseManualAssist && strlen(AssistName) > 3)
	{
		int doAssist = 0;
		PCHARINFO pChar = GetCharInfo();
		char assist[MAX_STRING];
		sprintf_s(assist, "/squelch /assist %s", AssistName);
		if (PSPAWNINFO pMyTarget = (PSPAWNINFO)pTarget)
		{
			if (DistanceToSpawn(pChar->pSpawn, pMyTarget) > AssistRange && MQGetTickCount64() > AssistTimer)
				doAssist = 1;
		}
		else
		{
			if (MQGetTickCount64() > AssistTimer)
				doAssist = 1;
		}
		if (doAssist)
		{
			HideDoCommand(pChar->pSpawn, assist, FromPlugin);
			AssistTimer = MQGetTickCount64() + 2000LL;
			vAdds.clear();
			InCombat = false;
			if (PSPAWNINFO pMyTarget = (PSPAWNINFO)pTarget)
			{
				if (pMyTarget->Type == SPAWN_NPC)
				{
					if (((PCPLAYERWND)pPlayerWnd))
						if (((CXWnd*)pPlayerWnd)->GetChildItem("PW_CombatStateAnim"))
							if (((PCPLAYERWND)pPlayerWnd)->CombatState == 0)
								InCombat = true;
					if (InCombat || DistanceToSpawn(pChar->pSpawn, pMyTarget) < AssistRange && PctHP(pMyTarget) < AssistAt)
					{
						_Spawns add;
						add.ID = pMyTarget->SpawnID;
						add.Spawn = pMyTarget;
						add.Add = true;
						vAdds.push_front(add);
					}
					else
					{
						_Spawns add;
						add.ID = pMyTarget->SpawnID;
						add.Spawn = pMyTarget;
						add.Add = false;
						vAdds.push_back(add);
					}
				}
			}
		}
	}
	if (UseNetBots)
	{
		sprintf_s(testAddList, "${If[${NetBots[%s].InZone} && ${NetBots.Client.Find[%s]} && ${NetBots[%s].TargetID} && ${NetBots[%s].TargetHP}<=%d && (${NetBots[%s].CombatState}==0||${NetBots[%s].Attacking}) && ${Spawn[id ${NetBots[%s].TargetID}].Distance}<=%d,1,0]}", NetBotsName, NetBotsName, NetBotsName, NetBotsName, AssistAt, NetBotsName, NetBotsName, NetBotsName, AssistRange);
		ParseMacroData(testAddList, MAX_STRING);
		if (atoi(testAddList) == 1)
		{
			sprintf_s(testAddList, "${NetBots[%s].TargetID}", NetBotsName);
			ParseMacroData(testAddList, MAX_STRING);
			bool check = false;
			int vSize = vAdds.size();
			for (int i = 0; i < vSize; i++)
			{
				if (vAdds[i].ID == (DWORD)atol(testAddList))
					check = true;
			}
			if (!check)
			{
				if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID((DWORD)atol(testAddList)))
				{
					_Spawns add;
					add.ID = pSpawn->SpawnID;
					add.Spawn = pSpawn;
					add.Add = false;
					vAdds.push_front(add);
				}
			}

		}
	}
	if (vAdds.size() && AnnounceAdds > 0)
	{
		strcpy_s(szXTAR2, "|");
		int vSize = vAdds.size();
		for (int i = 0; i < vSize; i++)
		{
			try
			{
				if (vAdds[i].Spawn && vAdds[i].Spawn->Type != SPAWN_CORPSE)
				{
					// vAdds[i].Spawn = (PSPAWNINFO)GetSpawnByID(vAdds[i].ID); // TODO: why is this commented out?
					ConColorSwap(vAdds[i].Spawn);
					sprintf_s(szXTAR, "%s%s\ar|", conColor, vAdds[i].SpawnCopy.Name);
					strcat_s(szXTAR2, szXTAR);
				}
			}
			catch (...)
			{
				DebugSpewAlways("MQ2Bot::CheckAdds() **Exception** 2");
			}

		}
		if (strcmp(AddList, szXTAR2))
		{
			strcpy_s(AddList, szXTAR2);
			WriteChatf("\arAdds: %s", szXTAR2);
		}
		if (FightX == 0 && FightY == 0 && FightZ == 0)
		{
			FightX = GetCharInfo()->pSpawn->X;
			FightY = GetCharInfo()->pSpawn->Y;
			FightZ = GetCharInfo()->pSpawn->Z;
		}
	}
	if (vAdds.size())
	{
		if (GetSpawnByID(vAdds[0].ID) && vAdds[0].ID != LastBodyID)
		{
			sprintf_s(BodyTypeFix, "${Spawn[%s].Body}", vAdds[0].SpawnCopy.Name);
			ParseMacroData(BodyTypeFix, MAX_STRING);
			LastBodyID = vAdds[0].ID;
		}
	}
	if (!vAdds.size())
	{
		summoned = false;
		FightX = 0;
		FightY = 0;
		FightZ = 0;
		WarpDistance = 0;
	}
	/* FixNote 20160728.1
	Need to check vSpawn, if not there, insert vAdd to there, also double check the vAdds[i].Add flag.
	*/
	int vSize = vAdds.size();
	for (int i = 0; i < vSize; i++)
	{
		int found = 0;
		int vSize = vSpawns.size();
		for (int x = 0; x < vSize; x++)
		{
			if (vSpawns[x].ID == vAdds[i].ID)
			{
				found++;
			}
		}
		if (!found)
			vSpawns.push_back(vAdds[i]);
	}
	SortSpawnVector(vSpawns);
}

void CheckGroup()
{
	if (!InGameOK())
		return;
	try
	{
		strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
		DebugWrite("Checking %s", CurrentRoutine); // test code
		PCHARINFO pChar = GetCharInfo();
		if (pChar && !pChar->pGroupInfo)
		{
			_Spawns gMember;
			gMember.Spawn = pChar->pSpawn;
			gMember.ID = pChar->pSpawn->SpawnID;
			gMember.State = SPAWN_PLAYER;
			gMember.PetID = 0;
			if (pChar->pSpawn->PetID && pChar->pSpawn->PetID > 0)
				gMember.PetID = pChar->pSpawn->PetID;
			vGroup[0] = gMember;
			return;
		}
		PSPAWNINFO pGroupMember;
		char test[MAX_STRING], szClass[MAX_STRING] = { 0 };
		for (int i = 0; i < 6; i++)
		{
			DebugWrite("Checking %s 5", CurrentRoutine); // test code
			sprintf_s(test, "${If[(${Group.Member[%d].Type.Equal[PC]}||${Group.Member[%d].Type.Equal[mercenary]}),1,0]}", i, i);
			ParseMacroData(test, MAX_STRING);
			if (atoi(test) == 1)
			{
				if (pChar && pChar->pGroupInfo && pChar->pGroupInfo->pMember[i] && pChar->pGroupInfo->pMember[i]->pSpawn && pChar->pGroupInfo->pMember[i]->pSpawn->SpawnID > 0)
				{
					if (pChar->pGroupInfo->pMember[i]->pSpawn->Type == SPAWN_PLAYER || pChar->pGroupInfo->pMember[i]->Mercenary)
					{
						pGroupMember = pChar->pGroupInfo->pMember[i]->pSpawn;
						if (pGroupMember && (pGroupMember->RespawnTimer || pGroupMember->StandState == STANDSTATE_DEAD))
						{
							//if (vPets[i].ID != 0)
							//	CheckGroupPets(i); //add this later
							_Spawns gMember;
							gMember.Spawn = pGroupMember;
							gMember.ID = pGroupMember->SpawnID;
							gMember.State = STANDSTATE_DEAD;
							gMember.PetID = 0;
							gMember.Friendly = true;
							strcpy_s(gMember.Class, pEverQuest->GetClassThreeLetterCode(pChar->pGroupInfo->pMember[i]->pSpawn->mActorClient.Class));
							if (pGroupMember->PetID && pGroupMember->PetID > 0)
								gMember.PetID = pGroupMember->PetID;
							vGroup[i] = gMember;
						}
						if (pGroupMember && !pGroupMember->RespawnTimer && pGroupMember->StandState != STANDSTATE_DEAD)
						{
							if (pGroupMember->SpawnID != vGroup[i].ID)
							{
								_Spawns gMember;
								gMember.Spawn = pGroupMember;
								gMember.ID = pGroupMember->SpawnID;
								gMember.State = SPAWN_PLAYER;
								gMember.PetID = 0;
								gMember.Friendly = true;
								if (pGroupMember->PetID && pGroupMember->PetID > 0)
									gMember.PetID = pGroupMember->PetID;
								vGroup[i] = gMember;
							}
						}
					}
				}
			}
			else
			{
				if (vGroup[i].ID)
				{
					_Spawns blank;
					vGroup[i] = blank;
					vGroup[i].ID = 0;
				}
			}
		}
	}
	catch (...)
	{
		DebugSpewAlways("MQ2Bot::CheckGroup() **Exception**");
	}
	DebugWrite("Checking %s 10", CurrentRoutine); // test code
	return;
}

void CheckGroupPets(int i)
{
	if (!InGameOK())
		return;
	try
	{
		strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
		DebugWrite("Checking %s", CurrentRoutine); // test code
		PCHARINFO pChar = GetCharInfo();
		if (i == 0)
		{
			if (vGroup[0].ID == pChar->pSpawn->SpawnID)
				if (!pChar->pSpawn->PetID && vPets[0].ID == 0 || pChar->pSpawn->PetID && pChar->pSpawn->PetID != 0xFFFFFFFF && vPets[0].ID == pChar->pSpawn->PetID)
					return;
				else
				{
					// FixNote 20160731.1  might need to add more members here, but for now just setting ID and spawn
					_Spawns pet;
					ZeroMemory(&vPets[0], sizeof(vPets[0]));
					if (!pChar->pSpawn->PetID)
					{
						pet.ID = 0;
						vPets[0] = pet;
					}
					else
					{
						pet.ID = pChar->pSpawn->PetID;
						pet.Spawn = (PSPAWNINFO)GetSpawnByID(pChar->pSpawn->PetID);
						pet.Friendly = true;
						vPets[0] = pet;
					}
				}
		}
		else
		{
			PSPAWNINFO pGroupMember;
			char test[MAX_STRING];
			sprintf_s(test, "${If[(${Group.Member[%d].Type.Equal[PC]}||${Group.Member[%d].Type.Equal[mercenary]}),1,0]}", i, i);
			ParseMacroData(test, MAX_STRING);
			if (atoi(test) == 1)
			{
				if (pChar && pChar->pGroupInfo && pChar->pGroupInfo->pMember[i] && pChar->pGroupInfo->pMember[i]->pSpawn && pChar->pGroupInfo->pMember[i]->pSpawn->SpawnID > 0)
				{
					if (pChar->pGroupInfo->pMember[i]->pSpawn->Type == SPAWN_PLAYER || pChar->pGroupInfo->pMember[i]->Mercenary)
					{
						pGroupMember = pChar->pGroupInfo->pMember[i]->pSpawn;
						if (pGroupMember && (pGroupMember->RespawnTimer || pGroupMember->StandState == STANDSTATE_DEAD))
						{
							if (!pChar->pSpawn->PetID && vPets[i].ID == 0 || pChar->pSpawn->PetID && pChar->pSpawn->PetID != 0xFFFFFFFF && vPets[i].ID == pChar->pSpawn->PetID)
								return;
							else
							{
								// FixNote 20160731.1  might need to add more members here, but for now just setting ID and spawn
								_Spawns pet;
								ZeroMemory(&vPets[i], sizeof(vPets[i]));
								if (!pChar->pSpawn->PetID)
								{
									pet.ID = 0;
									vPets[i] = pet;
								}
								else
								{
									pet.ID = pChar->pSpawn->PetID;
									pet.Spawn = (PSPAWNINFO)GetSpawnByID(pChar->pSpawn->PetID);
									pet.Friendly = true;
									vPets[i] = pet;
								}
							}
						}
					}
				}
			}
		}
	}
	catch (...)
	{
		DebugSpewAlways("MQ2Bot::CheckGroupPets() **Exception**");
	}
	return;
}


#pragma endregion SpawnFunctionDefinitions

#pragma region SpellFunctionDefinitions

BOOL AAReady(DWORD index)
{
	int result = 0;
	if (pAltAdvManager)
		if (index)
			if (PALTABILITY ability = pAltAdvManager->GetAAById(index))
			{
				if (pAltAdvManager->GetCalculatedTimer(pPCData, ability) > 0)
					pAltAdvManager->IsAbilityReady(pPCData, ability, &result);
			}
	return (result < 0);
}

BOOL DiscReady(char disc[MAX_STRING])
{
	for (unsigned long nCombatAbility = 0; nCombatAbility < NUM_COMBAT_ABILITIES; nCombatAbility++)
	{
		if (pCombatSkillsSelectWnd->ShouldDisplayThisSkill(nCombatAbility)) {
			if (PSPELL pSpell = GetSpellByID(pPCData->GetCombatAbility(nCombatAbility)))
			{
				if (!_stricmp(disc, pSpell->Name))
				{
					DWORD timeNow = (DWORD)time(NULL);
					if (pPCData->GetCombatAbilityTimer(pSpell->ReuseTimerIndex, pSpell->SpellGroup) < timeNow)
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

BOOL ItemReady(DWORD itemID)
{
	if (PCONTENTS pCont = FindItemByID(itemID))
	{
		if (PITEMINFO pIteminf = GetItemFromContents(pCont))
		{
			if (pIteminf->Clicky.TimerID != -1)
			{
				DWORD timer = GetItemTimer(pCont);
				if (timer == 0)
				{
					return true;
				}
				else if (pIteminf->Clicky.SpellID != -1)
				{
					return true; // insta-click or instant recast
				}
			}
		}
	}
	return false;
}

BOOL SpellReady(DWORD index)
{
	if (PSPELL mySpell = GetSpellByID(index))
	{
		for (unsigned long nGem = 0; nGem < NUM_SPELL_GEMS; nGem++)
		{
			if (PSPELL pSpell = GetSpellByID(GetMemorizedSpell(nGem)))
			{
				if (!_stricmp(mySpell->Name, pSpell->Name))
					if (((PCDISPLAY)pDisplay)->TimeStamp >((PSPAWNINFO)pLocalPlayer)->SpellGemETA[nGem] && (int)((PCDISPLAY)pDisplay)->TimeStamp > ((PSPAWNINFO)pLocalPlayer)->GetSpellCooldownETA()) {
						return true;
					}
			}
		}
	}
	return false;
}

void BotCastCommand(_BotSpell& spell) {
	return;
}

/*
typedef struct _Spawns
{
	vector<_BotSpell>	vSpellList;					// list of what they have? was going to use as bufflist but maybe it is deprecated
	vector<_Buff>		vBuffList;					// tracks buffs on this character (mine or others)
	char				SpawnBuffList[MAX_STRING];	// might want string version? idk maybe not
	char				Class[MAX_STRING];			// class short name for buff purposes.
	DWORD				ID;							// ID
	int					PetID;						// ID of any pet
	bool				Add;						// is this a confirmed add?
	bool				Friendly;					// is this a friendly?
	PSPAWNINFO			Spawn;						// actual spawn entry, might dump this due to crashes in original bot
	SPAWNINFO			SpawnCopy;					// use this copy instead maybe
	int					Priority;					// priority of spawn. can assign for adds or for spells id think.
	bool				NeedsCheck;					// do i need to check this spawn for buffs/mez/whatever
	ULONGLONG			LastChecked;				// timestamp of last check
	BYTE				State;
} Spawns, *PSpawns, SpawnCopy;
*/
void BuildSpawn(_Spawns &spawn)
{

}

void BuildSpell(_BotSpell &spell)
{
	bool bFound = false;
	// blank out the id value
	storeID = 0;
	//WriteChatf("Buildspell: %s", spell.Name);
	// Figure out type of data
	SkillType(spell.Name);
	spell.Spell = storeSpell;
	spell.SpellIcon = storeSpell->SpellIcon;
	if(!spell.Type)
		spell.Type = storeType;
	spell.ID = storeID;
	spell.Prestige = storePrestige;
	spell.RequiredLevel = storeRequiredLevel;
	sprintf_s(spell.Gem, storeGem);

	//check the loadbot stuff
	if (spell.Type == TYPE_DISC)
		DiscCategory(spell.Spell);
	else
		SpellType(spell.Spell);
	strcpy_s(spell.SpellCat, spellCat);
	spell.BuffType = buffType;
	if(strlen(spell.SpellType)<2)
		strcpy_s(spell.SpellType, spellType);
	if (!spell.SpellTypeOption)
		spell.SpellTypeOption = spellTypeOption;
	if(!spell.CheckFunc)
		spell.CheckFunc = spellFunc;

	// check the ini otherwise use default settings
	char szTemp[MAX_STRING] = { 0 }, szSpell[MAX_STRING], szClasses[MAX_STRING] = { 0 }, spellNum[10] = { 0 }, color[10] = { 0 }, szRank2[MAX_STRING] = { 0 }, szRank3[MAX_STRING] = { 0 };
	int customSpells = GetPrivateProfileInt(INISection, "SpellTotal", 0, INIFileName);
	int defStartAt = 0, defUse = 0, defStopAt = 0, defPriority = 0, defNamedOnly = 0, defUseOnce = 0, defForceCast = 0, defUseInCombat=0;
	int found = 0;
	for (int i = 0; DefaultSection[i]; i++)
	{
		if (!_stricmp(spell.SpellType, DefaultSection[i]))
		{
			strcpy_s(color, DefaultColor[i]);
			defStartAt = atoi(DefaultStartAt[i]);
			defUse = atoi(DefaultUse[i]);
			defStopAt = atoi(DefaultStopAt[i]);
			defPriority = atoi(DefaultPriority[i]);
			defNamedOnly = atoi(DefaultNamedOnly[i]);
			defUseOnce = atoi(DefaultUseOnce[i]);
			defForceCast = atoi(DefaultForceCast[i]);
			defUseInCombat = atoi(DefaultUseInCombat[i]);
			found++;
			break;
		}
	}
	if (!found)
	{
		WriteChatf("Somethings fucky, bois. SpellType %s for spell: %s. is unknown.", spell.SpellType, spell.Name);
		defStartAt = 0;
		defUse = 1;
		defStopAt = 0;
		defPriority = 0;
		defNamedOnly = 0;
		defUseOnce = 0;
		defForceCast = 0;
		defUseInCombat =0;
	}
	// actually search for it
	for (int i = 1; i < customSpells; i++)
	{
		sprintf_s(szSpell, "Spell%dName", i);
		if (GetPrivateProfileString(INISection, szSpell, NULL, szTemp, MAX_STRING, INIFileName))
		{
			// see if it is a match
			// check 3 ranks
			sprintf_s(szRank2, "%s Rk. II", szTemp);
			sprintf_s(szRank3, "%s Rk. II", szTemp);
			if (!_stricmp(szTemp, spell.Name) || !_stricmp(szRank2, spell.Name) || !_stricmp(szRank3, spell.Name))
			{
				bFound = true;
				// great it is a match, let's load any non-default values
				spell.IniMatch = i;
				strcpy_s(spell.Color, color);
				sprintf_s(szSpell, "Spell%dIf", spell.IniMatch);
				if (GetPrivateProfileString(INISection, szSpell, "1", szTemp, MAX_STRING, INIFileName))
				{
					strcpy_s(spell.If, szTemp);
				}
				sprintf_s(szSpell, "Spell%dGem", spell.IniMatch);
				if (GetPrivateProfileString(INISection, szSpell, spell.Gem, szTemp, MAX_STRING, INIFileName)) // defaults to existing gem if already memmed
				{
					strcpy_s(spell.Gem, szTemp);
				}
				sprintf_s(szSpell, "Spell%dUseOnce", spell.IniMatch);
				spell.UseOnce = GetPrivateProfileInt(INISection, szSpell, defUseOnce, INIFileName);
				sprintf_s(szSpell, "Spell%dForceCast", spell.IniMatch);
				spell.ForceCast = GetPrivateProfileInt(INISection, szSpell, defForceCast, INIFileName);
				sprintf_s(szSpell, "Spell%dUse", spell.IniMatch);
				spell.Use = GetPrivateProfileInt(INISection, szSpell, defUse, INIFileName);
				sprintf_s(szSpell, "Spell%dStartAt", spell.IniMatch);
				spell.StartAt = GetPrivateProfileInt(INISection, szSpell, defStartAt, INIFileName);
				sprintf_s(szSpell, "Spell%dStopAt", spell.IniMatch);
				spell.StopAt = GetPrivateProfileInt(INISection, szSpell, defStopAt, INIFileName);
				sprintf_s(szSpell, "Spell%dNamedOnly", spell.IniMatch);
				spell.NamedOnly = GetPrivateProfileInt(INISection, szSpell, defNamedOnly, INIFileName);
				sprintf_s(szSpell, "Spell%dPriority", spell.IniMatch);
				spell.Priority = GetPrivateProfileInt(INISection, szSpell, defPriority, INIFileName);
				sprintf_s(szSpell, "Spell%dUseInCombat", spell.IniMatch);
				spell.UseInCombat = GetPrivateProfileInt(INISection, szSpell, defUseInCombat, INIFileName);
				sprintf_s(szSpell, "|%s|", spell.SpellType);
				if (StrStrIA(BuffClasses, szSpell))
				{
					sprintf_s(szClasses, "%sClasses", spell.SpellType);
					GetPrivateProfileString(INISection, szClasses, "ALL", szTemp, MAX_STRING, INIFileName);
					strcpy_s(spell.Classes,szTemp);
				}
			}
		}
	}
	// but what if we didnt find it? guess we better use some defaults, Timmy
	if (!bFound)
	{
		strcpy_s(spell.Color, color);
		spell.UseOnce = defUseOnce;
		spell.ForceCast = defForceCast;
		spell.Use = 1;
		spell.StartAt = defStartAt;
		spell.StopAt = defStopAt;
		spell.NamedOnly = defNamedOnly;
		spell.Priority = defPriority;
		spell.UseInCombat = defUseInCombat;
		sprintf_s(spell.If, "1");
	}
	// set some defaults
	spell.LastTargetID = 0;
	spell.LastCast = 0;
	spell.Recast = 0;
	spell.TargetID = 0;
}

int CalcDuration(PSPELL pSpell)
{
	float fCalc = 1.0;
	int iCalc = 0;
	if (!pSpell)
		return 0;
	iCalc = GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer);
	if (pSpell->SpellType == 0) {
		fCalc = fCalc + detDurExtend;
		if (iCalc > 3 && pSpell->Category == 20)
			iCalc = (int)botround(fCalc) + dotExtend;
		else
			iCalc = (int)botround(fCalc);
		if (iCalc > 3 && pSpell->Category == 31)
			iCalc = (int)botround(fCalc) + mezExtend;
		else
			iCalc = (int)botround(fCalc);
	}
	if (pSpell->SpellType == 1) {
		fCalc = fCalc + benDurExtend + reinforcement;
		if (iCalc > 3 && pSpell->Subcategory == 32)
			iCalc = (int)botround(fCalc) + hotExtend;
		else
			iCalc = (int)botround(fCalc);
		if (iCalc > 3 && pSpell->Subcategory == 140)
			iCalc = 4;
	}
	return iCalc;
}

bool CanICast(_BotSpell &spell)
{
	if (!InGameOK())
		return false;
	if (spell.Type == TYPE_AA)
		if (!AAReady(spell.ID))
			return false;
	if (spell.Type == TYPE_SPELL)
		if (!SpellReady(spell.ID))
			return false;
	if (spell.Type == TYPE_ITEM)
		if (!ItemReady(spell.ID))
			return false;
	if (spell.Type == TYPE_DISC)
		if (!DiscReady(spell.Name))
			return false;
	if (!spell.Spell->SpellType)
		if (!ValidDet(spell.Spell, (PSPAWNINFO)GetSpawnByID(spell.MyTarget.ID)))
			return false;
	if (spell.Spell->SpellType)
		if (!ValidBen(spell.Spell, (PSPAWNINFO)GetSpawnByID(spell.MyTarget.ID)))
			return false;
	return true;
}

void CheckMemmedSpells()
{
	if (!InGameOK())
		return;
	bool change = false;
	bool bAlreadyInMaster = false;
	bool bFound=false;
	try
	{
		if (pDisplay && pLocalPlayer)
		{
			for (unsigned long nGem = 0; nGem < NUM_SPELL_GEMS; nGem++)
			{
				if (GetMemorizedSpell(nGem) != 0xFFFFFFFF)
				{
					if (PSPELL pSpell = GetSpellByID(GetMemorizedSpell(nGem)))
					{
						if (nGem < vMemmedSpells.size() && pSpell->ID != vMemmedSpells[nGem].ID && vMemmedSpells[nGem].GroupID!=pSpell->SpellGroup)
						{
							// change detected
							change = true;
							//WriteChatf("\arMQ2Bot\aw::\amUnknown: \aw%s", pSpell->Name);
							int vSize = vMaster.size();

							// Check if it is in master already
							for (int i = 0; i<vSize; i++)
							{
								if (!_stricmp(vMaster[i].Name,pSpell->Name))
								{
									bAlreadyInMaster = true; //cool, i can skip my happy ass way down there then
									WriteChatf("\arMQ2Bot\aw::\amKnown: \aw%s (%s)", vMemmedSpells[nGem].Name, vMemmedSpells[nGem].SpellCat);
								}
							}
							if (!bAlreadyInMaster)
							{
								// not in master so let's build it
								strcpy_s(vMemmedSpells[nGem].Name, pSpell->Name);
								//store the old data
								vMemmedSpells[nGem].PreviousID = vMemmedSpells[nGem].ID;
								//build the new data
								BuildSpell(vMemmedSpells[nGem]);
								vMemmedSpells[nGem].ID = vMemmedSpells[nGem].ID;

								if (vMemmedSpells[nGem].SpellTypeOption != ::OPTIONS::ZERO)
									WriteChatf("\arMQ2Bot\aw::\ayAdded: \aw%s (%s)", vMemmedSpells[nGem].Name, vMemmedSpells[nGem].SpellCat);
								else
									WriteChatf("\arMQ2Bot\aw::\amDetected: \aw%s (%s)", vMemmedSpells[nGem].Name, vMemmedSpells[nGem].SpellCat);
								if (vMemmedSpells[nGem].SpellTypeOption != ::OPTIONS::ZERO)
									vMaster.push_back(vMemmedSpells[nGem]);
							}
						}
					}
				}
				else
				{
					//WriteChatf("\arMQ2Bot\aw::\aoRemoved: \aw(%s)", vMemmedSpells[nGem].Name);
					if (nGem < vMemmedSpells.size() && vMemmedSpells[nGem].ID)
					{
						int vSize = vMaster.size();
						for (int i = 0; i < vSize; i++)
						{
							if (vMaster[i].ID == vMemmedSpells[nGem].ID)
							{
								WriteChatf("\arMQ2Bot\aw::\aoRemoved: \aw(%s)", vMemmedSpells[nGem].Name);
								vMaster.erase(vMaster.begin() + i);
								//BotSpell spell;
								vMemmedSpells[nGem].Spell = NULL;
								vMemmedSpells[nGem].ID = 0;
								change = true;
								break;
							}
						}
					}
				}

			}
		}
		if (change) {
			SortSpellVector(vMaster);
		}
	}
	catch (...)
	{
		DebugSpewAlways("MQ2Bot::CheckMemmedSpells() **Exception**");
	}
}

PSPELL CheckTrigger(PSPELL pSpell, int index)
{
	PSPELL pmyspell = pSpell;
	bool spafound = false;
	if (IsSPAEffect(pSpell, SPA_TRIGGER_BEST_SPELL_GROUP))
	{
		spafound = true;
	}
	if (pSpellMgr && spafound)
	{
		long numeffects = GetSpellNumEffects(pSpell);
		if (numeffects > index) {
			if (long groupid = GetSpellBase2(pmyspell, index)) {
				//PSPELL pTrigger = (PSPELL)pSpellMgr->GetSpellByGroupAndRank(groupid, pmyspell->SpellSubGroup, pmyspell->SpellRank, true);
				PSPELL pTrigger = (PSPELL)pSpellMgr->GetSpellByGroupAndRank(groupid, 0, pmyspell->SpellRank, true);
				return pTrigger;
			}
		}
	}
	return pSpell;
}

void CustomSpells() //this only builds initial list
{
	vCustomSpells.clear();
	char szTemp[MAX_STRING] = { 0 }, szSpell[MAX_STRING] = { 0 }, color[10] = { 0 };
	bool bDuplicate = false;
	int customSpells = GetPrivateProfileInt(INISection, "SpellTotal", 0, INIFileName);
	WriteChatf("DEBUG: %d spells detected in INI", customSpells);
	for (int i = 0; i < customSpells; i++)
	{
		sprintf_s(szSpell, "Spell%dName", i);
		if (GetPrivateProfileString(INISection, szSpell, NULL, szTemp, MAX_STRING, INIFileName))
		{
			//found something, let's see if it is valid
			BotSpell spell;
			strcpy_s(spell.Name, szTemp);
			BuildSpell(spell);
			if (spell.ID > 0)
			{
				//WriteChatf("\arFOUND: \aw%s\ag=\aw%s \ardetected in INI.", szSpell, spell.Name);
				//verify it isnt already on our list
				if (vCustomSpells.size())
				{
					int vSize = vCustomSpells.size();
					for (int x = 0; x < vSize; x++)
					{
						if (spell.ID == vCustomSpells[x].ID)
							bDuplicate = true;
					}
					if (!bDuplicate)
					{
						//not a duplicate so lets add it to custom and master
						vCustomSpells.push_back(spell);
						vMaster.push_back(spell);
					}
				}
			}
			else
			{
				WriteChatf("\arERROR: \aw%s\ag=\aw%s \ardetected in INI but is spelled wrong or isn't valid for your character.", szSpell, szTemp);
			}
		}
	}
}

void DiscCategory(PSPELL pSpell)
{
	strcpy_s(spellCat, "Unknown");
	if (!pSpell)
		return;
	if (DWORD cat = pSpell->Subcategory) {
		if (PVOID ptr = pCDBStr->GetString(cat, 5, NULL)) {
			sprintf_s(spellCat, "%s", pCDBStr->GetString(cat, 5, NULL));
		}
	}

	int attrib0 = GetSpellAttribX(pSpell, 0);

	if (pSpell->Subcategory == 152)
	{
		strcpy_s(spellCat, "Aggro");
		spellTypeOption = ::OPTIONS::AGGRO; 
		spellFunc = CheckAggro;
		return;
	}
	if (attrib0 == 189)
	{
		strcpy_s(spellCat, "End Regen");
		return;
	}
	if (attrib0 == 31 || pSpell->Subcategory == 35 || pSpell->Category == 35)
	{
		strcpy_s(spellCat, "Mez");
		spellTypeOption = ::OPTIONS::MEZ; 
		spellFunc = CheckMez;  
		buffType = ::BUFFTYPE::Mez;
		return;
	}
	if (attrib0 == 86)
	{
		strcpy_s(spellCat, "Pacify");
		return;
	}
	if (attrib0 == 180)
	{
		strcpy_s(spellCat, "Spell Block");
		return;
	}
	if (!strcmp(szSkills[pSpell->Skill], "Throwing"))
	{
		strcpy_s(spellCat, "Throwing");
		return;
	}
	if (strstr(pSpell->Name, "s Aura"))
	{
		strcpy_s(spellCat, "Aura");
		spellTypeOption = ::OPTIONS::AURA;
		spellFunc = CheckAura;
		return;
	}
	if (HasSpellAttrib(pSpell, 220) || strstr(pSpell->Name, "Dichotomic"))
	{
		strcpy_s(spellCat, "Offense");
		return;
	}
	if ((attrib0 == 0 && GetSpellBase(pSpell, 0) < 0) || attrib0 == 300 || attrib0 == 200)
	{
		strcpy_s(spellCat, "Offense");
		return;
	}
	for (int slot = 0; slot < GetSpellNumEffects(pSpell); ++slot)
	{
		int attrib = GetSpellAttrib(pSpell, slot);
		if (attrib == 169 || attrib == 171 || attrib == 182 || attrib == 183 || attrib == 184
			|| attrib == 185 || attrib == 186 || attrib == 193 || attrib == 196)
		{
			strcpy_s(spellCat, "Offense");
			return;
		}
		if (attrib == 162 || attrib == 168 || attrib == 172 || attrib == 174
			|| attrib == 175 || attrib == 188 || attrib == 320)
		{
			strcpy_s(spellCat, "Defense");
			return;
		}
	}
	if (attrib0 == 147)
	{
		strcpy_s(spellCat, "OOC Heal");
		return;
	}
	if (attrib0 == 181)
	{
		strcpy_s(spellCat, "Fearless");
		return;
	}
	if (attrib0 == 227 && GetSpellBase2(pSpell, 0) == 8)
	{
		strcpy_s(spellCat, "Offense");
		return;
	}
	if (attrib0 == 340 || attrib0 == 374 || attrib0 == 323)
	{
		if (PSPELL pSpell2 = GetSpellByID(GetSpellBase2(pSpell, 0)))
		{
			if (GetSpellAttribX(pSpell2, 0) == 328 || HasSpellAttrib(pSpell, 162))
			{
				strcpy_s(spellCat, "Defense");
				return;
			}
			if (HasSpellAttrib(pSpell2, 85))
			{
				strcpy_s(spellCat, "Combat Innates");
				return;
			}
			if (HasSpellAttrib(pSpell2, 184))
			{
				strcpy_s(spellCat, "Offense");
				return;
			}
		}
	}
	if (attrib0 == 374 || attrib0 == 85)
	{
		if (PSPELL pSpell2 = GetSpellByID(GetSpellBase2(pSpell, 0)))
		{
			int attrib0_ = GetSpellAttribX(pSpell2, 0);

			if (attrib0_ == 444)
			{
				strcpy_s(spellCat, "Aggro");
				spellTypeOption = ::OPTIONS::AGGRO; 
				spellFunc = CheckAggro;
				return;
			}
			if (HasSpellAttrib(pSpell2, 192))
			{
				strcpy_s(spellCat, "Aggro");
				spellTypeOption = ::OPTIONS::AGGRO; 
				spellFunc = CheckAggro;
				return;
			}
			if (attrib0_ == 92 && GetSpellBase(pSpell2, 0) < 0)
			{
				strcpy_s(spellCat, "Jolt");
				spellTypeOption = ::OPTIONS::JOLT; 
				spellFunc = CheckJolt; 
				return;
			}
		}
	}
	if (HasSpellAttrib(pSpell, 192))
	{
		strcpy_s(spellCat, "Aggro");
		spellTypeOption = ::OPTIONS::AGGRO; 
		spellFunc = CheckAggro;
		return;
	}
	if (strstr(pSpell->Name, "Unholy Aura"))
	{
		strcpy_s(spellCat, "Lifetap");
		spellTypeOption = ::OPTIONS::LIFETAP;
		spellFunc = CheckLifetap;
		return;
	}
}

//bool FindBuff(PSPELL pSpell, vector<_Buff>& v)
//{
//	char szRank2[MAX_STRING] = { 0 }, szRank3[MAX_STRING] = { 0 };
//	int vSize = v.size();
//	for (int i = 0; i < vSize; i++)
//	{
//		sprintf_s(szRank2, "%s Rk. II", pSpell->Name);
//		sprintf_s(szRank3, "%s Rk. II", pSpell->Name);
//		if (v[i].BotSpell.Spell == pSpell||_stricmp(szRank2, v[i].BotSpell.Spell->Name)|| _stricmp(szRank3, v[i].BotSpell.Spell->Name))
//			return true;
//	}
//	return false;
//}

bool FindSpell(PSPELL pSpell, vector<_BotSpell> &v)
{
	int vSize = v.size();
	for (int i = 0; i < vSize; i++)
	{
		if (v[i].Spell == pSpell)
			return true;
	}
	return false;
}

int FindSpellInfoForAttrib(PSPELL pSpell, ATTRIBSLOTS what, int attrib)
{
	int slot = FindSpellAttrib(pSpell, attrib);

	if (slot == -1)
		return 0;

	switch (what)
	{
	case ATTRIB:
		return slot;
	case BASE:
		return GetSpellBase(pSpell, slot);
	case BASE2:
		return GetSpellBase2(pSpell, slot);
	case CALC:
		return GetSpellCalc(pSpell, slot);
	case MAX:
		return GetSpellMax(pSpell, slot);
	}

	return 0;
}

DWORD GetSpellDuration2(PSPELL pSpell)
{
	if (!InGameOK())
		return 0;
	if (!pSpell)
		return 0;
	int pSpellDuration = 0;
	pSpellDuration = GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer);
	for (int i = 0; i < GetSpellNumEffects(pSpell); i++)
	{
		int attrib = GetSpellAttrib(pSpell, i);

		if (attrib == 323 || attrib == 374 || attrib == 419 || attrib == 339 || attrib == 85 || attrib == 340)
		{
			PSPELL pSpell2 = GetSpellByID(GetSpellBase2(pSpell, i));
			int duration = pSpell2 ? GetSpellDuration(pSpell2, (PSPAWNINFO)pLocalPlayer) : 0;

			if (duration > pSpellDuration)
				pSpellDuration = duration;
		}
	}
	return pSpellDuration;
}

bool ShouldICast(_BotSpell &spell)
{
	if (spell.If[0] != '\0')
	{
		char sSpell[MAX_STRING] = { 0 };
		sprintf_s(sSpell, "${If[(%s),1,0]}", SpellIt->second.c_str());
		ParseMacroData(sSpell, MAX_STRING);
		if (atoi(sSpell) == 0)
			return false;
	}
	if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(spell.MyTarget.ID))
	{
		if(spell.MyTarget.Add)
			if (spell.NamedOnly && !IsNamed(pSpawn) || PctHP(pSpawn) > AssistAt || PctHP(pSpawn) > spell.StartAt || PctHP(pSpawn) <= spell.StopAt || spell.UseOnce && spell.LastTargetID == pSpawn->SpawnID)
				return false;
		if (MQGetTickCount64() < SpellTimer || !GetCharInfo()->pSpawn || (GetCharInfo()->pSpawn->CastingData.SpellETA - GetCharInfo()->pSpawn->TimeStamp) > 0 && (GetCharInfo()->pSpawn->CastingData.SpellETA - GetCharInfo()->pSpawn->TimeStamp) < 30000)
			return false;
	}
	return true;
}

bool ShouldICastDetrimental(_BotSpell &spell)
{
	if (!vAdds.size())
		return false;
	if (!GetSpawnByID(vAdds[0].ID))
		return false;
	if (GetDistance(vAdds[0].Spawn->X, vAdds[0].Spawn->Y) > AssistRange)
		return false;
	return true;
}

bool sortByPriority(const BotSpell &lhs, const BotSpell &rhs) { return lhs.Priority > rhs.Priority; } // Sort spell vectors by priority

void SortSpellVector(vector<_BotSpell> &v)  // the actual sorting of a spell vector by priority
{
	sort(v.begin(), v.end(), sortByPriority);
}

void SkillType(char szName[MAX_STRING])
{
	bool bFound = false;
	//check AAs
	int aaIndex;
	PALTABILITY aa;
	aaIndex = GetAAIndexByName(szName);
	if (aaIndex > 0)
	{
		aa = pAltAdvManager->GetAAById(aaIndex);
		if (aa && GetSpellByID(aa->SpellID))
		{
			storeSpell = GetSpellByID(aa->SpellID);
			strcpy_s(storeName, szName);
			storeType = TYPE_AA;
			storeID = aa->SpellID;
			strcpy_s(storeGem, "alt");
			storePrestige = false;
			storeRequiredLevel = 0; //TODO: i dont know if this matters yet. it might, in which case this should get changed to be accurate
			bFound = true;
		}
	}
	if (!bFound)
	{
		//check discs
		for (unsigned long nCombatAbility = 0; nCombatAbility < NUM_COMBAT_ABILITIES; nCombatAbility++)
		{
			if (pCombatSkillsSelectWnd->ShouldDisplayThisSkill(nCombatAbility))
			{
				if (PSPELL pSpell = GetSpellByID(pPCData->GetCombatAbility(nCombatAbility)))
				{
					if (strstr(pSpell->Name, szName))
					{
						storeSpell = pSpell;
						strcpy_s(storeName, szName);
						storeType = TYPE_DISC;
						storeID = pSpell->ID;
						strcpy_s(storeGem, "disc");
						storePrestige = false;
						storeRequiredLevel = 0; //TODO: i dont know if this matters yet. it might, in which case this should get changed to be accurate
						bFound = true;
					}
				}
			}
		}
	}
	if (!bFound)
	{
		//check spells
		if (PSPELL pSpell = GetSpellByName(szName))
		{
			storeSpell = pSpell;
			strcpy_s(storeName, szName);
			storeType = TYPE_SPELL;
			storeID = pSpell->ID;
			strcpy_s(storeGem, "1"); // set to default value in case it isnt memmed
			for (unsigned long nGem = 0; nGem < NUM_SPELL_GEMS; nGem++)
			{
				if (GetMemorizedSpell(nGem) != 0xFFFFFFFF)
				{
					if (PSPELL gemSpell = GetSpellByID(GetMemorizedSpell(nGem)))
					{
						if (gemSpell == pSpell)
						{
							nGem = nGem + 1; // set real gem value if found
							char szTemp[10];
							_itoa_s(nGem, szTemp, 10);
							strcpy_s(storeGem, szTemp);
							storePrestige = false;
							storeRequiredLevel = 0; //TODO: i dont know if this matters yet. it might, in which case this should get changed to be accurate
							bFound = true;
						}
					}
				}
			}
		}
	}
	if (!bFound)
	{
		//check items
		char szItem[MAX_STRING] = { 0 };
		sprintf_s(szItem, "${If[${FindItem[=%s].ID},1,0]}", szName);
		ParseMacroData(szItem, MAX_STRING);
		if (atoi(szItem) == 1)
		{
			PCONTENTS pCont = 0;
			if (pDisplay && pLocalPlayer)
			{
				BOOL bExact = true;
				pCont = FindItemByName(szName, bExact);
				if (pCont) {
					if (PITEMINFO pIteminf = GetItemFromContents(pCont)) {
						if (pIteminf->Clicky.SpellID != -1) {
							strcpy_s(storeName, szName);
							storeSpell = GetSpellByID(pIteminf->Clicky.SpellID);
							storeType = TYPE_ITEM;
							strcpy_s(storeGem, "item");
							storeID = pIteminf->ItemNumber;
							storePrestige = pIteminf->Prestige;
							storeRequiredLevel = pIteminf->RequiredLevel;
							bFound = true;
						}
					}
				}
			}
		}
	}
}

void SpellCategory(PSPELL pSpell)
{
	strcpy_s(spellCat, "Unknown");
	if (!pSpell)
		return;
	if (DWORD cat = pSpell->Subcategory) {
		if (PVOID ptr = pCDBStr->GetString(cat, 5, NULL)) {
			sprintf_s(spellCat, "%s", pCDBStr->GetString(cat, 5, NULL));
		}
	}
	if (pSpell->Subcategory == 81 && strstr(pSpell->Name, "Mal") || pSpell->SpellIcon == 55)
	{
		strcpy_s(spellCat, "Malo");
		buffType = ::BUFFTYPE::Malo;
		return;
	}
	if ((pSpell->Subcategory == 81 || pSpell->SpellIcon == 72) && strstr(pSpell->Name, "Tash"))
	{
		strcpy_s(spellCat, "Tash");
		buffType = ::BUFFTYPE::Tash;
		return;
	}
	if (pSpell->SpellIcon == 17 || strstr(pSpell->Name, "Helix of the Undying") || strstr(pSpell->Name, "Deep Sleep's Malaise"))
	{
		strcpy_s(spellCat, "Slow");
		buffType = ::BUFFTYPE::Slow;
		return;
	}
	if (pSpell->SpellIcon == 5 || pSpell->SpellIcon == 160 && strstr(pSpell->Name, "Darkness"))
	{
		strcpy_s(spellCat, "Snare");
		buffType = ::BUFFTYPE::Snare;
		return;
	}
	if (pSpell->SpellIcon == 117)
	{
		strcpy_s(spellCat, "Root");
		buffType = ::BUFFTYPE::Root;
		return;
	}
	if (pSpell->Subcategory == 35 || pSpell->SpellIcon == 25)
	{
		strcpy_s(spellCat, "Mez");
		buffType = ::BUFFTYPE::Mez;
		return;
	}
	if (pSpell->Subcategory == 30 || pSpell->SpellIcon == 50)
	{
		strcpy_s(spellCat, "Cripple");
		buffType = ::BUFFTYPE::Cripple;
		return;
	}
	if (pSpell->SpellIcon == 134 || pSpell->SpellIcon == 16)
	{
		strcpy_s(spellCat, "Haste");
		buffType = ::BUFFTYPE::Haste;
		return;
	}
	if (pSpell->SpellIcon == 132 && !strstr(pSpell->Name, "Growth"))
	{
		strcpy_s(spellCat, "Aego");
		buffType = ::BUFFTYPE::Aego;
		return;
	}
	if (pSpell->SpellIcon == 131 && pSpell->Subcategory == 46)
	{
		strcpy_s(spellCat, "Skin");
		buffType = ::BUFFTYPE::Skin;
		return;
	}
	if (pSpell->SpellIcon == 130 || pSpell->SpellIcon == 133 && strstr(pSpell->Name, "Shield of") || pSpell->SpellIcon == 133 && strstr(pSpell->Name, "Armor of the"))
	{
		strcpy_s(spellCat, "Focus");
		buffType = ::BUFFTYPE::Focus;
		return;
	}
	if (pSpell->SpellIcon == 118 && pSpell->Subcategory == 43)
	{
		strcpy_s(spellCat, "Regen");
		buffType = ::BUFFTYPE::Regen;
		return;
	}
	if (pSpell->SpellIcon == 150 && pSpell->Subcategory == 112 && pSpell->Category == 45)
	{
		strcpy_s(spellCat, "Symbol");
		buffType = ::BUFFTYPE::Symbol;
		return;
	}
	if (pSpell->SpellIcon == 21 && !strstr(pSpell->Name, "Knowledge") && !strstr(pSpell->Name, "Recourse") && !strstr(pSpell->Name, "Soothing") && !strstr(pSpell->Name, "Brell") && pSpell->Subcategory == 59 && pSpell->Category == 79)
	{
		strcpy_s(spellCat, "Clarity");
		buffType = ::BUFFTYPE::Clarity;
		return;
	}
	if (pSpell->SpellIcon == 123 && strstr(pSpell->Name, "of the Predator") || pSpell->SpellIcon == 158 && strstr(pSpell->Name, "Protection of the"))
	{
		strcpy_s(spellCat, "Pred");
		buffType = ::BUFFTYPE::Pred;
		return;
	}
	if (pSpell->SpellIcon == 123 && strstr(pSpell->Name, "Strength of the") || pSpell->SpellIcon == 158 && strstr(pSpell->Name, "Protection of the"))
	{
		strcpy_s(spellCat, "Strength");
		buffType = ::BUFFTYPE::Strength;
		return;
	}
	if (pSpell->SpellIcon == 90 && strstr(pSpell->Name, "Brell's"))
	{
		strcpy_s(spellCat, "Brells");
		buffType = ::BUFFTYPE::Brells;
		return;
	}
	if (pSpell->SpellIcon == 1 && strstr(pSpell->Name, "Spiritual V"))
	{
		strcpy_s(spellCat, "SV");
		buffType = ::BUFFTYPE::SV;
		return;
	}
	if (pSpell->SpellIcon == 72 && strstr(pSpell->Name, "Spiritual E"))
	{
		strcpy_s(spellCat, "SE");
		buffType = ::BUFFTYPE::SE;
		return;
	}
	if (pSpell->SpellIcon == 132 && (strstr(pSpell->Name, "Growth") || strstr(pSpell->Name, "Stance")))
	{
		strcpy_s(spellCat, "Growth");
		buffType = ::BUFFTYPE::Growth;
		return;
	}
	if (pSpell->SpellIcon == 70 && strstr(pSpell->Name, "Shining"))
	{
		strcpy_s(spellCat, "Shining");
		buffType = ::BUFFTYPE::Shining;
		return;
	}
	if (pSpell->SpellType != 0 && pSpell->Category == 125 && pSpell->Subcategory == 21 && pSpell->TargetType != 6)
	{
		for (long x = 0; x < GetSpellNumEffects(pSpell); x++)
		{
			if (GetSpellAttrib(pSpell, x) == 59)
			{
				sprintf_s(spellCat, "DS%d", x + 1);
				buffType = (BUFFTYPE)(::BUFFTYPE::DS1+(BUFFTYPE)x);
				return;
			}
		}
	}
	if (HasSpellAttrib(pSpell, 85) && (strstr(pSpell->Name, "Talisman of the ") || strstr(pSpell->Name, "Spirit of the ")) && pSpell->SpellIcon == 2)
	{
		strcpy_s(spellCat, "Panther");
		buffType = ::BUFFTYPE::MeleeProc;
		return;
	}
	if (HasSpellAttrib(pSpell, 85) && pSpell->TargetType == ::TARGETTYPE::Self)
	{
		strcpy_s(spellCat, "MeleeProc");
		buffType = ::BUFFTYPE::MeleeProc;
		return;
	}
	if (HasSpellAttrib(pSpell, 360) && pSpell->TargetType==::TARGETTYPE::Self)
	{
		strcpy_s(spellCat, "KillShotProc");
		buffType = ::BUFFTYPE::KillShotProc;
		return;
	}
	if (HasSpellAttrib(pSpell, 323))
	{
		strcpy_s(spellCat, "DefensiveProc");
		buffType = ::BUFFTYPE::DefensiveProc;
		return;
	}
	if (strstr(pSpell->Name, "Ferocity") && pSpell->SpellIcon == 81)
	{
		strcpy_s(spellCat, "Fero");
		buffType = ::BUFFTYPE::Fero;
		return;
	}
	if (strstr(pSpell->Name, "Retort") && pSpell->SpellIcon == 2)
	{
		strcpy_s(spellCat, "Retort");
		buffType = ::BUFFTYPE::Retort;
		return;
	}
	if (pSpell->Category == 42 && pSpell->Subcategory == 32)
	{
		strcpy_s(spellCat, "HoT");
		return;
	}
}

void SpellType(PSPELL pSpell)
{
	if (!InGameOK())
		return;
	if (!pSpell)
		return;
	strcpy_s(spellType, "Unknown");
	spellTypeOption = ZERO;
	buffType = ::BUFFTYPE::NOTYPE;
	SpellCategory(pSpell); //renji: moved to top to ensure it fires before returns
	if ((GetSpellAttribX(pSpell, 0) == 192 && GetSpellBase(pSpell, 0) > 0) || (GetSpellAttribX(pSpell, 4) == 192 && GetSpellBase(pSpell, 4) > 0)) {
		strcpy_s(spellType, "Aggro"); spellTypeOption = ::OPTIONS::AGGRO; spellFunc = CheckAggro;  return;
	}
	if (pSpell->Category == 132 || pSpell->Subcategory == 132 || pSpell->Category == 71) {
		strcpy_s(spellType, "Aura"); spellTypeOption = ::OPTIONS::AURA; spellFunc = CheckAura;  return;
	}
	if (pSpell->Category == 13 || pSpell->Subcategory == 13) {
		strcpy_s(spellType, "Charm"); spellTypeOption = ::OPTIONS::CHARM;  spellFunc = CheckCharm;  return;
	}
	if (strstr(pSpell->Name, " Counterbias") || (pSpell->Subcategory == 21 && pSpell->TargetType == 0 || pSpell->Subcategory == 30 || pSpell->Subcategory == 88 || pSpell->Subcategory == 81) && pSpell->Category != 132) {
		strcpy_s(spellType, "Debuff"); spellTypeOption = ::OPTIONS::DEBUFF; spellFunc = CheckDebuff;  return;
	}
	if (strstr(pSpell->Name, "Dichotomic Paroxysm") || pSpell->Category == 126 && pSpell->Subcategory == 60 || pSpell->Category == 20 || pSpell->Category == 114 && pSpell->Subcategory == 33 || pSpell->Category == 114 && pSpell->Subcategory == 43 && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 0) {
		strcpy_s(spellType, "Dot"); spellTypeOption = ::OPTIONS::DOT; spellFunc = CheckDot;  return;
	}
	if (strstr(pSpell->Name, "Growl of the") || strstr(pSpell->Name, "Grim Aura") || strstr(pSpell->Name, "Roar of the Lion") || strstr(pSpell->Name, "Chaotic Benefaction") ||
		strstr(pSpell->Name, "Dichotomic Companion") || strstr(pSpell->Name, "Dichotomic Fury") || HasSpellAttrib(pSpell, 323) && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) < 61 ||
		pSpell->Category == 45 && pSpell->Subcategory == 141 && pSpell->TargetType == 6 || pSpell->Subcategory == 142 && pSpell->TargetType == 6 && pSpell->Category != 132 ||
		pSpell->Category == 125 && pSpell->Subcategory == 16 && (pSpell->TargetType == 6 || pSpell->TargetType == 41) && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) < 61) {
		strcpy_s(spellType, "FightBuff"); spellTypeOption = ::OPTIONS::FIGHTBUFF; spellFunc = CheckFightBuff;  return;
	}
	if (pSpell->Category == 42 && (pSpell->Subcategory != 19 || strstr(pSpell->Name, "Balm") || strstr(pSpell->Name, "Splash of")) && pSpell->Subcategory != 82 && (pSpell->TargetType == 45 || pSpell->TargetType == 3 || pSpell->TargetType == 5 || pSpell->TargetType == 6 || pSpell->TargetType == 8 || pSpell->TargetType == 41)) {
		strcpy_s(spellType, "Heal"); spellTypeOption = ::OPTIONS::HEAL; spellFunc = CheckHeal;  return;
	}
	//cure has to be after heal else fail
	if (pSpell->Category == 42 && pSpell->Subcategory == 19) {
		strcpy_s(spellType, "Cure"); spellTypeOption = ::OPTIONS::CURE; spellFunc = CheckCure;  return;
	}
	if (pSpell->Subcategory == 42 && pSpell->Category == 69 || pSpell->Category == 42 && (pSpell->TargetType == 3 || pSpell->TargetType == 5 || pSpell->TargetType == 8)) {
		strcpy_s(spellType, "HealPet"); spellTypeOption = ::OPTIONS::HEALPET; spellFunc = CheckHealPet;  return;
	}
	if (pSpell->TargetType == 46 && FindSpellInfoForAttrib(pSpell, BASE, 0) > 0) {
		strcpy_s(spellType, "HealToT"); spellTypeOption = ::OPTIONS::HEALTOT; spellFunc = CheckHealToT; return;
	}
	if (pSpell->Category == 5 && (GetSpellBaseX(pSpell, 0) < 0) || strstr(pSpell->Name, "freeze")) {
		PSPELL pSpell2 = GetSpellByID(GetSpellBase2X(pSpell, 1));
		if (pSpell2 && ((GetSpellAttribX(pSpell2, 2) == 192) || strstr(pSpell->Name, "Concussive ")) && GetCharInfo()->pSpawn->mActorClient.Class == Wizard) {
			strcpy_s(spellType, "Jolt"); spellTypeOption = ::OPTIONS::JOLT; spellFunc = CheckJolt; return;
		}
	}
	if (strstr(pSpell->Name, "Dichotomic Fang") || pSpell->Category == 114 && (pSpell->Subcategory == 43 || pSpell->Subcategory == 33)) {
		strcpy_s(spellType, "Lifetap"); spellTypeOption = ::OPTIONS::LIFETAP; spellFunc = CheckLifetap;  return;
	}
	if (!strstr(pSpell->Name, " the Hunt") && pSpell->Subcategory == 21 && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) < 51 || pSpell->Subcategory == 16 && pSpell->Category != 132 && GetCharInfo()->pSpawn->mActorClient.Class != RNG || pSpell->Subcategory == 62 || pSpell->Subcategory == 64 && strstr(pSpell->Name, " Retort") || pSpell->Category == 45 && pSpell->Subcategory == 141 || pSpell->Subcategory == 64 && strstr(pSpell->Name, "Divine I")) {
		strcpy_s(spellType, "MainTankBuff"); spellTypeOption = ::OPTIONS::MAINTANKBUFF;  spellFunc = CheckMainTankBuff;  return;
	}
	if (pSpell->Subcategory == 61 || pSpell->Subcategory == 17 && FindSpellInfoForAttrib(pSpell, BASE, 15) > 0 || strstr(pSpell->Name, " Symbiosis")) {
		strcpy_s(spellType, "Mana"); spellTypeOption = ::OPTIONS::MANA; spellFunc = CheckMana;  return;
	}
	if (pSpell->Subcategory == 35) {
		strcpy_s(spellType, "Mez"); spellTypeOption = ::OPTIONS::MEZ; spellFunc = CheckMez;  buffType = ::BUFFTYPE::Mez; return;
	}
	if (strstr(pSpell->Name, "Dichotomic Fusillade") || strstr(pSpell->Name, "Dichotomic Force") || strstr(pSpell->Name, "Dichotomic Fire") || pSpell->Category == 25 && pSpell->Subcategory != 35) {
		strcpy_s(spellType, "Nuke");		spellTypeOption = NUKE; spellFunc = CheckNuke; return;
	}
	if (pSpell && pSpell->Autocast == 19780) {
		if (pSpell->TargetType == 46 && FindSpellInfoForAttrib(pSpell, BASE, 0) > 0) {
			strcpy_s(spellType, "NukeToT"); spellTypeOption = ::OPTIONS::NUKETOT; spellFunc = CheckNukeToT; return;
		}
		else if (pSpell->TargetType == 5) {
			for (int slot = 0; slot < GetSpellNumEffects(pSpell); ++slot) {
				if (GetSpellAttrib(pSpell, slot) == 374) {
					PSPELL pSpell2 = GetSpellByID(GetSpellBase2(pSpell, slot));
					if (pSpell2 && GetSpellAttribX(pSpell2, 0) == 0 && GetSpellBase(pSpell2, 0) > 0 && pSpell2->TargetType == 46) {
						strcpy_s(spellType, "NukeToT"); spellTypeOption = ::OPTIONS::NUKETOT; spellFunc = CheckNukeToT; return;
					}
				}
			}
		}
	}
	if (pSpell->Category == 69 && pSpell->Subcategory > 97 && pSpell->Subcategory < 106 && pSpell->Subcategory != 101 && GetCharInfo()->pSpawn->mActorClient.Class != 12 && GetCharInfo()->pSpawn->mActorClient.Class != 2 && GetCharInfo()->pSpawn->mActorClient.Class != 6) {
		strcpy_s(spellType, "Pet");		spellTypeOption = ::OPTIONS::PET; spellFunc = CheckPet;  return;
	}
	if (pSpell->Subcategory == 82) {
		strcpy_s(spellType, "Rez");		spellTypeOption = ::OPTIONS::REZ; spellFunc = CheckRez;  return;
	}
	if (pSpell->Subcategory == 83) {
		strcpy_s(spellType, "Root");		spellTypeOption = ::OPTIONS::ROOT; spellFunc = CheckRoot;  buffType = ::BUFFTYPE::Root; return;
	}
	// skipping SelfBuff for now
	if (pSpell->Subcategory == 89) {
		strcpy_s(spellType, "Snare");		spellTypeOption = ::OPTIONS::SNARE; spellFunc = CheckSnare;  buffType = ::BUFFTYPE::Snare; return;
	}
	if (GetSpellAttribX(pSpell, 0) == 32) {
		spellTypeOption = AGGRO;
		strcpy_s(spellType, "SummonItem");	spellTypeOption = ::OPTIONS::SUMMONITEM; spellFunc = CheckSummonItem;  return;
	}
	if (pSpell->Subcategory == 104 && pSpell->Category == 69 && strstr(pSpell->Extra, "Rk") || pSpell->Subcategory == 99 && (GetCharInfo()->pSpawn->mActorClient.Class == 12 || GetCharInfo()->pSpawn->mActorClient.Class == 2) || pSpell->Subcategory == 139) {
		strcpy_s(spellType, "Swarm");		spellTypeOption = ::OPTIONS::SWARM; spellFunc = CheckSwarm;  return;
	}
	//renji: call to SpellCategory moved to top
	if (spellCat != "Unknown") {
		if (pSpell->TargetType == 6 || pSpell->TargetType == 41) {
			strcpy_s(spellType, "SelfBuff"); spellTypeOption = ::OPTIONS::SELFBUFF; spellFunc = CheckSelfBuff; return;
		}
		if (pSpell->TargetType != 6 && pSpell->TargetType != 41) {
			strcpy_s(spellType, "Buff"); spellTypeOption = ::OPTIONS::BUFF;  spellFunc = CheckBuff;  return;
		}
	}
}

bool ValidBen(PSPELL pSpell, PSPAWNINFO pSpawn)
{
	if (!InGameOK())
		return false;
	if (!pSpawn)
		return false;
	if (!pSpell)
		return false;
	bool valid = false;
	if (pSpell->ZoneType == 1 && !outdoor)
		return false;
	if (pSpell->SpellType != 0)
		if (((int)pSpell->ManaCost < GetCharInfo()->pSpawn->GetCurrentMana()  &&  GetCharInfo()->pSpawn->GetMaxMana()>0 || pSpell->ManaCost < 1) && (DistanceToSpawn3D((PSPAWNINFO)pCharSpawn, pSpawn) < pSpell->Range || DistanceToSpawn3D((PSPAWNINFO)pCharSpawn, pSpawn) < pSpell->AERange))
			valid = true;
	return valid;
}

bool ValidDet(PSPELL pSpell, PSPAWNINFO Target)
{
	if (!InGameOK())
		return false;
	if (!Target || Target->Type == SPAWN_CORPSE)
		return false;
	if (!pSpell)
		return false;
	if (pSpell->ZoneType == 1 && !outdoor)
		return false;
	if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(Target->MasterID))
		if (pSpawn->Type == SPAWN_PLAYER && !strstr(EQADDR_SERVERNAME, "Zek"))
			return false;
	char bodyType[MAX_STRING] = "", testBody[MAX_STRING] = "Single";
	bool validDet = true;
	sprintf_s(bodyType, "${If[(${Spell[%s].TargetType.Find[%s]}||!${Select[${Spell[%s].TargetType},Animal,Humanoid,Undead,Plant,Summoned,Uber Giants,Uber Dragons,AE Undead,AE Summoned]} && !${Spell[%s].Subcategory.Find[summoned]}||${Spell[%s].TargetType.Find[Undead]} && ${Target.Buff[smoldering bones].ID}||${Spell[%s].Subcategory.Find[summoned]} && ${Select[%d,5,24,27,28]}),1,0]}", pSpell->Name, BodyTypeFix, pSpell->Name, pSpell->Name, pSpell->Name, pSpell->Name, GetBodyType(Target));
	ParseMacroData(bodyType, MAX_STRING);
	if (atoi(bodyType) == 0 || !outdoor && pSpell->ZoneType == 1)
	{
		validDet = false;
		return validDet;
	}
	if (pSpell->TargetType == 5 && !(CastRay(GetCharInfo()->pSpawn, Target->Y, Target->X, Target->Z)) || (int)pSpell->ManaCost > GetCharInfo()->pSpawn->GetCurrentMana() && GetCharInfo()->pSpawn->GetMaxMana() > 0 || DistanceToSpawn3D(GetCharInfo()->pSpawn, Target) > pSpell->Range && pSpell->Range > 0 || DistanceToSpawn3D(GetCharInfo()->pSpawn, Target) > pSpell->AERange && pSpell->Range == 0)
		validDet = false;
	return validDet;
}
#pragma endregion SpellFunctionDefinitions

#pragma region CreateRoutines
void CreateAA()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szAA[] = { "Frenzied Devastation", "Prolonged Destruction", "Improved Sustained Destruction", "Arcane Destruction", "Fury of Kerfyrm", "Fury of Ro", "Sustained Devastation",
		"Calculated Insanity", "Chromatic Haze", "Improved Reactive Rune", "Reactive Rune", "Illusions of Grandeur", "Improved Twincast",
		"Sustained Destruction", "Frenzied Burnout", "Virulent Talon", "Fire Core", "Elemental Union", "Visage of Death", "Gift of the Quick Spear",
		"Heart of Skyfire", "Heart of Froststone", "Fury of Eci", "Fury of Druzzil",
		"Valorous Rage", "Rabid Bear", "Nature's Blessing", "Group Spirit of the Great Wolf", "Spirit of the Wood",
		"Divine Avatar", "Celestial Rapidity", "Silent Casting", "Mercurial Torment", "Funeral Pyre", "Gift of Deathly Resolve",
		"Group Guardian of the Forest", "Outrider's Accuracy", "Outrider's Attack", "Auspice of the Hunter", "Guardian of the Forest",
		"Imperator's Command", "Wars Sheol's Heroic Blade", "Healing Frenzy", "Flurry of Life", "Roar of Thunder",
		"Frenzied Swipes", "Chameleon Strike", "Frenzy of Spirit", "Group Bestial Alignment", "Bestial Bloodrage", "Zan Fi's Whistle",
		"Infusion of Thunder", "Hand of Death", "Embalmer's Carapace", "Bestial Alignment", "Ragged Bite of Agony", "Group Armor of the Inquisitor",
		"Inquisitor's Judgement", "Armor of the Inquisitor", "Imbued Ferocity", "Soul Flay", "Quick Time", "Fierce Eye",
		"Bladed Song", "Arcane Fury", "Inquisitor's Judgment", "Dance of Blades", "Channeling the Divine", "Life Burn",
		"Feral Swipe", "Vehement Rage", "Distraction Attack", "Juggernaut Surge", "Reckless Abandon", "Savage Spirit",
		"Untamed Rage", "Braxi's Howl", "Five Point Palm", "Battle Stomp", "Blinding Fury", "Desperation", "Stunning Kick",
		"Two-Finger Wasp Touch", "Bloodlust", "Cascading Rage", "6492", "Hand of Tunare", "Thunder of Karana", "Destructive Vortex",
		"Nature's Fury", "Elemental Arrow", "Visage of Decay", "T'Vyl's Resolve", "Flurry of Notes", "Frenzied Kicks", "Rake's Rampage",
		"War Stomp", "Lyrical Prankster", "Enduring Frenzy", "Fleeting Spirit", "Silent Strikes", "Focused Destructive Force", "Ton Po's Stance",
		"Swift Tails' Chant", "Knee Strike", "Blood Magic", "Companion's Fury", "Heretic's Twincast", "Ferociousness", "Distant Conflagration",
		"Ligament Slice", "Twisted Shank", "T`Vyl's Resolve", "Spiritual Channeling", "Ward of Destruction", NULL };
	char szTemp[MAX_STRING] = { 0 }; 
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szAA[i]; i++) {
		strcpy_s(szTemp, szAA[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::AA;
				spell.CheckFunc = CheckAA;
				strcpy_s(spell.SpellType, "AA");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateAggro()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szAggro[] = { "Improved Explosion of Spite", "Explosion of Spite", "Improved Explosion of Hatred", "Explosion of Hatred",
		"Beacon of the Righteous", "Blast of Anger", "Projection of Fury", "Projection of Piety", "Ageless Enmity",
		"Projection of Doom", "Warlord's Grasp", "Enhanced Area Taunt", "Area Taunt", "Dissonant Chord", "Roaring Strike",
		"Mindless Hatred", "Stream of Hatred", "Veil of Darkness", "Hallowed Lodestar", "Warlord's Fury", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szAggro[i]; i++) {
		strcpy_s(szTemp, szAggro[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::AGGRO;
				spell.CheckFunc = CheckAggro;
				strcpy_s(spell.SpellType, "Aggro");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateCall()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szCall[] = { "Call of the Wild", "Call of the Herald", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szCall[i]; i++) {
		strcpy_s(szTemp, szCall[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::CALL;
				spell.CheckFunc = CheckCall;
				strcpy_s(spell.SpellType, "Call");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateCure()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szCure[] = { "Radiant Cure","Purified Spirits", "Ward of Purity", "Purification", "Blessing of Purification","Purify Body", "Purify Soul",
		"Group Purify Soul","Embrace the Decay","Nature's Salve","Agony of Absolution","Purge Poison", "Purity of Death", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szCure[i]; i++) {
		strcpy_s(szTemp, szCure[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::CURE;
				spell.CheckFunc = CheckCure;
				strcpy_s(spell.SpellType, "Cure");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateDebuff()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szDebuff[] = { "Malaise", "Turgur's Swarm", "Helix of the Undying", "Bite of Tashani", "Dreary Deeds",
		"Blessing of Ro", "Vortex of Ro", "Scent of Terris", "Death's Malaise", "Crippling Sparkle", "Ethereal Manipulation",
		 "Lower Element", "Sha's Reprisal", "Turgur's Virulent Swarm","Wind of Malaise", "Season's Wrath", "Eradicate Magic",
		"Slowing Helix", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szDebuff[i]; i++) {
		strcpy_s(szTemp, szDebuff[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::DEBUFF;
				spell.CheckFunc = CheckDebuff;
				strcpy_s(spell.SpellType, "Debuff");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateDisc()
{
	if (!InGameOK())
		return;
	vTemp.clear();
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	// discs are special due to timer ID and how this detects, I really dont want to screw with how they are ordered, even in a custom ini scenario.
	vector<PSPELL> tempSpell;
	vector<int> tempLevel;
	vector<int> tempTimer;
	vector<int> noOverwrite;
	for (int i = 0; i < 21; i++)
	{
		tempSpell.push_back(NULL);
		tempLevel.push_back(0);
		tempTimer.push_back(i);
		noOverwrite.push_back(0);
	}
	char szTemp[MAX_STRING] = { 0 }, szSpell[MAX_STRING] = { 0 }, szSpellGem[MAX_STRING] = { 0 }, szTempGem[MAX_STRING] = { 0 };
	int customSpells = GetPrivateProfileInt(INISection, "SpellTotal", 0, INIFileName);
	for (int i = 0; i < customSpells; i++)
	{
		sprintf_s(szSpell, "Spell%dName", i);
		if (GetPrivateProfileString(INISection, szSpell, NULL, szTemp, MAX_STRING, INIFileName))
		{
			sprintf_s(szSpellGem, "Spell%dGem", i);
			GetPrivateProfileString(INISection, szSpellGem, NULL, szTempGem, MAX_STRING, INIFileName);
			_strlwr_s(szTempGem);
			if (strstr(szTempGem, "disc"))
			{
				for (unsigned long nCombatAbility = 0; nCombatAbility < NUM_COMBAT_ABILITIES; nCombatAbility++)
				{
					if (pCombatSkillsSelectWnd->ShouldDisplayThisSkill(nCombatAbility)) {
						if (PSPELL pSpell = GetSpellByID(pPCData->GetCombatAbility(nCombatAbility)))
						{
							if (strstr(pSpell->Name, szTemp))
							{
								tempSpell[pSpell->ReuseTimerIndex] = pSpell;
								tempLevel[pSpell->ReuseTimerIndex] = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
								noOverwrite[pSpell->ReuseTimerIndex] = 1;
							}
						}
					}
				}
			}
		}
	}
	for (unsigned long nCombatAbility = 0; nCombatAbility < NUM_COMBAT_ABILITIES; nCombatAbility++)
	{
		if (pCombatSkillsSelectWnd->ShouldDisplayThisSkill(nCombatAbility)) {
			if (PSPELL pSpell = GetSpellByID(pPCData->GetCombatAbility(nCombatAbility)))
			{
				DiscCategory(pSpell);
				if (!strstr(spellCat, "Summon Weapon") && !strstr(pSpell->Name, "Thief's ") && !strstr(pSpell->Name, "Tireless Sprint") && !strstr(pSpell->Name, "Conditioned Reflexes") && (pSpell->ReuseTimerIndex != 8 || GetCharInfo()->pSpawn->mActorClient.Class != 16 || strstr(pSpell->Name, "Cry")))
				{
					if ((long)pSpell->ReuseTimerIndex != -1 && pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > tempLevel[pSpell->ReuseTimerIndex] && !noOverwrite[pSpell->ReuseTimerIndex])
					{
						tempSpell[pSpell->ReuseTimerIndex] = pSpell;
						tempLevel[pSpell->ReuseTimerIndex] = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
						// adding in low level discs that are the best available
						if (strstr(pSpell->Name, "Twisted Chance") || strstr(pSpell->Name, "Frenzied Stabbing") || strstr(pSpell->Name, "Amplified Frenzy"))
							noOverwrite[pSpell->ReuseTimerIndex] = 1;
					}
					else if ((long)pSpell->ReuseTimerIndex == -1)
					{
						tempSpell.push_back(pSpell);
						tempLevel.push_back(pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class]);
					}
				}
			}
		}
	}
	// ------------------finalize discs------------------------------
	// first check for if statement for section

	strcpy_s(szSpell, "DiscIf");
	if (GetPrivateProfileString(INISection, szSpell, NULL, szTemp, MAX_STRING, INIFileName))
	{
		SpellIf.insert(make_pair<string, string>(szSpell, szTemp));
		SpellIt = SpellIf.find(szSpell);
		if (SpellIt != SpellIf.end())
			WriteChatf("DiscIf: %s", SpellIt->second.c_str());
	}
	// now create and check ini entries on discs
	int vSize = tempSpell.size();
	int lSize = tempLevel.size();
	for (int i = 0; i < vSize && i < lSize; i++)
	{
		if (tempLevel[i] > 0)
		{
			BotSpell disc;
			disc.Spell = tempSpell[i];
			strcpy_s(disc.Name, tempSpell[i]->Name);
			
			disc.Type = TYPE_DISC;
			disc.CheckFunc = CheckDisc;
			disc.SpellTypeOption = ::OPTIONS::DISC;
			disc.ID = tempSpell[i]->ID;
			disc.CanIReprioritize = 1;
			strcpy_s(disc.Gem, "disc");
			strcpy_s(disc.SpellType, "Disc");
			BuildSpell(disc);
			DiscCategory(disc.Spell);
			strcpy_s(disc.SpellType, spellCat); // TODO: i dont know why i have to do this again but i do for it to work correctly.. so.. screw it.
			vMaster.push_back(disc);
			vTemp.push_back(disc);
		}
	}
}
void CreateDot()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szDot[] = { "Funeral Dirge", "Bite of the Asp", "Cacophony", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szDot[i]; i++) {
		strcpy_s(szTemp, szDot[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::DOT;
				spell.CheckFunc = CheckDot;
				strcpy_s(spell.SpellType, "Dot");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateGrab()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szGrab[] = { "Hate's Attraction", "Divine Call", "Lure of the Siren's Song", "Grappling Strike", "Moving Mountains", "Call of Challenge", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szGrab[i]; i++) {
		strcpy_s(szTemp, szGrab[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::GRAB;
				spell.CheckFunc = CheckGrab;
				strcpy_s(spell.SpellType, "Grab");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateGroup()
{
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	vGroup.clear();
	_Spawns group;
	group.ID = 0;
	for (int i = 0; i < 6; i++)
	{
		vGroup.push_back(group);
	}
}
void CreateHeal()
{
	if (!InGameOK())
		return;
	if (BardClass || GetCharInfo()->pSpawn->mActorClient.Class == 11 || GetCharInfo()->pSpawn->mActorClient.Class == 12 || GetCharInfo()->pSpawn->mActorClient.Class == 13 || GetCharInfo()->pSpawn->mActorClient.Class == 14)
		return;
	vTemp.clear();
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szHeal[] = { "Divine Arbitration", "Burst of Life", "Beacon of Life", "Focused Celestial Regeneration", "Celestial Regeneration",
		"Convergence of Spirits", "Union of Spirits", "Focused Paragon of Spirits", "Paragon of Spirit", "Lay on Hands",
		"Hand of Piety", "Ancestral Aid", "Call of the Ancients", "Exquisite Benediction", "Blessing of Tunare", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szHeal[i]; i++)
	{
		strcpy_s(szTemp, szHeal[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.CheckFunc = CheckHeal;
				spell.SpellTypeOption = ::OPTIONS::HEAL;
				spell.ID = aa->ID;
				strcpy_s(spell.SpellType, "Heal");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateImHit()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szImHit[] = { "Improved Sanctuary", "Sanctuary", "Outrider's Evasion", "Shield of Notes", "Hymn of the Last Stand", "Doppelganger",
	"Mind Crash", "Brace for Impact", "Companion of Necessity", "Hold the Line", "Divine Retribution",
	"Blessing of Sanctuary", "Embalmer's Carapace", "Improved Death Peace", "Lyrical Prankster", "Ancestral Guard", "Death Peace",
	"Armor of Ancestral Spirits", "Uncanny Resilience", "Protection of the Spirit Wolf", "Tumble", "Warlord's Bravery",
	"Mark of the Mage Hunter", "Blade Guardian", "Repel", "Reprove", "Renounce", "Defy", "Withstand", "Rune of Banishment",
	"Color Shock", "Mind Over Matter", "Warlord's Resurgence", "Warlord's Tenacity", "Warder's Gift", nullptr };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szImHit[i]; i++) {
		strcpy_s(szTemp, szImHit[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::IMHIT;
				spell.CheckFunc = CheckImHit;
				strcpy_s(spell.SpellType, "ImHit");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateJolt()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szJolt[] = { "Mind Crash", "Concussion", "Improved Death Peace", "Arcane Whisper", "Playing Possum",
	"Roar of Thunder", "Sleight of Hand", "Death Peace", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szJolt[i]; i++) {
		strcpy_s(szTemp, szJolt[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::JOLT;
				spell.CheckFunc = CheckJolt;
				strcpy_s(spell.SpellType, "Jolt");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateKnockback()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szKnockback[] = { "Spiritual Rebuke", "Repel the Wicked", "Silent Displacement", "Sonic Displacement", "Paralytic Spores",
		"Virulent Paralysis", "Wall of Wind", "Press the Attack", "Unbridled Strike of Fear",
		"Beam of Displacement", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szKnockback[i]; i++) {
		strcpy_s(szTemp, szKnockback[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::KNOCKBACK;
				spell.CheckFunc = CheckKnockback;
				strcpy_s(spell.SpellType, "Knockback");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateMainTankBuff()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szMainTankBuff[] = { "Divine Guardian", "Spirit Guardian", "Focused Celestial Regeneration", "Swirl of Fireflies", "Blade Guardian", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szMainTankBuff[i]; i++) {
		strcpy_s(szTemp, szMainTankBuff[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::MAINTANKBUFF;
				spell.CheckFunc = CheckMainTankBuff;
				strcpy_s(spell.SpellType, "MainTankBuff");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateMez()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szMez[] = { "Dirge of the Sleepwalker", "Nightmare Stasis", "Beam of Slumber", "Wave of Slumber", "Dead Mesmerization",
	"Scintillating Beam", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szMez[i]; i++) {
		strcpy_s(szTemp, szMez[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::MEZ;
				spell.CheckFunc = CheckMez;
				strcpy_s(spell.SpellType, "Mez");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateNuke()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szNuke[] = { "Force of Will", "Force of Flame", "Force of Ice", "Force of Elements", "Vicious Bite of Chaos", "Divine Stun",
		"Harm Touch", "Nature's Bolt", "Elemental Arrow", "Smite the Wicked", "Turn Undead",
		"Turn Summoned", "Disruptive Persecution", "Nature's Fire", "Nature's Frost", "Force of Disruption", "Mana Burn", "Vanquish the Fallen", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szNuke[i]; i++) {
		strcpy_s(szTemp, szNuke[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::NUKE;
				spell.CheckFunc = CheckNuke;
				strcpy_s(spell.SpellType, "Nuke");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateRoot()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szRoot[] = { "Grasp of Sylvan Spirits", "Virulent Paralysis", "Shackles of Tunare", "Beguiler's Directed Banishment",
	"Strong Root", "Paralytic Spores", "Pestilent Paralysis", "Blessed Chains", "Frost Shackles", "Paralytic Spray",
	"Binding Axe", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szRoot[i]; i++) {
		strcpy_s(szTemp, szRoot[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::ROOT;
				spell.CheckFunc = CheckRoot;
				strcpy_s(spell.SpellType, "Root");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}

void BuildBuff(_BotSpell &spell)
{

	// I need to check vMaster, or do i?
	//WriteChatf("Checking: %s", spell.Name);
	// I need to check vBuff
	int vSize = vBuff.size();
	int found = 0;
	for (int i = 0; i < vSize; i++)
	{
		if (vBuff[i].Type == spell.BuffType && spell.BuffType>0)
		{
			// already have a spell of this type. let's figure out what it is
			if (spell.Spell->TargetType == ::TARGETTYPE::Self)
				vBuff[i].Self = spell;
			if (spell.Spell->TargetType == ::TARGETTYPE::Single|| spell.Spell->TargetType == ::TARGETTYPE::SingleInGroup)
				vBuff[i].Single = spell;
			if(spell.Spell->TargetType == ::TARGETTYPE::Pet|| spell.Spell->TargetType == ::TARGETTYPE::Pet2)
				vBuff[i].Pet = spell;
			if (spell.Spell->TargetType == ::TARGETTYPE::Group_v1|| spell.Spell->TargetType == ::TARGETTYPE::Group_v2|| spell.Spell->TargetType == ::TARGETTYPE::AE_PCv1|| spell.Spell->TargetType == ::TARGETTYPE::AE_PCv2)
				vBuff[i].Group = spell;
			//WriteChatf("Found, Adding: %s", spell.Name);
			found++;
		}
	}
	if (!found)
	{
		//didnt find it, womp womp. let's add it
		Buff buff;
		buff.Type = spell.BuffType;
		if (spell.Spell->TargetType == ::TARGETTYPE::Self)
			buff.Self = spell;
		if (spell.Spell->TargetType == ::TARGETTYPE::Single || spell.Spell->TargetType == ::TARGETTYPE::SingleInGroup)
			buff.Single = spell;
		if (spell.Spell->TargetType == ::TARGETTYPE::Pet || spell.Spell->TargetType == ::TARGETTYPE::Pet2)
			buff.Pet = spell;
		if (spell.Spell->TargetType == ::TARGETTYPE::Group_v1 || spell.Spell->TargetType == ::TARGETTYPE::Group_v2 || spell.Spell->TargetType == ::TARGETTYPE::AE_PCv1 || spell.Spell->TargetType == ::TARGETTYPE::AE_PCv2)
			buff.Group = spell;
		// need to add it to our master buffs list
		switch (spell.BuffType)
		{
		case ::BUFFTYPE::Aego:			MyBuffs.Aego = buff;			break;
		case ::BUFFTYPE::Charge:		MyBuffs.Charge = buff;			break;
		case ::BUFFTYPE::Skin:			MyBuffs.Skin = buff;			break;
		case ::BUFFTYPE::Focus:			MyBuffs.Focus = buff;			break;
		case ::BUFFTYPE::Regen:			MyBuffs.Regen = buff;			break;
		case ::BUFFTYPE::Symbol:		MyBuffs.Symbol = buff;			break;
		case ::BUFFTYPE::Clarity:		MyBuffs.Clarity = buff;			break;
		case ::BUFFTYPE::Pred:			MyBuffs.Pred = buff;			break;
		case ::BUFFTYPE::Strength:		MyBuffs.Strength = buff;		break;
		case ::BUFFTYPE::Brells:		MyBuffs.Brells = buff;			break;
		case ::BUFFTYPE::SV:			MyBuffs.SV = buff;				break;
		case ::BUFFTYPE::SE:			MyBuffs.SE = buff;				break;
		case ::BUFFTYPE::Growth:		MyBuffs.Growth = buff;			break;
		case ::BUFFTYPE::Rune:			MyBuffs.Rune = buff;			break;
		case ::BUFFTYPE::Rune2:			MyBuffs.Rune2 = buff;			break;
		case ::BUFFTYPE::Shining:		MyBuffs.Shining = buff;			break;
		case ::BUFFTYPE::SpellHaste:	MyBuffs.SpellHaste = buff;		break;
		case ::BUFFTYPE::MeleeProc:		MyBuffs.MeleeProc = buff;		break;
		case ::BUFFTYPE::SpellProc:		MyBuffs.SpellProc = buff;		break;
		case ::BUFFTYPE::Invis:			MyBuffs.Invis = buff;			break;
		case ::BUFFTYPE::IVU:			MyBuffs.IVU = buff;				break;
		case ::BUFFTYPE::Levitate:		MyBuffs.Levitate = buff;		break;
		case ::BUFFTYPE::MoveSpeed:		MyBuffs.MoveSpeed = buff;		break;
		case ::BUFFTYPE::Haste:			MyBuffs.Haste = buff;			break;
		case ::BUFFTYPE::DS1:			MyBuffs.DS1 = buff;				break;
		case ::BUFFTYPE::DS2:			MyBuffs.DS2 = buff;				break;
		case ::BUFFTYPE::DS3:			MyBuffs.DS3 = buff;				break;
		case ::BUFFTYPE::DS4:			MyBuffs.DS4 = buff;				break;
		case ::BUFFTYPE::DS5:			MyBuffs.DS5 = buff;				break;
		case ::BUFFTYPE::DS6:			MyBuffs.DS6 = buff;				break;
		case ::BUFFTYPE::DS7:			MyBuffs.DS7 = buff;				break;
		case ::BUFFTYPE::DS8:			MyBuffs.DS8 = buff;				break;
		case ::BUFFTYPE::DS9:			MyBuffs.DS9 = buff;				break;
		case ::BUFFTYPE::DS10:			MyBuffs.DS10 = buff;			break;
		case ::BUFFTYPE::DS11:			MyBuffs.DS11 = buff;			break;
		case ::BUFFTYPE::DS12:			MyBuffs.DS12 = buff;			break;
		case ::BUFFTYPE::Fero:			MyBuffs.Fero = buff;			break;
		case ::BUFFTYPE::Retort:		MyBuffs.Retort = buff;			break;
		case ::BUFFTYPE::KillShotProc:	MyBuffs.KillShotProc = buff;	break;
		case ::BUFFTYPE::DefensiveProc:	MyBuffs.DefensiveProc = buff;	break;
		}
		//WriteChatf("Adding: %s", spell.Name);
		vBuff.push_back(buff);
	}
}

void CreateBuff()
{
	if (!InGameOK())
		return;
	PCHAR szSelfBuff[] = { "Preincarnation", "Selo's Sonata", "Sionachie's Crescendo", "Eldritch Rune", /*"Voice of Thule",*/
		 "Elemental Form", "Cascade of Decay", "Dark Lord's Unity (Azia)", "Pact of the Wurine", "Talisman of Celerity", "Lupine Spirit",
		"Etherealist's Unity","Divine Protector's Unity","Enticer's Unity",
		"Feralist's Unity","Mortifier's Unity","Orator's Unity","Saint's Unity","Thaumaturge's Unity","Transfixer's Unity",
		"Visionary's Unity","Wildstalker's Unity (Azia)","Wildstalker's Unity (Beza)",  "Wildtender's Unity",	NULL
		//Broken need to figure out why it crashes with pSpell->Name access   	"Dark Lord's Unity (Beza)",
	}; //readd "Veil of Mindshadow" slot 11 //Broken need to figure out why it crashes with pSpell->Name access
	vector<PSPELL> vSelfBuff;
	PCHAR szFamiliar[] = { "E'ci's Icy Familiar", "Druzzil's Mystical Familiar", "Ro's Flaming Familiar", "Improved Familiar",
		"Kerafyrm's Prismatic Familiar", NULL };
	//PCHAR szMancy[] = { "Cryomancy", "Arcomancy", "Pyromancy", NULL };//Patch notes 11/20/19 - Wizard - Arcomancy, Cryomancy, and Pyromancy have been consolidated into a toggled passive ability named Trifurcating Magic. Not sure if it used to be toggled or activated.
	for (unsigned int i = 0; szSelfBuff[i]; i++)
	{
		for (unsigned long nAbility = 0; nAbility < AA_CHAR_MAX_REAL; nAbility++)
		{
			if (PALTABILITY pAbility = pAltAdvManager->GetAAById(pPCData->GetAlternateAbilityId(nAbility)))
			{
				if (PCHAR pName = pCDBStr->GetString(pAbility->nName, 1, NULL))
				{
					if (!_stricmp(szSelfBuff[i], pName))
					{
						if (!StrStrIA(szSelfBuff[i], "'s Unity"))
						{
							if (PSPELL pSpell = GetSpellByID(pAbility->SpellID))
							{
								if (GetSpellAttrib(pSpell, 0) == 470 || GetSpellAttrib(pSpell, 0) == 374) // casts another spell or more
								{
									if (PSPELL temp = CheckTrigger(pSpell, 0))
										WriteChatf("ALERT: Update AA Buffs - Need to add: AA=%s, Spell=%s", szSelfBuff[i], temp->Name);
									// i dont think any of the AAs actually do this atm.
								}
								else
								{
									Buff buff;
									BotSpell botspell;
									botspell.AA = pAbility;
									strcpy_s(botspell.Name, pName);
									botspell.GroupID = pSpell->SpellGroup;
									BuildSpell(botspell);
									BuildBuff(botspell);
								}
							}
						}
						if (StrStrIA(szSelfBuff[i], "'s Unity"))
						{
							if (PSPELL pUnitySpell = GetSpellByID(pAbility->SpellID))
							{
								BotSpell unityspell;
								unityspell.Spell = pUnitySpell;
								strcpy_s(unityspell.Name, pUnitySpell->Name);
								unityspell.AA = pAbility;
								UnitySpell = unityspell.Spell;
								long numeffects = GetSpellNumEffects(pUnitySpell);
								for (int x = 0; x < numeffects; x++)
								{
									if (GetSpellAttrib(pUnitySpell, x) == 470) //pTrigger = (PSPELL)pSpellMgr->GetSpellByGroupAndRank(groupid, pSpell->SpellSubGroup, pSpell->SpellRank, true)
									{
										if (PSPELL pUnitySubSpell = CheckTrigger(pUnitySpell, x))
										{
											Buff buff;
											BotSpell botspell;
											buff.Unity = pAbility;
											char szTemp[MAX_STRING] = { 0 };
											botspell.AA = pAbility;
											if (!bGoldStatus && StrStrIA(pUnitySubSpell->Name, " Rk. II"))
											{
												if (StrStrIA(pUnitySubSpell->Name, "Rk."))
												{
													// all these correct for wrong rank spells if you are not gold status
													sprintf_s(szTemp, "%s", pUnitySubSpell->Name);
													std::string str1 = szTemp;
													str1.replace(str1.find("Rk."), 4, "");
													if (str1.substr(str1.length() - 4) == " III")
														str1.replace(str1.find(" III"), 4, "");
													if (str1.substr(str1.length() - 3) == " II")
														str1.replace(str1.find(" II"), 3, "");
													sprintf_s(szTemp, "%s", str1.c_str());
												}
											}
											else
												strcpy_s(szTemp, pUnitySubSpell->Name);
											strcpy_s(botspell.Name, szTemp);
											botspell.GroupID = pUnitySpell->SpellGroup;
											botspell.UnitySpell = pUnitySpell;
											BuildSpell(botspell);
											BuildBuff(botspell);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (!BardClass)
	{
		for (unsigned int i = 0; szFamiliar[i]; i++)
		{
			for (unsigned long nAbility = 0; nAbility < AA_CHAR_MAX_REAL; nAbility++)
			{
				if (PALTABILITY pAbility = pAltAdvManager->GetAAById(pPCData->GetAlternateAbilityId(nAbility)))
				{
					if (PCHAR pName = pCDBStr->GetString(pAbility->nName, 1, NULL))
					{
						if (!_stricmp(szFamiliar[i], pName))
						{
							if (GetSpellByID(pAbility->SpellID))
							{
								BotSpell botspell;
								botspell.AA = pAbility;
								strcpy_s(botspell.Name, szFamiliar[i]);
								BuildSpell(botspell);
								BuildBuff(botspell);
								WriteChatf("AA=%s, Spell=%s", szFamiliar[i], GetSpellByID(pAbility->SpellID)->Name);
							}
						}
					}
				}
			}
		}
	//	/*
	//	I've commented out this section. Previously the 3 different mancy skills were activated buffs, now they are on an AA that is either Active, or Inactive.
	//	for (unsigned int i = 0; szMancy[i]; i++)
	//	{
	//		for (unsigned long nAbility = 0; nAbility<AA_CHAR_MAX_REAL; nAbility++)
	//		{
	//			if (PALTABILITY pAbility = pAltAdvManager->GetAAById(pPCData->GetAlternateAbilityId(nAbility)))
	//			{
	//				if (PCHAR pName = pCDBStr->GetString(pAbility->nName, 1, NULL))
	//				{
	//					if (!_stricmp(szMancy[i], pName))
	//					{
	//						if (GetSpellByID(pAbility->SpellID))
	//						{
	//							vSelfBuff.push_back(GetSpellByID(pAbility->SpellID));
	//							vSelfBuffName.push_back(szMancy[i]);
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}*/
		/*enum CLASSES { NONE, WAR, CLR, PAL, RNG, SHD, DRU, MNK, BRD, ROG, SHM, NEC, WIZ, MAG, ENC, BST, BER };
		 enum TARGETTYPE { UNKNOWN, LoS, AE_PCv1, Group_v1, PB_AE, Single, Self, Ignore, Targeted_AE, Animal, Undead, Summoned, Lifetap = 13, 
		Pet = 14, Corpse, Plant, UberGiants, UberDragons, Targeted_AE_Tap = 20, AE_Undead = 24, AE_Summoned = 25, Hatelist = 32, Hatelist2 = 33, Chest = 34,
			SpecialMuramites = 35, Caster_PB_PC = 36, Caster_PB_NPC = 37, Pet2 = 38, NoPets = 39, AE_PCv2 = 40, Group_v2 = 41, Directional_AE = 42, SingleInGroup = 43,
			Beam = 44, FreeTarget = 45, TargetOfTarget = 46, PetOwner = 47, Target_AE_No_Players_Pets = 50
		};*/
		/*
		//long duration buffs
buffSelfSpellResist,buffSingleSpellResist,buffGroupSpellResist, // might need to break this out by class or resist type since there's so many.
buffSingleResistCorruption,buffGroupResistCorruption, //maybe?
buffSelfHybridHP, // pal/sk self hp buffs
buffSelfConvert, // SK/nec mana conversion
buffSelfAttack, // SK attack buff
buffSingleAego, buffGroupAego, buffSingleUnifiedAego, buffGroupUnifiedAego, buffGroupSpellHaste, buffSingleAC, buffGroupAC,//clr
buffSingleSymbol, buffGroupSymbol, buffSingleUnifiedSymbol, buffGroupUnifiedSymbol, buffSingleSpellHaste, //clr .. 
buffSingleHaste, buffGrouphaste, buffAAHaste, // shm, enc, bst (single)
buffSingleSkin, buffGroupSkin, //dru /rng (single only rng)
buffSelfFocus, //clr/casters
buffSingleFocus, buffGroupFocus, // shm / bst (group only except for one single spell at level 67)
buffSingleRegen, buffGroupRegen, // shm/dru rang(single)
buffSelfClarity, buffSingleClarity, buffGroupClarity, // rng, dru self . enc single/group
buffSelfPred, buffGroupPred, buffGroupStrength, // rng
buffGroupBrells, //pal only
buffGroupSV, buffGroupSE, //bst only
HybridHP, //not sure if i need this atm
buffSelfDS, buffSingleDS, buffGroupDS, buffPetDS,
buffSingleFerocity, buffGroupFerocity, // dont use single if you have group
// there are a ton of different lines for procs on hit or cast. many classes get 2, sometimes 3
buffSelfMeleeProc, buffSingleMeleeProc, buffGroupMeleeProc,
buffSelfMeleeProc2,
buffSelfSpellProc, buffSingleSpellProc, buffGroupSpellProc,
buffSelfKillShotProc,
// misc buffs common across classes
buffSelfInvis, buffSingleInvis, buffGroupInvis,
buffSelfIVU, buffSingleIVU, buffGroupIVU,
buffSelfLevitate, buffSingleLevitate, buffGroupLevitate,
buffSingleMoveSpeed, buffGroupMoveSpeed,
//things i might only use if memmed, usually short duration and require getting hit to matter
buffSelfDefensiveProc, buffSingleDefensiveProc,
buffSelfRune, buffSingleRune, buffGroupRune,
buffSelfRune2, buffSingleRune2, buffGroupRune2,
buffDivineIntervention, //clr
//things i should only use if memmed.. high reuse timer or extremely short duration or both
buffSingleRetort, buffSelfDivineAura, buffSingleDivineAura, buffSelfDivineAura2, buffYaulp, buffSingleShining, //clr.. ok clerics.. we get it. you are special
buffSelfWard, buffSingleWard,
buffSingleGrowth;// dru/shm
		*/
		int skin = 0, aego = 0, symbol = 0, type2 = 0, shielding = 0, clarity = 0, se = 0, haste = 0, regen = 0, pred = 0, ds = 0,
			rune = 0, rune2 = 0, mana2 = 0, defensive = 0, shroud = 0, horror = 0, fero = 0, call = 0, convert = 0, killshot = 0,
			hastesingle = 0, claritysingle = 0, regensingle = 0, aegosingle=0, aegounifiedsingle=0,aegogroupunified=0,symbolsingle=0,symbolunifiedsingle=0,
			meleeprocself=0,meleeprocsingle=0,meleeprocgroup=0,meleeprocself2=0,meleeprocself3,defensivesingle=0, defensiveself=0,spellhastesingle=0,spellhastegroup=0,
			absorbsingle=0,absorbself=0,absorbgroup=0,
			symbolunifiedgroup=0, clarityself = 0, ACsingle=0, ACgroup=0, dsself = 0, dssingle = 0, divineintervention = 0, skinsingle=0, focussingle=0, focusgroup=0;
		PSPELL	pskin, paego, psymbol, ptype2, pshielding, pclarity, pse, phaste, pregen, ppred, pds, prune, prune2, pmana2, pdefensive,
			pshroud, phorror, pfero, pcall, pconvert, pkillshot, phastesingle, pclaritysingle, pregensingle, paegosingle, paegounifiedsingle, paegogroupunified, psymbolsingle, psymbolunifiedsingle,
			pmeleeprocself,pmeleeprocsingle,pmeleeprocgroup,pmeleeprocself2,pmeleeprocself3,pdefensivesingle,pdefensiveself,pspellhastesingle,pspellhastegroup,
			pabsorbself,pabsorbsingle,pabsorbgroup,
			psymbolunifiedgroup, pclarityself, pACsingle,pACgroup, pdsself, pdssingle, pdivineintervention, pskinsingle, pfocussingle, pfocusgroup;
		vector<_BotSpell> vMeleeProcs;

		for (unsigned int nSpell = 0; nSpell < NUM_BOOK_SLOTS; nSpell++)
		{
			if (PSPELL pSpell = GetSpellByID(GetCharInfo2()->SpellBook[nSpell]))
			{
				long attrib0 = GetSpellAttrib(pSpell, 0);
				
				if (attrib0 != 33)
				{
					if (attrib0 == 85 && pSpell->TargetType==Self) // melee proc
					{
						BotSpell botspell;
						botspell.Spell = pSpell;
						botspell.Priority = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
						vMeleeProcs.push_back(botspell);
					}
					if (GetSpellNumEffects(pSpell)>1) // melee proc on slot 1
						if (GetSpellAttrib(pSpell, 1)==85)
						{
							BotSpell botspell;
							botspell.Spell = pSpell;
							botspell.Priority = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							vMeleeProcs.push_back(botspell);
						}
					if (attrib0 == 85 && pSpell->TargetType == Single && GetCharInfo()->pSpawn->mActorClient.Class!=BST) // melee proc single.. and idk that i want this for bsts
					{
						meleeprocsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
						pmeleeprocsingle = pSpell;
					}
					if (attrib0 == 127 && GetCharInfo()->pSpawn->mActorClient.Class ==CLR && pSpell->TargetType==Single) // spell haste single
					{
						spellhastesingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
						pspellhastesingle = pSpell;
					}
					if (attrib0 == 127 && GetCharInfo()->pSpawn->mActorClient.Class == CLR && pSpell->TargetType == Group_v2) // spell haste group
					{
						spellhastegroup = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
						pspellhastegroup = pSpell;
					}
					//if (attrib0 == 85 && pSpell->TargetType == Single) // melee proc group // i dont think i want panther here.
					//{
					//	meleeprocsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
					//	pmeleeprocsingle = pSpell;
					//}
					if (pSpell->Category == 95 && pSpell->Subcategory == 6 && pSpell->TargetType == Single) // single clr ward
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > ACsingle)
						{
							ACsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pACsingle = pSpell;
						}
					}
					if (pSpell->Category == 95 && pSpell->Subcategory == 6 && pSpell->TargetType == Group_v2) // group clr ward
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > ACgroup)
						{
							ACgroup = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pACgroup = pSpell;
						}
					}
					if ((StrStrIA(pSpell->Name, "Shield of ") || (StrStrIA(pSpell->Name, "Rallied") && StrStrIA(pSpell->Name, "of Vie"))) && pSpell->Category == 125 && pSpell->Subcategory == 62 && pSpell->TargetType==Self) // vie and wiz shields
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > absorbself)
						{
							absorbself = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pabsorbself = pSpell;
						}
					}
					if ((StrStrIA(pSpell->Name, "Shield of ") || (StrStrIA(pSpell->Name, "Rallied") && StrStrIA(pSpell->Name, "of Vie"))) && pSpell->Category == 125 && pSpell->Subcategory == 62 && pSpell->TargetType == Single) // vie and wiz shields
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > absorbsingle)
						{
							absorbsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pabsorbsingle = pSpell;
						}
					}
					if ((StrStrIA(pSpell->Name, "Shield of ") || (StrStrIA(pSpell->Name, "Rallied") && StrStrIA(pSpell->Name, "of Vie"))) && pSpell->Category == 125 && pSpell->Subcategory == 62 && pSpell->TargetType==Group_v2)
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > absorbgroup)
						{
							absorbgroup = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pabsorbgroup = pSpell;
						}
					}
					if (StrStrIA(pSpell->Name, "Ferocity") && !StrStrIA(pSpell->Name, "Sha's") && GetCharInfo()->pSpawn->mActorClient.Class == BST)
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > fero)
						{
							fero = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pfero = pSpell;
						}
					}
					if (pSpell->Subcategory == 59 && pSpell->Category == 79 && pSpell->TargetType == 6 || pSpell->Subcategory == 17 && GetSpellAttribX(pSpell, 1) == 15 && GetSpellBaseX(pSpell, 1) > 0 && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 0
						|| pSpell->Subcategory == 59 && GetSpellAttribX(pSpell, 2) == 15 && GetSpellBaseX(pSpell, 2) > 2)
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > mana2)
						{
							mana2 = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pmana2 = pSpell;
						}
					}
					if (GetCharInfo()->pSpawn->mActorClient.Class == SHD && !StrStrIA(pSpell->Name, "Shroud of Hate") && !StrStrIA(pSpell->Name, "Shroud of Pain") && (StrStrIA(pSpell->Name, "Shroud of") || StrStrIA(pSpell->Name, "Scream of Death") || StrStrIA(pSpell->Name, "Black Shroud") || StrStrIA(pSpell->Name, "Vampiric Embrace")) && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 60)
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > shroud)
						{
							shroud = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pshroud = pSpell;
						}
					}
					if (GetCharInfo()->pSpawn->mActorClient.Class == SHD && pSpell->Category == 125 && pSpell->Subcategory == 17 && GetSpellAttribX(pSpell, 1) == 2) // SK ATK buff
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > call)
						{
							call = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pcall = pSpell;
						}
					}
					if ((GetCharInfo()->pSpawn->mActorClient.Class == SHD || GetCharInfo()->pSpawn->mActorClient.Class == NEC) && pSpell->Category == 125 && pSpell->Subcategory == 17 && GetSpellAttribX(pSpell, 1) == 15) // SK / NEC mana convert
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > convert)
						{
							convert = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pconvert = pSpell;
						}
					}
					if (GetCharInfo()->pSpawn->mActorClient.Class == SHD && StrStrIA(pSpell->Name, " Horror") && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 60) // SK horror line
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > horror)
						{
							horror = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							phorror = pSpell;
						}
					}
					if (HasSpellAttrib(pSpell, 323) && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 60 && pSpell->TargetType==Self) // self defensive proc
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > defensiveself)
						{
							defensiveself = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pdefensiveself = pSpell;
						}
					}
					if (HasSpellAttrib(pSpell, 323) && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 60 && pSpell->TargetType == Single) // single defensive proc
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > defensivesingle)
						{
							defensivesingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pdefensivesingle = pSpell;
						}
					}
					if (HasSpellAttrib(pSpell, 323) && GetSpellDuration(pSpell, (PSPAWNINFO)pLocalPlayer) > 60 && pSpell->TargetType == Group_v2) // group defensive proc .. TODO: do these exist?
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > defensive)
						{
							defensive = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pdefensive = pSpell;
						}
					}
					if (pSpell->Subcategory == 84 && pSpell->Category == 125 && pSpell->ReagentID[0] == -1 && pSpell->TargetType == 6)
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > rune)
						{
							rune = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							prune = pSpell;
						}
					}
					if (pSpell->Subcategory == 1 && pSpell->TargetType == Single && StrStrIA(pSpell->Name, "Unified")) // unified single aego
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > aegounifiedsingle)
						{
							aegounifiedsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							paegounifiedsingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 1 && pSpell->TargetType == Single && !StrStrIA(pSpell->Name, "Unified")) // single aego
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > aegosingle)
						{
							aegosingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							paegosingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 1 && pSpell->TargetType == Group_v2 && StrStrIA(pSpell->Name, "Unified")) // unified group aego
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > aegogroupunified)
						{
							aegogroupunified = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							paegogroupunified = pSpell;
						}
					}
					if (pSpell->Subcategory == 1 && pSpell->TargetType == Group_v2 && !StrStrIA(pSpell->Name, "Unified") || StrStrIA(pSpell->Name, "Heroic Bond")) // group aego
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > aego)
						{
							aego = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							paego = pSpell;
						}
					}
					if (pSpell->Subcategory == 46 && pSpell->TargetType == Single && (GetCharInfo()->pSpawn->mActorClient.Class != 2 || GetCharInfo()->pSpawn->Level < 60) && (GetCharInfo()->pSpawn->mActorClient.Class != 3 || GetCharInfo()->pSpawn->Level < 70))
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > skinsingle)
						{
							skinsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pskinsingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 46 && pSpell->TargetType == Group_v2 && (GetCharInfo()->pSpawn->mActorClient.Class != 2 || GetCharInfo()->pSpawn->Level < 60) && (GetCharInfo()->pSpawn->mActorClient.Class != 3 || GetCharInfo()->pSpawn->Level < 70))
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > skin)
						{
							skin = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pskin = pSpell;
						}
					}
					if (pSpell->Subcategory == 87 && pSpell->TargetType==Self) // self focus/shielding
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > shielding)
						{
							shielding = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pshielding = pSpell;
						}
					}
					if (pSpell->Subcategory == 87 && pSpell->TargetType==Single) // single focus
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > focussingle)
						{
							focussingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pfocussingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 87 && pSpell->TargetType==Group_v2) // group focus
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > focusgroup)
						{
							focusgroup = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pfocusgroup = pSpell;
						}
					}
					if (pSpell->Subcategory == 59 && pSpell->Category == 79 && pSpell->TargetType == Self) // self clarity	
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > clarityself)
						{
							clarityself = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pclarityself = pSpell;
						}
					}
					if (pSpell->Subcategory == 59 && pSpell->Category == 79 && pSpell->TargetType == Single) // single clarity
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > claritysingle)
						{
							claritysingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pclaritysingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 59 && pSpell->Category == 79 && pSpell->TargetType == Group_v2) // group clarity
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > clarity)
						{
							clarity = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pclarity = pSpell;
						}
					}
					if (pSpell->Subcategory == 44 && pSpell->Category == 79 && pSpell->TargetType == Group_v2) // bst SE
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > se)
						{
							se = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pse = pSpell;
						}
					}
					if (pSpell->Subcategory == 43 && pSpell->Category == 79 && pSpell->TargetType == Single) // single regen
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > regensingle)
						{
							regensingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pregensingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 43 && pSpell->Category == 79 && pSpell->TargetType == Group_v2) // group regen
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > regen)
						{
							regen = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pregen = pSpell;
						}
					}
					if (pSpell->Subcategory == 7 && pSpell->Category == 95 && pSpell->TargetType == 41 && GetCharInfo()->pSpawn->mActorClient.Class == 4) // rng predator
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > pred)
						{
							pred = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							ppred = pSpell;
						}
					}
					if (pSpell->Subcategory == 112 && pSpell->TargetType == Group_v2 && StrStrIA(pSpell->Name,"Unified")) //unified group symbol
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > symbolunifiedgroup)
						{
							symbolunifiedgroup = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							psymbolunifiedgroup = pSpell;
						}
					}
					if (pSpell->Subcategory == 112 && pSpell->TargetType == Group_v2 && !StrStrIA(pSpell->Name, "Unified"))
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > symbol) // group symbol
						{
							symbol = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							psymbol = pSpell;
						}
					}
					if (pSpell->Subcategory == 112 && pSpell->TargetType == Single && StrStrIA(pSpell->Name, "Unified")) //unified single symbol
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > symbolunifiedsingle)
						{
							symbolunifiedsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							psymbolunifiedsingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 112 && pSpell->TargetType == Single && !StrStrIA(pSpell->Name, "Unified"))
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > symbolsingle) // single symbol
						{
							symbolsingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							psymbolsingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 47 && pSpell->TargetType == 41) // BST/PAL/RNG hp buff lines
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > type2)
						{
							type2 = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							ptype2 = pSpell;
						}
					}
					if (pSpell->Subcategory == 41 && pSpell->Category == 125 && pSpell->TargetType == Single) // single haste
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > hastesingle)
						{
							hastesingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							phastesingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 41 && pSpell->Category == 125 && pSpell->TargetType == Group_v2) // group haste
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > haste)
						{
							haste = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							phaste = pSpell;
						}
					}
					if (pSpell->Subcategory == 21 && pSpell->Category == 125 && pSpell->TargetType == Self) // self ds
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > dsself)
						{
							dsself = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pdsself = pSpell;
						}
					}
					if (pSpell->Subcategory == 21 && pSpell->Category == 125 && pSpell->TargetType == Single && GetCharInfo()->pSpawn->mActorClient.Class!=WIZ && pSpell->SpellType!=0) // single ds
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > dssingle)
						{
							dssingle = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pdssingle = pSpell;
						}
					}
					if (pSpell->Subcategory == 21 && pSpell->Category == 125 && pSpell->TargetType == Group_v2) // group ds
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > ds)
						{
							ds = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pds = pSpell;
						}
					}
					if ((pSpell->Subcategory == 16 || pSpell->Subcategory == 64) && pSpell->Category == 125 && (StrStrIA(pSpell->Name, "Pack of ") || StrStrIA(pSpell->Name, "by the Hunt") || StrStrIA(pSpell->Name, "Face of ") || StrStrIA(pSpell->Name, "Remorse") || StrStrIA(pSpell->Name, "Natural ")))
					{
						if (pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class] > killshot)
						{
							killshot = pSpell->ClassLevel[GetCharInfo()->pSpawn->mActorClient.Class];
							pkillshot = pSpell;
						}
					}
				}
			}
		}

		if (vMeleeProcs.size()) // little complicated due to multiple lines
		{
			SortSpellVector(vMeleeProcs);
			strcpy_s(vMeleeProcs[0].Name, vMeleeProcs[0].Spell->Name);
			BuildSpell(vMeleeProcs[0]);
			BuildBuff(vMeleeProcs[0]);
			WriteChatf("MeleeProc \ay[Self]\aw: %s", vMeleeProcs[0].Name);
			if (vMeleeProcs.size() > 1)
			{
				if (vMeleeProcs[1].Priority > (vMeleeProcs[0].Priority - 5)) // if second proc is still within last 5 spells of first, let's use it because it is a different line
				{
					strcpy_s(vMeleeProcs[1].Name, vMeleeProcs[1].Spell->Name);
					BuildSpell(vMeleeProcs[1]);
					BuildBuff(vMeleeProcs[1]);
					WriteChatf("MeleeProc2 \ay[Self]\aw: %s", vMeleeProcs[1].Name);
				}
			}
			if (vMeleeProcs.size() > 2)
			{
				if (vMeleeProcs[2].Priority > (vMeleeProcs[0].Priority - 5)) // if third proc is still within last 5 spells of first, let's use it because it is a different line
				{
					strcpy_s(vMeleeProcs[2].Name, vMeleeProcs[2].Spell->Name);
					BuildSpell(vMeleeProcs[2]);
					BuildBuff(vMeleeProcs[2]);
					WriteChatf("MeleeProc3 \ay[Self]\aw: %s", vMeleeProcs[2].Name);
				}
			}

		}
		if (shroud)
		{
			BotSpell spell;
			spell.Spell = pshroud;
			strcpy_s(spell.Name, pshroud->Name);
			BuildSpell(spell);
			BuildBuff(spell);
		}
		if (horror)
		{
			BotSpell spell;
			spell.Spell = phorror;
			strcpy_s(spell.Name, phorror->Name);
			BuildSpell(spell);
			BuildBuff(spell);
		}
		if (call)
		{
			BotSpell spell;
			spell.Spell = pcall;
			strcpy_s(spell.Name, pcall->Name);
			BuildSpell(spell);
			BuildBuff(spell);
		}
		if (convert)
		{
			BotSpell spell;
			spell.Spell = pconvert;
			strcpy_s(spell.Name, pconvert->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Convert \ay[Self]\aw: %s", pconvert->Name);
		}
		if (fero)
		{
			BotSpell spell;
			spell.Spell = pfero;
			strcpy_s(spell.Name, pfero->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			if(pfero->TargetType==Single)
				WriteChatf("Fero \ay[Single]\aw: %s", pfero->Name);
			else
				WriteChatf("Fero \ap[Group]\aw: %s", pfero->Name);
		}
		if (aego)
		{
			BotSpell spell;
			spell.Spell = paego;
			strcpy_s(spell.Name, paego->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Aego \ap[Group]\aw: %s", paego->Name);
		}
		if (aegosingle)
		{
			BotSpell spell;
			spell.Spell = paegosingle;
			strcpy_s(spell.Name, paegosingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Aego \ao[Single]\aw: %s", paegosingle->Name);
		}
		if (aegogroupunified)
		{
			BotSpell spell;
			spell.Spell = paegogroupunified;
			strcpy_s(spell.Name, paegogroupunified->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Aego \ap[Group]\at[Unified]\aw: %s", paegogroupunified->Name);
		}
		if (aegounifiedsingle)
		{
			BotSpell spell;
			spell.Spell = paegounifiedsingle;
			strcpy_s(spell.Name, paegounifiedsingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Aego \ao[Single]\at[Unified]\aw: %s", paegounifiedsingle->Name);
		}
		if (spellhastesingle)
		{
			BotSpell spell;
			spell.Spell = pspellhastesingle;
			strcpy_s(spell.Name, pspellhastesingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("SpellHaste \ao[Single]\aw: %s", pspellhastesingle->Name);
		}
		if (spellhastegroup)
		{
			BotSpell spell;
			spell.Spell = pspellhastegroup;
			strcpy_s(spell.Name, pspellhastegroup->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("SpellHaste \ap[Group]\aw: %s", pspellhastegroup->Name);
		}
		if (ACsingle)
		{
			BotSpell spell;
			spell.Spell = pACsingle;
			strcpy_s(spell.Name, pACsingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Shielding \ao[Single]\aw: %s", pACsingle->Name);
		}
		if (ACgroup)
		{
			BotSpell spell;
			spell.Spell = pACgroup;
			strcpy_s(spell.Name, pACgroup->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Shielding \ap[Group]\aw: %s", pACgroup->Name);
		}
		if (absorbself)
		{
			BotSpell spell;
			spell.Spell = pabsorbself;
			strcpy_s(spell.Name, pabsorbself->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Absorb \ay[Self]\aw: %s", pabsorbself->Name);
		}
		if (absorbsingle)
		{
			BotSpell spell;
			spell.Spell = pabsorbsingle;
			strcpy_s(spell.Name, pabsorbsingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Mitigate \ao[Single]\aw: %s", pabsorbsingle->Name);
		}
		if (absorbgroup)
		{
			BotSpell spell;
			spell.Spell = pabsorbgroup;
			strcpy_s(spell.Name, pabsorbgroup->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Mitigate \ap[Group]\aw: %s", pabsorbgroup->Name);
		}
		if (skinsingle)
		{
			BotSpell spell;
			spell.Spell = pskinsingle;
			strcpy_s(spell.Name, pskinsingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Skin \ao[Single]\aw: %s", pskinsingle->Name);
		}
		if (skin)
		{
			BotSpell spell;
			spell.Spell = pskin;
			strcpy_s(spell.Name, pskin->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Skin \ap[Group]\aw: %s", pskin->Name);
		}
		if (symbol)
		{
			BotSpell spell;
			spell.Spell = psymbol;
			strcpy_s(spell.Name, psymbol->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Symbol \ap[Group]\aw: %s", psymbol->Name);
		}
		if (clarityself)
		{
			BotSpell spell;
			spell.Spell = pclarityself;
			strcpy_s(spell.Name, pclarityself->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Clarity \ay[Self]\aw: %s", pclarityself->Name);
		}
		if (claritysingle)
		{
			BotSpell spell;
			spell.Spell = pclaritysingle;
			strcpy_s(spell.Name, pclaritysingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Clarity \ao[Single]\aw: %s", pclaritysingle->Name);
		}
		if (clarity)
		{
			BotSpell spell;
			spell.Spell = pclarity;
			strcpy_s(spell.Name, pclarity->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Clarity \ap[Group]\aw: %s", pclarity->Name);
		}
		if (se)
		{
			BotSpell spell;
			spell.Spell = pse;
			strcpy_s(spell.Name, pse->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("SE \ap[Group]\aw: %s", pse->Name);
		}
		if (hastesingle)
		{
			BotSpell spell;
			spell.Spell = phastesingle;
			strcpy_s(spell.Name, phastesingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Haste \ao[Single]\aw: %s", phastesingle->Name);
		}
		if (haste)
		{
			BotSpell spell;
			spell.Spell = phaste;
			strcpy_s(spell.Name, phaste->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Haste \ap[Group]\aw: %s", phaste->Name);
		}
		if (shielding)
		{
			BotSpell spell;
			spell.Spell = pshielding;
			strcpy_s(spell.Name, pshielding->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Shielding \ay[Self]\aw: %s", pshielding->Name);
		}
		if (focussingle)
		{
			BotSpell spell;
			spell.Spell = pfocussingle;
			strcpy_s(spell.Name, pfocussingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Focus \ao[Single]\aw: %s", pfocussingle->Name);
		}
		if (focussingle)
		{
			BotSpell spell;
			spell.Spell = pfocussingle;
			strcpy_s(spell.Name, pfocussingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Focus \ao[Single]\aw: %s", pfocussingle->Name);
		}if (focusgroup)
		{
			BotSpell spell;
			spell.Spell = pfocusgroup;
			strcpy_s(spell.Name, pfocusgroup->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Focus \ap[Group]\aw: %s", pfocusgroup->Name);
		}
		if (type2)
		{
			BotSpell spell;
			spell.Spell = ptype2;
			strcpy_s(spell.Name, ptype2->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("HybridHP \ap[Group]\aw: %s", ptype2->Name);
		}
		if (regensingle)
		{
			BotSpell spell;
			spell.Spell = pregensingle;
			strcpy_s(spell.Name, pregensingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Regen \ao[Single]\aw: %s", pregensingle->Name);
		}
		if (regen)
		{
			BotSpell spell;
			spell.Spell = pregen;
			strcpy_s(spell.Name, pregen->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Regen \ap[Group]\aw: %s", pregen->Name);
		}
		if (pred)
		{
			BotSpell spell;
			spell.Spell = ppred;
			strcpy_s(spell.Name, ppred->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("Predator \ap[Group]\aw: %s", ppred->Name);
		}
		if (dsself)
		{
			BotSpell spell;
			spell.Spell = pdsself;
			strcpy_s(spell.Name, pdsself->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("DS \ay[Self]\aw: %s", pdsself->Name);
		}
		if (dssingle)
		{
			BotSpell spell;
			spell.Spell = pdssingle;
			strcpy_s(spell.Name, pdssingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("DS \ao[Single]\aw: %s", pdssingle->Name);
		}
		if (ds)
		{
			BotSpell spell;
			spell.Spell = pds;
			strcpy_s(spell.Name, pds->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("DS \ap[Group]\aw: %s", pds->Name);
		}
		if (rune)
		{
			BotSpell spell;
			spell.Spell = prune;
			strcpy_s(spell.Name, prune->Name);
			BuildSpell(spell);
			BuildBuff(spell);
		}
		/*if (rune2)
		{
			BotSpell spell;
			spell.Spell = prune2;
			strcpy_s(spell.Name, prune2->Name);
			BuildSpell(spell);
			BuildBuff(spell);
		}*/
		if (mana2)
		{
			BotSpell spell;
			spell.Spell = pmana2;
			strcpy_s(spell.Name, pmana2->Name);
			BuildSpell(spell);
			BuildBuff(spell);
		}
		if (defensiveself)
		{
			BotSpell spell;
			spell.Spell = pdefensiveself;
			strcpy_s(spell.Name, pdefensiveself->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("DefensiveProc \ay[Self]\aw: %s", pdefensiveself->Name);
		}
		if (defensivesingle)
		{
			BotSpell spell;
			spell.Spell = pdefensivesingle;
			strcpy_s(spell.Name, pdefensivesingle->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("DefensiveProc \ao[Single]\aw: %s", pdefensivesingle->Name);
		}
		if (defensive)
		{
			BotSpell spell;
			spell.Spell = pdefensive;
			strcpy_s(spell.Name, pdefensive->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("DefensiveProc \ap[Group]\aw: %s", pdefensive->Name);
		}
		if (killshot)
		{
			BotSpell spell;
			spell.Spell = pkillshot;
			strcpy_s(spell.Name, pkillshot->Name);
			BuildSpell(spell);
			BuildBuff(spell);
			WriteChatf("KillshotProc \ay[Self]\aw: %s", pkillshot->Name);
		}
	}

	int vSize = vBuff.size();
	//WriteChatf("%d", vSize);
	for (int i = 0; i < vSize; i++)
	{
		/*if (vBuff[i].Unity)
		{
			PCHAR pName = pCDBStr->GetString(vBuff[i].Unity->nName, 1, NULL);
			WriteChatf("UNITY: %s", pName);
		}*/
		if (vBuff[i].Self.ID)
			WriteChatf("SELF: %s (%s) %d", vBuff[i].Self.Name, vBuff[i].Self.SpellCat,i);
		if (vBuff[i].Single.ID)
			WriteChatf("SINGLE: %s (%s) %d", vBuff[i].Single.Name, vBuff[i].Single.SpellCat,i);
		if (vBuff[i].Group.ID)
			WriteChatf("GROUP: %s (%s) %d", vBuff[i].Group.Name, vBuff[i].Group.SpellCat,i);
		if (vBuff[i].Pet.ID)
			WriteChatf("PET: %s (%s) %d", vBuff[i].Pet.Name, vBuff[i].Pet.SpellCat,i);
		// WriteChatf("%s (%s)", vBuff[i].BotSpell.Name, vBuff[i].Category);
	}
}
void CreateSnare()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szSnare[] = { "Encroaching Darkness", "Entrap", "Atol's Unresistable Shackles", "Crippling Strike", "Atol's Shackles", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szSnare[i]; i++) {
		strcpy_s(szTemp, szSnare[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::SNARE;
				spell.CheckFunc = CheckSnare;
				strcpy_s(spell.SpellType, "Snare");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void CreateSwarm()
{
	if (!InGameOK())
		return;
	strcpy_s(CurrentRoutine, &(__FUNCTION__[6]));
	PCHAR szSwarm[] = { "Phantasmal Opponent", "Army of the Dead", "Wake the Dead", "Swarm of Decay", "Rise of Bones", "Song of Stone", "Servant of Ro",
		"Host of the Elements", "Nature's Guardian", "Celestial Hammer", /*"Chattering Bones",*/ "Call of Xuzl",
		"Spirit Call", "Attack of the Warders", "Illusory Ally", "Pack Hunt", "Spirits of Nature", NULL };
	char szTemp[MAX_STRING] = { 0 };
	int aaIndex;
	PALTABILITY aa;
	for (int i = 0; szSwarm[i]; i++) {
		strcpy_s(szTemp, szSwarm[i]);
		aaIndex = GetAAIndexByName(szTemp);
		if (aaIndex > 0)
		{
			aa = pAltAdvManager->GetAAById(aaIndex);
			if (aa && GetSpellByID(aa->SpellID) && (int)aa->ReuseTimer > 0)
			{
				BotSpell spell;
				strcpy_s(spell.Name, szTemp);
				// i dont think we need to manually set things but adding for now and keep removing as long as things work
				spell.Type = TYPE_AA;
				spell.ID = aa->ID;
				spell.SpellTypeOption = ::OPTIONS::SWARM;
				spell.CheckFunc = CheckSwarm;
				strcpy_s(spell.SpellType, "Swarm");
				BuildSpell(spell);
				vMaster.push_back(spell);
			}
		}
	}
}
void ValidateBuffs()
{
	return;
	vector<Buff> test;
	if (MyBuffs.Aego.Type)
		WriteChatf("Aego line detected");
	if (MyBuffs.Pred.Type)
		WriteChatf("Pred line detected");
	if (MyBuffs.Strength.Type)
		WriteChatf("Strength line detected");
	if (MyBuffs.Haste.Type)
		WriteChatf("Haste line detected");
	if (MyBuffs.Symbol.Type)
		WriteChatf("Symbol line detected");
	if (MyBuffs.MeleeProc.Type)
		WriteChatf("Meleeproc line detected");
	if (MyBuffs.Regen.Type)
		WriteChatf("Regen line detected");
	if (MyBuffs.SV.Type)
		WriteChatf("SV line detected");
	if (MyBuffs.Focus.Type)
		WriteChatf("Focus line detected");
			/*case ::BUFFTYPE::Charge:		MyBuffs.Charge = buff;			break;
			case ::BUFFTYPE::Skin:			MyBuffs.Skin = buff;			break;
			case ::BUFFTYPE::Focus:			MyBuffs.Focus = buff;			break;
			case ::BUFFTYPE::Regen:			MyBuffs.Regen = buff;			break;
			case ::BUFFTYPE::Symbol:		MyBuffs.Symbol = buff;			break;
			case ::BUFFTYPE::Clarity:		MyBuffs.Clarity = buff;			break;
			case ::BUFFTYPE::Pred:			MyBuffs.Pred = buff;			break;
			case ::BUFFTYPE::Strength:		MyBuffs.Strength = buff;		break;
			case ::BUFFTYPE::Brells:		MyBuffs.Brells = buff;			break;
			case ::BUFFTYPE::SV:			MyBuffs.SV = buff;				break;
			case ::BUFFTYPE::SE:			MyBuffs.SE = buff;				break;
			case ::BUFFTYPE::Growth:		MyBuffs.Growth = buff;			break;
			case ::BUFFTYPE::Rune:			MyBuffs.Rune = buff;			break;
			case ::BUFFTYPE::Rune2:			MyBuffs.Rune2 = buff;			break;
			case ::BUFFTYPE::Shining:		MyBuffs.Shining = buff;			break;
			case ::BUFFTYPE::SpellHaste:	MyBuffs.SpellHaste = buff;		break;
			case ::BUFFTYPE::MeleeProc:		MyBuffs.MeleeProc = buff;		break;
			case ::BUFFTYPE::SpellProc:		MyBuffs.SpellProc = buff;		break;
			case ::BUFFTYPE::Invis:			MyBuffs.Invis = buff;			break;
			case ::BUFFTYPE::IVU:			MyBuffs.IVU = buff;				break;
			case ::BUFFTYPE::Levitate:		MyBuffs.Levitate = buff;		break;
			case ::BUFFTYPE::MoveSpeed:		MyBuffs.MoveSpeed = buff;		break;
			case ::BUFFTYPE::Haste:			MyBuffs.Haste = buff;			break;
			case ::BUFFTYPE::DS1:			MyBuffs.DS1 = buff;				break;
			case ::BUFFTYPE::DS2:			MyBuffs.DS2 = buff;				break;
			case ::BUFFTYPE::DS3:			MyBuffs.DS3 = buff;				break;
			case ::BUFFTYPE::DS4:			MyBuffs.DS4 = buff;				break;
			case ::BUFFTYPE::DS5:			MyBuffs.DS5 = buff;				break;
			case ::BUFFTYPE::DS6:			MyBuffs.DS6 = buff;				break;
			case ::BUFFTYPE::DS7:			MyBuffs.DS7 = buff;				break;
			case ::BUFFTYPE::DS8:			MyBuffs.DS8 = buff;				break;
			case ::BUFFTYPE::DS9:			MyBuffs.DS9 = buff;				break;
			case ::BUFFTYPE::DS10:			MyBuffs.DS10 = buff;			break;
			case ::BUFFTYPE::DS11:			MyBuffs.DS11 = buff;			break;
			case ::BUFFTYPE::DS12:			MyBuffs.DS12 = buff;			break;
			case ::BUFFTYPE::Fero:			MyBuffs.Fero = buff;			break;
			case ::BUFFTYPE::Retort:		MyBuffs.Retort = buff;			break;
			case ::BUFFTYPE::KillShotProc:	MyBuffs.KillShotProc = buff;	break;
			case ::BUFFTYPE::DefensiveProc:	MyBuffs.DefensiveProc = buff;	break;*/
}
#pragma endregion CreateRoutines

#pragma region CheckRoutines
// master check
void CheckMaster()
{
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	CheckMemmedSpells();
	int vSize = vMaster.size();
	for (int i = 0; i < vSize; i++)
	{
		if (vMaster[i].CheckFunc != NULL)
			vMaster[i].CheckFunc(i);
	}
}
void CheckAA(int spell)
{
	if (!InGameOK())
		return;
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	if (vMaster[spell].Spell->SpellType == 0)
		if (!ShouldICastDetrimental(vMaster[spell]))
			return;
	if (vMaster[spell].Spell->SpellType != 0)
		if (SpellStacks(vMaster[spell].Spell) || GetCharInfo()->pSpawn->mActorClient.Class == 16 && strcmp(vMaster[spell].Name, "Savage Spirit"))
			return;
	// all the basic checks have cleared, now we need to test the one-offs

	// FixNote 20160728.2 - this check will need readded for AACutOffTime
	// if (valid && (atol(sTest) <= (AACutoffTime * 60) || named && GetSpawnByID((DWORD)atol(sNamed))))
	if (GetCharInfo()->pSpawn->mActorClient.Class == 16 && (!strcmp(vMaster[spell].Name, "Savage Spirit") || !strcmp(vMaster[spell].Name, "Untamed Rage")))
	{
		char sstest[MAX_STRING] = "";
		sprintf_s(sstest, "${If[${Me.ActiveDisc.ID},1,0]}");
		ParseMacroData(sstest, MAX_STRING);
		if (atoi(sstest) == 1)
			return;
	}
	BotCastCommand(vMaster[spell]);
}
void CheckAggro(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckAura(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckBard(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckBuff(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckCall(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckCharm(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckClickies(int item)
{
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	//DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckClickyBuffs(int item)
{
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	//DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckClickyNukes(int item)
{
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	//DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckCure(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckDebuff(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckDisc(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckDot(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckEndurance(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckFade(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckFightBuff(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckGrab(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckHeal(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckHealPet(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckHealToT(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckImHit(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckItems(int item)
{
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	//DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckJolt(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckKnockback(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckLifetap(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckMainTankBuff(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckMana(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckMez(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckNuke(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckNukeToT(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckPet(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckRez(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckRoot(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckSelfBuff(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckSnare(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckSummonItem(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
void CheckSwarm(int spell)
{
	if ((unsigned int)spell >= vMaster.size())
		return;
	strcpy_s(CurrentRoutine, MAX_STRING, &(__FUNCTION__[5]));
	DebugWrite("Checking %s: Spell: %s -- Priority: %d", CurrentRoutine, vMaster[spell].Name, vMaster[spell].Priority); // test code
	return;
}
#pragma endregion CheckRoutines

#pragma region Configure
//void Configure(char szCustomIni[MAX_STRING], int force)
void Configure(PCHAR szCustomIni, int force)
{
	if (!InGameOK())
		return;
	//save for later // int i;
	vMaster.clear();
	vMemmedSpells.clear();
	vMemmedSpells.assign(NUM_SPELL_GEMS - 1, _BotSpell());
	DWORD Shrouded;
	char tempINI[MAX_STRING] = { 0 }, tempCustomINI[MAX_STRING] = { 0 };
	long Class = GetCharInfo()->pSpawn->mActorClient.Class;
	MyClass = Class;
	long Races = GetCharInfo2()->Race;
	long Level = GetCharInfo2()->Level;
	sprintf_s(INIFileName, "%s\\%s_%s.ini", gszINIPath, EQADDR_SERVERNAME, GetCharInfo()->Name);
	sprintf_s(INISection, "%s_%d_%s_%s", PLUGIN_NAME, Level, pEverQuest->GetRaceDesc(Races), pEverQuest->GetClassDesc(Class));
	Shrouded = GetCharInfo2()->Shrouded;
	if (!Shrouded)
		INISection[strlen(PLUGIN_NAME)] = 0;
	strcpy_s(tempINI, INISection);
	if (szCustomIni != nullptr) {
		_strlwr_s(szCustomIni, MAX_STRING);
		if (force && strlen(szCustomIni) > 1 && !strstr(szCustomIni, "default"))
			sprintf_s(INISection, "%s_%s", tempINI, szCustomIni);
	}
	char szTemp[MAX_STRING] = { 0 };
	GetPrivateProfileString(INISection, "Debugging", NULL, DEBUG_DUMPFILE, MAX_STRING, INIFileName);//log to file?
	WritePrivateProfileString(INISection, "Debugging", DEBUG_DUMPFILE, INIFileName);
	DefaultGem = GetPrivateProfileInt(INISection, "DefaultGem", 1, INIFileName); //default gem to cast with
	if (EQADDR_SUBSCRIPTIONTYPE && *EQADDR_SUBSCRIPTIONTYPE) 
	{
		DWORD dwsubtype = *(DWORD*)EQADDR_SUBSCRIPTIONTYPE;
		if (dwsubtype) 
		{
			BYTE subtype = *(BYTE*)dwsubtype;
			switch (subtype)
			{
			case 0:
				break;
			case 1:
				break;
			case 2:
				bGoldStatus=true;
				break;
			}
		}
	}
	//Buffs
	if (Class == 2) //cleric
	{
		GetPrivateProfileString(INISection, "AegoClasses", "|WAR|MNK|ROG|BER|CLR|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", AegoClasses, sizeof(AegoClasses), INIFileName);
		GetPrivateProfileString(INISection, "ACClasses", "|WAR|MNK|ROG|BER|CLR|SHM|RNG|BST|PAL|SHD|BRD|", ACClasses, sizeof(ACClasses), INIFileName);
		GetPrivateProfileString(INISection, "SpellHasteClasses", "|WAR|MNK|ROG|BER|CLR|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SpellHasteClasses, sizeof(SpellHasteClasses), INIFileName);
		GetPrivateProfileString(INISection, "SymbolClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SymbolClasses, sizeof(SymbolClasses), INIFileName);
	}
	if (Class == 3) //pal
	{
		GetPrivateProfileString(INISection, "AegoClasses", "|WAR|MNK|ROG|BER|CLR|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", AegoClasses, sizeof(AegoClasses), INIFileName);
		GetPrivateProfileString(INISection, "SymbolClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SymbolClasses, sizeof(SymbolClasses), INIFileName);
		GetPrivateProfileString(INISection, "BrellsClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", BrellsClasses, sizeof(BrellsClasses), INIFileName);
	}
	if (Class == 4) //rng
	{
		GetPrivateProfileString(INISection, "SkinClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SkinClasses, sizeof(SkinClasses), INIFileName);
		GetPrivateProfileString(INISection, "PredClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", PredClasses, sizeof(PredClasses), INIFileName);
		GetPrivateProfileString(INISection, "StrengthClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", StrengthClasses, sizeof(StrengthClasses), INIFileName);
		GetPrivateProfileString(INISection, "DSClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|", DSClasses, sizeof(DSClasses), INIFileName);
	}
	if (Class == 6) //dru
	{
		GetPrivateProfileString(INISection, "SkinClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SkinClasses, sizeof(SkinClasses), INIFileName);
		GetPrivateProfileString(INISection, "DSClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|", DSClasses, sizeof(DSClasses), INIFileName);
		GetPrivateProfileString(INISection, "RegenClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", RegenClasses, sizeof(RegenClasses), INIFileName);
	}
	if (Class == 10) //shm
	{
		GetPrivateProfileString(INISection, "FocusClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", FocusClasses, sizeof(FocusClasses), INIFileName);
		GetPrivateProfileString(INISection, "HasteClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", HasteClasses, sizeof(HasteClasses), INIFileName);
		GetPrivateProfileString(INISection, "RegenClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", RegenClasses, sizeof(RegenClasses), INIFileName);
	}
	if (Class == 13) //mag
	{
		GetPrivateProfileString(INISection, "DSClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|", DSClasses, sizeof(DSClasses), INIFileName);
	}
	if (Class == 14) //enc
	{
		GetPrivateProfileString(INISection, "ClarityClasses", "|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", ClarityClasses, sizeof(ClarityClasses), INIFileName);
		GetPrivateProfileString(INISection, "HasteClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", HasteClasses, sizeof(HasteClasses), INIFileName);
	}
	if (Class == 15) //bst
	{
		GetPrivateProfileString(INISection, "FocusClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", FocusClasses, sizeof(FocusClasses), INIFileName);
		GetPrivateProfileString(INISection, "HasteClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", HasteClasses, sizeof(HasteClasses), INIFileName);
		GetPrivateProfileString(INISection, "SVClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SVClasses, sizeof(SVClasses), INIFileName);
		GetPrivateProfileString(INISection, "SEClasses", "|WAR|MNK|ROG|BER|CLR|DRU|SHM|RNG|BST|PAL|SHD|BRD|ENC|MAG|NEC|WIZ|", SEClasses, sizeof(SEClasses), INIFileName);
		GetPrivateProfileString(INISection, "FeroClasses", "|WAR|MNK|ROG|BER|RNG|BST|PAL|SHD|BRD|", FeroClasses, sizeof(FeroClasses), INIFileName);
	}
}
#pragma endregion Configure

#pragma region create
void Create()
{
	DurationSetup();
	CreateGroup();
	CreateAA();
	CreateAggro();
	CreateBuff();
	CreateCall();
	CreateCure();
	CreateDebuff();
	CreateDisc();
	CreateDot();
	CreateGrab();
	CreateHeal();
	CreateImHit();
	CreateJolt();
	CreateKnockback();
	CreateMainTankBuff();
	CreateMez();
	CreateNuke();
	CreateRoot();
	CreateSnare();
	CreateSwarm();
	ValidateBuffs();
}
#pragma endregion create


#pragma region Commands
void BotCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	if (!InGameOK())
		return;
	vMaster.clear();
	Configure(NULL, 0);
	DebugWrite("BotCommand");
	CustomSpells();				//create all the ini stuff first so it sets precedent
	Create();					//create all the unknown AAs/discs/items
	CheckMemmedSpells();		//create any remaining unknown spells that you have memmed
	if(vMaster.size())
		SortSpellVector(vMaster);
	ConfigureLoaded = true;
	if (strlen(szLine) != 0)
	{
		CHAR Arg1[MAX_STRING] = { 0 }, Arg2[MAX_STRING] = { 0 };
		GetArg(Arg1, szLine, 1);
		if (!_stricmp(Arg1, "populate"))
		{
			GetArg(Arg2, szLine, 2);
			if (strlen(Arg2) != 0)
			{
				if (!_stricmp(Arg2, "spell"))
				{
					PopulateIni(vMaster, "Spell");
				}
			}
		}
	}
}
void ListCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	if (!InGameOK())
		return;
	if (!vMaster.size())
		return;
	int vSize = vMaster.size();
	for (int i = 0; i < vSize; i++)
	{
		if (!_stricmp(vMaster[i].If, "0")||vMaster[i].Use==0)
			WriteChatf("\arSpell%d: [%s] %s, Pri: %d (Disabled)", i, vMaster[i].SpellType, vMaster[i].Name,  vMaster[i].Priority);
		else
		{
			if(vMaster[i].UseInCombat)
				WriteChatf("\aoSpell%d: \aw[%s] %s%s, \aw Pri: %d", i, vMaster[i].SpellType, vMaster[i].Color, vMaster[i].Name, vMaster[i].Priority);
			else
				WriteChatf("\aoSpell%d: \ag[%s] %s%s, \aw Pri: %d", i, vMaster[i].SpellType, vMaster[i].Color, vMaster[i].Name, vMaster[i].Priority);
		}
			
		if (_stricmp(vMaster[i].If,"1") && _stricmp(vMaster[i].If, "0"))
			WriteChatf("\aySpell%dIf: \aw%s", i, vMaster[i].If);
	}
}
void MemmedCommand(PSPAWNINFO pChar, PCHAR szLine)
{
	CheckMemmedSpells();
	//SortSpellVector(vMemmedSpells); //we probably shouldn't sort this since they're in gem-order
}
#pragma endregion Commands

#pragma region Loading
void PluginOn()
{
	AddCommand("/bot", BotCommand);
	AddCommand("/botlist", ListCommand);
	AddCommand("/memmed", MemmedCommand);
}

void PluginOff()
{
	RemoveCommand("/bot");
	RemoveCommand("/botlist");
	RemoveCommand("/memmed");
}

// Called when plugin is loading
PLUGIN_API VOID InitializePlugin(VOID)
{
	PluginOn();
}

// Called when plugin is unloading
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2Bot");
	PluginOff();
}

PLUGIN_API VOID OnPulse(VOID)
{
	if (!InGameOK())
		return;

	if (!ConfigureLoaded)
		return;
	CheckAdds();
	CheckGroup();
	CheckMaster();
}

// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
// It is NOT called for each existing spawn when a plugin shuts down.
PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	if (gGameState == GAMESTATE_INGAME)
	{
		//Compare to spawn lists
		if (pSpawn) {
			SPAWNINFO tSpawn;
			memcpy(&tSpawn, pSpawn, sizeof(SPAWNINFO));
			// check against vSpawn/vAdds
			//RemoveFromNotify(&tSpawn, true);
		}
	}
}
#pragma endregion Loading

#pragma warning ( pop )

#endif MQ2Bot2
