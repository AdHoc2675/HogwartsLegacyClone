#include "HOG_GameplayTags.h"

namespace HOGGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Team_Player, "Team.Player")
	UE_DEFINE_GAMEPLAY_TAG(Team_Enemy, "Team.Enemy")
	UE_DEFINE_GAMEPLAY_TAG(Team_Object, "Team.Object")


	//입력
	UE_DEFINE_GAMEPLAY_TAG(Input_Move, "Input.Move")
	UE_DEFINE_GAMEPLAY_TAG(Input_Look, "Input.Look")
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump")
	UE_DEFINE_GAMEPLAY_TAG(Input_Interact, "Input.Interact")

	//Ability 입력
	UE_DEFINE_GAMEPLAY_TAG(Input_Primary, "Input.Primary")
	UE_DEFINE_GAMEPLAY_TAG(Input_Defense, "Input.Defense")
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill1, "Input.Skill1")
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill2, "Input.Skill2")
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill3, "Input.Skill3")
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill4, "Input.Skill4")

	//State
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead")
	UE_DEFINE_GAMEPLAY_TAG(State_Hit, "State.Hit")
	UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "State.Attacking")
	UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned")

	//Enemy Ability 
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack, "Ability.Enemy.MeleeAttack")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack2, "Ability.Enemy.MeleeAttack2")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Dash, "Ability.Enemy.Dash")


	UE_DEFINE_GAMEPLAY_TAG(Spell_BasicAttack, "Spell.BasicAttack")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Protego, "Spell.Protego")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Accio, "Spell.Accio")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Incendio, "Spell.Incendio")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Leviosa, "Spell.Leviosa")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Stupefy, "Spell.Stupefy")

	//CombatState
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Active, "State.Combat.Active")
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Inactive, "State.Combat.Inactive")


	// Casting State
	UE_DEFINE_GAMEPLAY_TAG(State_Casting_Active, "State.Casting.Active")
	UE_DEFINE_GAMEPLAY_TAG(State_Casting_Inactive, "State.Casting.Inactive")

	//Protage State
	UE_DEFINE_GAMEPLAY_TAG(State_Spell_Protego_Active, "State.Spell.Protego.Active")
	UE_DEFINE_GAMEPLAY_TAG(State_Spell_Protego_ParrySuccess, "State.Spell.Protego.ParrySuccess")

	//Damage
	UE_DEFINE_GAMEPLAY_TAG(Damage_Melee, "Damage.Melee")
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage")

	//Event
	UE_DEFINE_GAMEPLAY_TAG(Event_Weapon_Hit, "Event.Weapon.Hit")


	// Leviosa State	
	UE_DEFINE_GAMEPLAY_TAG(State_Spell_Leviosa_Levitated, "State.Spell.Leviosa.Levitated")


	//Interactable Object
	UE_DEFINE_GAMEPLAY_TAG(Interactable_Chest_Opened, "Interactable.Chest.Opened")
	UE_DEFINE_GAMEPLAY_TAG(Interactable_Chest_Closed, "Interactable.Chest.Closed")

	UE_DEFINE_GAMEPLAY_TAG(Interactable_Burnable_Unlit, "Interactable.Burnable.Unlit")
	UE_DEFINE_GAMEPLAY_TAG(Interactable_Burnable_Lit, "Interactable.Burnable.Lit")

	UE_DEFINE_GAMEPLAY_TAG(Interactable_Levitatable_Grounded, "Interactable.Levitatable.Grounded")
	
	UE_DEFINE_GAMEPLAY_TAG(Interactable_AccioPlatform, "Interactable.AccioPlatform")
	UE_DEFINE_GAMEPLAY_TAG(Interactable_AccioTarget, "Interactable.AccioTarget")

	//Unit Tags
	UE_DEFINE_GAMEPLAY_TAG(Unit_Player, "Unit.Player")
	UE_DEFINE_GAMEPLAY_TAG(Unit_Enemy_Goblin, "Unit.Enemy.Goblin")
	UE_DEFINE_GAMEPLAY_TAG(Unit_Enemy_Troll, "Unit.Enemy.Troll")
	UE_DEFINE_GAMEPLAY_TAG(Unit_Enemy_Dementor, "Unit.Enemy.Dementor")

}
