﻿/*
 * Copyright (C) 2014 NorthStrider
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "Player.h"
#include "lost_city_of_the_tolvir.h"

enum Texts
{
    // Augh Intro
    SAY_AGGRO_INTRO         = 0,
    SAY_ESCAPE_FIGHT        = 1,

    // Augh Boss
    SAY_ANNOUNCE_STEALTH    = 0,
    SAY_WONDER              = 1,
    SAY_HAPPY               = 2,

    // Augh Supporters
    SAY_ATTACK              = 0,
    SAY_DESPAWN             = 1,

    // Lockmaw
};

enum Spells
{
    // Intro Augh
    SPELL_SMOKE_BOMB            = 84768,

    // Lockmaw
    SPELL_VISCOUS_POISON        = 81630,
    SPELL_SCENT_OF_BLOOD        = 81690,
    SPELL_SUMMON_CROCOLISK      = 84242,
    SPELL_DUST_FLAIL_SUMMON     = 81652,
    SPELL_DUST_FLAIL            = 81642,
    SPELL_DUST_FLAIL_TRIGGERED  = 81646,

    SPELL_SUMMON_AUGH_1         = 84808,
    SPELL_SUMMON_AUGH_2         = 84809,

    // Augh 1
    SPELL_STEALTHED             = 84244,
    SPELL_PARALYTIC_BLOW_DART   = 84799,

    // Augh 2
    // SPELL_STEALTHED
    SPELL_RANDOM_AGGRO_TAUNT    = 50230,
    SPELL_WHIRLWIND             = 84784,
    SPELL_WHIRLWIND_BATTLE      = 91408,

    // Augh Boss
    SPELL_DRAGONS_BREATH        = 83776,
    SPELL_FRENZY                = 91415,
};

enum Events
{
    // Augh Intro
    EVENT_SMOKE_BOMB = 1,
    EVENT_INVISIBLE,

    // Lockmaw
    EVENT_VISCOUS_POISON,
    EVENT_SCENT_OF_BLOOD,
    EVENT_SUMMON_CROCOLISK,
    EVENT_SUMMON_AUGH,
    EVENT_DUST_FLAIL,
    EVENT_ATTACK,

    // Frenzied Crocolisk
    EVENT_FIND_BLOOD_PLAYER,

    // Augh
    EVENT_TALK,
    EVENT_TALK_2,
    EVENT_MOVE,
    EVENT_MOVE_OUT,
    EVENT_PARALYTIC_BLOW,
    EVENT_WHIRLWIND,
    EVENT_PICK_RANDOM_VICTIM,
    EVENT_FOLLOW_PLAYER,
    EVENT_TALK_OUTRO,
    EVENT_TALK_OUTRO_2,
    EVENT_TALK_OUTRO_3,
    EVENT_DRAGONS_BREATH,
    EVENT_FRENZY,
    EVENT_CANCEL_FOLLOW,
};

Position const AughSpawnPos = {-11079.485f, -1648.531f, 0.879f, 5.490f};
Position const AughWP       = {-11064.710f, -1664.492f, 0.746f, 0.664f};

class boss_lockmaw : public CreatureScript
{
public:
    boss_lockmaw() : CreatureScript("boss_lockmaw") { }

    struct boss_lockmawAI : public BossAI
    {
        boss_lockmawAI(Creature* creature) : BossAI(creature, DATA_LOCKMAW)
        {
            _firstAugh = true;
        }

        bool _firstAugh;

        void Reset()
        {
            _firstAugh = true;
        }

        void EnterCombat(Unit* /*victim*/)
        {
            _EnterCombat();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            events.ScheduleEvent(EVENT_VISCOUS_POISON, 18000);
            events.ScheduleEvent(EVENT_SCENT_OF_BLOOD, 5000);
            events.ScheduleEvent(EVENT_DUST_FLAIL, 29000);
            events.ScheduleEvent(EVENT_SUMMON_AUGH, 7000);
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->DespawnCreaturesInArea(NPC_FRENZIED_CROCOLISK, 500.0f);
            me->DespawnCreaturesInArea(NPC_AUGH_1, 500.0f);
            me->DespawnCreaturesInArea(NPC_AUGH_2, 500.0f);
            if (IsHeroic())
                me->SummonCreature(BOSS_AUGH, AughSpawnPos, TEMPSUMMON_MANUAL_DESPAWN);
        }

        void EnterEvadeMode()
        {
            _EnterEvadeMode();
            events.Reset();
            instance->SetBossState(DATA_LOCKMAW, FAIL);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->GetMotionMaster()->MoveTargetedHome();
            me->DespawnCreaturesInArea(NPC_FRENZIED_CROCOLISK, 500.0f);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
            _firstAugh = true;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || !CheckInRoom())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_VISCOUS_POISON:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, 0))
                            DoCast(target, SPELL_VISCOUS_POISON);
                        events.ScheduleEvent(EVENT_VISCOUS_POISON, 31000);
                        break;
                    case EVENT_SCENT_OF_BLOOD:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, 0))
                            target->AddAura(SPELL_SCENT_OF_BLOOD, target);
                        events.ScheduleEvent(EVENT_SCENT_OF_BLOOD, 31000);
                        events.ScheduleEvent(EVENT_SUMMON_CROCOLISK, 100);
                        break;
                    case EVENT_SUMMON_CROCOLISK:
                    {
                        std::list<Creature*> units;
                        GetCreatureListWithEntryInGrid(units, me, NPC_ADD_STALKER, 500.0f);
                        units.resize(units.size() - 3);
                        for (std::list<Creature*>::iterator itr = units.begin(); itr != units.end(); ++itr)
                            (*itr)->AI()->DoCastAOE(SPELL_SUMMON_CROCOLISK);
                        break;
                    }
                    case EVENT_DUST_FLAIL:
                        DoCastAOE(SPELL_DUST_FLAIL_SUMMON);
                        events.ScheduleEvent(EVENT_ATTACK, 5200);
                        events.ScheduleEvent(EVENT_DUST_FLAIL, 31000);
                        break;
                    case EVENT_ATTACK:
                        me->SetReactState(REACT_AGGRESSIVE);
                        break;
                    case EVENT_SUMMON_AUGH:
                    {
                        if (Creature* stalker = me->FindNearestCreature(NPC_ADD_STALKER, 500.0f))
                        {
                            if (_firstAugh)
                            {
                                stalker->AI()->DoCastAOE(SPELL_SUMMON_AUGH_2);
                                _firstAugh = false;
                                events.ScheduleEvent(EVENT_SUMMON_AUGH, 30500);
                            }
                            else if (!_firstAugh)
                            {
                                stalker->AI()->DoCastAOE(SPELL_SUMMON_AUGH_1);
                                _firstAugh = true;
                                events.ScheduleEvent(EVENT_SUMMON_AUGH, 30500);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    
    };
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_lockmawAI(creature);
    }
};

class npc_lct_augh_battle : public CreatureScript
{
    public:
        npc_lct_augh_battle() :  CreatureScript("npc_lct_augh_battle") { }

        struct npc_lct_augh_battleAI : public ScriptedAI
        {
            npc_lct_augh_battleAI(Creature* creature) : ScriptedAI(creature)
            {
                followTarget = NULL;
            }

            EventMap events;
            Unit* followTarget;

            void EnterCombat(Unit* /*who*/)
            {
                if (me->GetEntry() == BOSS_AUGH)
                {
                    events.ScheduleEvent(EVENT_WHIRLWIND, 5000);
                    events.ScheduleEvent(EVENT_FRENZY, 1);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetHomePosition(AughWP);
                }
            }

            void EnterEvadeMode()
            {
                if (me->GetEntry() == BOSS_AUGH)
                {
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->MoveTargetedHome();
                    _EnterEvadeMode();
                    events.Reset();
                }
            }

            void InitializeAI()
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->AddAura(SPELL_STEALTHED, me);
                if (me->GetEntry() == NPC_AUGH_1)
                {
                    events.ScheduleEvent(EVENT_MOVE, 1000);
                    me->SetInCombatWithZone();
                }
                else if (me->GetEntry() == NPC_AUGH_2)
                {
                    events.ScheduleEvent(EVENT_WHIRLWIND, 1000);
                    me->SetInCombatWithZone();
                }
                else if (me->GetEntry() == BOSS_AUGH)
                {
                    events.ScheduleEvent(EVENT_TALK_OUTRO, 2000);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC);
                }
            }
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() && me->GetEntry() == BOSS_AUGH && me->isInCombat())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE:
                            if (Unit* target = me->FindNearestPlayer(200.0f))
                                me->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), false);
                            me->RemoveAllAuras();
                            events.ScheduleEvent(EVENT_TALK, 5000);
                            break;
                        case EVENT_TALK:
                            Talk(SAY_ATTACK);
                            me->GetMotionMaster()->MovementExpired();
                            events.ScheduleEvent(EVENT_PARALYTIC_BLOW, 1000);
                            break;
                        case EVENT_PARALYTIC_BLOW:
                            DoCast(SPELL_PARALYTIC_BLOW_DART);
                            events.ScheduleEvent(EVENT_TALK_2, 2500);
                            break;
                        case EVENT_TALK_2:
                            Talk(SAY_DESPAWN);
                            me->GetMotionMaster()->MovementExpired();
                            events.ScheduleEvent(EVENT_SMOKE_BOMB, 1000);
                            break;
                        case EVENT_SMOKE_BOMB:
                            me->RemoveAllAuras();
                            me->GetMotionMaster()->MovementExpired();
                            events.CancelEvent(EVENT_PICK_RANDOM_VICTIM);
                            DoCast(me, SPELL_SMOKE_BOMB);
                            events.ScheduleEvent(EVENT_MOVE_OUT, 2000);
                            break;
                        case EVENT_MOVE_OUT:
                            if (Creature* stalker = me->FindNearestCreature(NPC_ADD_STALKER, 500.0f))
                                me->GetMotionMaster()->MovePoint(0, stalker->GetPositionX(), stalker->GetPositionY(), stalker->GetPositionZ(), false);
                            me->DespawnOrUnsummon(3000);
                            break;
                        case EVENT_WHIRLWIND:
                            me->RemoveAurasDueToSpell(SPELL_STEALTHED);
                            me->SetSpeed(MOVE_RUN, 3.9f); // Seen in sniffs
                            me->SetSpeed(MOVE_WALK, 3.9f); // Seen in sniffs
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();

                            DoCast(me, me->GetEntry() == BOSS_AUGH ? SPELL_WHIRLWIND_BATTLE : SPELL_WHIRLWIND);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, 0))
                            {
                                followTarget = target;
                                events.ScheduleEvent(EVENT_FOLLOW_PLAYER, 1);
                            }

                            if (me->GetEntry() != BOSS_AUGH)
                                events.ScheduleEvent(EVENT_SMOKE_BOMB, 21000);
                            else
                            {
                                events.ScheduleEvent(EVENT_WHIRLWIND, 23000);
                                events.ScheduleEvent(EVENT_DRAGONS_BREATH, 8100);
                                events.ScheduleEvent(EVENT_CANCEL_FOLLOW, 8000);
                            }
                            events.ScheduleEvent(EVENT_PICK_RANDOM_VICTIM, 3000);
                            break;
                        case EVENT_PICK_RANDOM_VICTIM:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, 0))
                                followTarget = target;

                            events.ScheduleEvent(EVENT_PICK_RANDOM_VICTIM, 2000);
                            break;
                        case EVENT_FOLLOW_PLAYER:
                            if (followTarget)
                            {
                                Position pos;
                                pos.Relocate(followTarget);
                                me->GetMotionMaster()->MovePoint(0, pos);
                                events.ScheduleEvent(EVENT_FOLLOW_PLAYER, 500);
                            }
                            break;
                        case EVENT_TALK_OUTRO:
                            me->SetWalk(true);
                            me->RemoveAllAuras();
                            TalkToMap(SAY_ANNOUNCE_STEALTH);
                            me->GetMotionMaster()->MovePoint(0, AughWP);
                            events.ScheduleEvent(EVENT_TALK_OUTRO_2, 7000);
                            break;
                        case EVENT_TALK_OUTRO_2:
                            me->SetWalk(false);
                            me->SetFacingTo(0.7f);
                            TalkToMap(SAY_WONDER);
                            events.ScheduleEvent(EVENT_TALK_OUTRO_3, 5000);
                            break;
                        case EVENT_TALK_OUTRO_3:
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_APPLAUD);
                            TalkToMap(SAY_HAPPY);
                            break;
                        case EVENT_DRAGONS_BREATH:
                            DoCast(SPELL_DRAGONS_BREATH);
                            break;
                        case EVENT_FRENZY:
                            DoCast(SPELL_FRENZY);
                            break;
                        case EVENT_CANCEL_FOLLOW:
                            events.CancelEvent(EVENT_FOLLOW_PLAYER);
                            me->GetMotionMaster()->MovementExpired();
                            me->SetReactState(REACT_AGGRESSIVE);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_augh_battleAI(creature);
        }
};

class npc_lct_dust_flail_facing : public CreatureScript
{
    public:
        npc_lct_dust_flail_facing() :  CreatureScript("npc_lct_dust_flail_facing") { }

        struct npc_lct_dust_flail_facingAI : public ScriptedAI
        {
            npc_lct_dust_flail_facingAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void IsSummonedBy(Unit* summoner)
            {
                summoner->ToCreature()->AttackStop();
                summoner->ToCreature()->SetReactState(REACT_PASSIVE);
                summoner->SetFacingToObject(me);
                summoner->ToCreature()->AI()->DoCastAOE(SPELL_DUST_FLAIL);
            }

            void UpdateAI(uint32 diff)
            {
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_dust_flail_facingAI(creature);
        }
};

class npc_lct_dust_flail_target : public CreatureScript
{
    public:
        npc_lct_dust_flail_target() :  CreatureScript("npc_lct_dust_flail_target") { }

        struct npc_lct_dust_flail_targetAI : public ScriptedAI
        {
            npc_lct_dust_flail_targetAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void InitializeAI()
            {
                me->SetReactState(REACT_PASSIVE);
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC);
                me->AddAura(SPELL_DUST_FLAIL_TRIGGERED, me);
            }

            void UpdateAI(uint32 diff)
            {
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_dust_flail_targetAI(creature);
        }
};

class npc_lct_frenzied_crocolisk : public CreatureScript
{
    public:
        npc_lct_frenzied_crocolisk() :  CreatureScript("npc_lct_frenzied_crocolisk") { }

        struct npc_lct_frenzied_crocoliskAI : public ScriptedAI
        {
            npc_lct_frenzied_crocoliskAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void IsSummonedBy(Unit* summoner)
            {
                Map::PlayerList const& player = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator itr = player.begin(); itr != player.end(); ++itr)
                    if (Player* player = itr->getSource())
                        if (player->HasAura(SPELL_SCENT_OF_BLOOD))
                        {
                            me->SetReactState(REACT_PASSIVE);
                            me->AI()->AttackStart(player);
                            player->AddThreat(me, 9000.0f);
                        }
            }

            void EnterCombat(Unit* /*victim*/)
            {
                events.ScheduleEvent(EVENT_FIND_BLOOD_PLAYER, 1000);
            }

            void JustDied(Unit* /*killer*/)
            {
                me->DespawnOrUnsummon(5000);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FIND_BLOOD_PLAYER:
                        {
                            Map::PlayerList const& player = me->GetMap()->GetPlayers();
                            for (Map::PlayerList::const_iterator itr = player.begin(); itr != player.end(); ++itr)
                                if (Player* player = itr->getSource())
                                    if (player->HasAura(SPELL_SCENT_OF_BLOOD))
                                    {
                                        me->AI()->AttackStart(player);
                                        player->AddThreat(me, 9000.0f);
                                    }

                            events.ScheduleEvent(EVENT_FIND_BLOOD_PLAYER, 1000);
                            break;
                        }
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_frenzied_crocoliskAI(creature);
        }
};

class npc_lct_augh : public CreatureScript
{
    public:
        npc_lct_augh() :  CreatureScript("npc_lct_augh") { }

        struct npc_lct_aughAI : public ScriptedAI
        {
            npc_lct_aughAI(Creature* creature) : ScriptedAI(creature)
            {
                done = false;
            }

            EventMap events;
            bool done;

            void Reset()
            {
                done = false;
            }

            void EnterCombat(Unit* who)
            {
                Talk(SAY_AGGRO_INTRO);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
            {
                if (me->GetHealthPct() < 50 && !done)
                {
                    Talk(SAY_ESCAPE_FIGHT);
                    me->AttackStop();
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                    me->SetReactState(REACT_PASSIVE);
                    events.ScheduleEvent(EVENT_SMOKE_BOMB, 1000);
                    done = true;

                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SMOKE_BOMB:
                            DoCastAOE(SPELL_SMOKE_BOMB);
                            events.ScheduleEvent(EVENT_INVISIBLE, 2000);
                            break;
                        case EVENT_INVISIBLE:
                            if (Creature* lockmaw = me->FindNearestCreature(BOSS_LOCKMAW, 500.0f))
                                me->GetMotionMaster()->MovePoint(0, lockmaw->GetPositionX(), lockmaw->GetPositionY(), lockmaw->GetPositionZ(), true);
                            me->DespawnOrUnsummon(3000);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_aughAI(creature);
        }
};

class spell_lct_paralytic_blow_dart : public SpellScriptLoader
{
public:
    spell_lct_paralytic_blow_dart() : SpellScriptLoader("spell_lct_paralytic_blow_dart") { }

    class spell_lct_paralytic_blow_dart_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_lct_paralytic_blow_dart_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (targets.empty())
                return;

            WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);
            targets.clear();
            targets.push_back(target);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_lct_paralytic_blow_dart_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_lct_paralytic_blow_dart_SpellScript();
    }
};

void AddSC_boss_lockmaw_and_augh()
{
    new boss_lockmaw();
    new npc_lct_augh_battle();
    new npc_lct_dust_flail_facing();
    new npc_lct_dust_flail_target();
    new npc_lct_frenzied_crocolisk();
    new npc_lct_augh();
    new spell_lct_paralytic_blow_dart();
}
