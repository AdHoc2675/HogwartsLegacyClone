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
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill5, "Input.Skill5")

	//State
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead")
	UE_DEFINE_GAMEPLAY_TAG(State_Hit, "State.Hit")
	UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "State.Attacking")
	UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned")
	UE_DEFINE_GAMEPLAY_TAG(State_Burned, "State.Burned")

	//Enemy Ability 
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack1, "Ability.Enemy.MeleeAttack1")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack2, "Ability.Enemy.MeleeAttack2")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack3, "Ability.Enemy.MeleeAttack3")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack4, "Ability.Enemy.MeleeAttack4")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_MeleeAttack5, "Ability.Enemy.MeleeAttack5")
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Dash, "Ability.Enemy.Dash")


	UE_DEFINE_GAMEPLAY_TAG(Spell_BasicAttack, "Spell.BasicAttack")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Protego, "Spell.Protego")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Accio, "Spell.Accio")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Incendio, "Spell.Incendio")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Leviosa, "Spell.Leviosa")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Stupefy, "Spell.Stupefy")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Lumos, "Spell.Lumos")

	//CombatState
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Active, "State.Combat.Active")
	UE_DEFINE_GAMEPLAY_TAG(State_Combat_Inactive, "State.Combat.Inactive")


	// Casting State
	UE_DEFINE_GAMEPLAY_TAG(State_Casting_Active, "State.Casting.Active")
	UE_DEFINE_GAMEPLAY_TAG(State_Casting_Inactive, "State.Casting.Inactive")

	//Protage State
	UE_DEFINE_GAMEPLAY_TAG(State_Spell_Protego_Active, "State.Spell.Protego.Active")
	UE_DEFINE_GAMEPLAY_TAG(State_Spell_Protego_ParrySuccess, "State.Spell.Protego.ParrySuccess")
	
	UE_DEFINE_GAMEPLAY_TAG(State_Spell_Lumos_Active, "State.Spell.Lumos.Active")

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
	
	//Interaction
	UE_DEFINE_GAMEPLAY_TAG(Interaction_Burn, "Interaction.Burn")
	

	//Unit Tags
	UE_DEFINE_GAMEPLAY_TAG(Unit_Player, "Unit.Player")
	UE_DEFINE_GAMEPLAY_TAG(Unit_Enemy_Goblin, "Unit.Enemy.Goblin")
	UE_DEFINE_GAMEPLAY_TAG(Unit_Enemy_Troll, "Unit.Enemy.Troll")
	UE_DEFINE_GAMEPLAY_TAG(Unit_Enemy_Dementor, "Unit.Enemy.Dementor")
	
	//Minimap Tags
	UE_DEFINE_GAMEPLAY_TAG(Minimap_BossArea, "Minimap.BossArea")

}
