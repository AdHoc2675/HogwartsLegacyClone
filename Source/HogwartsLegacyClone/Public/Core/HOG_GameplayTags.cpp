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
	
	//Enemy Ability 
	UE_DEFINE_GAMEPLAY_TAG(Ability_Attack, "Ability.Attack")
	
	UE_DEFINE_GAMEPLAY_TAG(Spell_BasicAttack, "Spell.BasicAttack")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Protego, "Spell.Protego")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Accio, "Spell.Accio")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Incendio, "Spell.Incendio")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Leviosa, "Spell.Leviosa")
	UE_DEFINE_GAMEPLAY_TAG(Spell_Stupefy, "Spell.Stupefy")
	
	
}
