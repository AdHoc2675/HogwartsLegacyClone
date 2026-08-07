# HogwartsLegacyClone

**Unreal Engine 5.4** 기반의 3인칭 액션 RPG 프로젝트입니다.  
호그와트 레거시의 핵심 전투·마법·탐험 메커니즘을 C++로 재현하는 것을 목표로 합니다.

---

## 기술 스택

| 항목 | 사용 기술 |
|---|---|
| 엔진 | Unreal Engine 5.4 |
| 언어 | C++ / Blueprint (혼합) |
| 빌드 | UnrealBuildTool, BuildSettingsVersion V5 |
| 입력 | Enhanced Input System |
| 능력치·전투 | Gameplay Ability System (GAS) |
| AI | Behavior Tree + AIModule |
| VFX | Niagara |
| UI | UMG (Slate / SlateCore) |

---

## 프로젝트 구조

```
Source/HogwartsLegacyClone/
├── Public/
│   ├── Character/            # 캐릭터 계층 (Player, Enemy)
│   ├── Component/            # 전투·스펠·락온·무기 컴포넌트
│   ├── Core/                 # 공용 Enum, Struct, GameplayTag 정의
│   ├── Data/                 # DataAsset (SpellDefinition, InputConfig, AbilitySet 등)
│   ├── GAS/                  # AbilitySystemComponent, AttributeSet, GameplayAbility
│   ├── GameFramework/        # GameInstance, GameMode, PlayerController, PlayerState
│   ├── Interactable/         # 상호작용 오브젝트 (상자, 화로, 부유물 등)
│   ├── Minimap/              # 미니맵 캡처·마커·위젯
│   ├── Notify/               # 애니메이션 노티파이 (콤보, VFX, 발사)
│   ├── Pool/                 # 오브젝트 풀 (데미지 넘버)
│   ├── Subsystem/            # 월드 서브시스템 (미니맵)
│   └── UI/                   # HUD, 위젯 컨트롤러, 스펠 슬롯, 체력바
└── Private/
    └── (위 Public 구조와 1:1 대응)
```

---

## 아키텍처

### 캐릭터 계층

```
ACharacter
 └─ ABaseCharacter
     │  · TeamTag (GameplayTag 기반 아군/적군 식별)
     │  · CombatComponent (전투 처리 위임)
     │  · DataTable 기반 Attribute 초기화
     │  · 공통 사망 처리 (HandleDeath)
     │
     ├─ APlayerCharacterBase
     │   │  · 3인칭 카메라 (SpringArm + Camera)
     │   │  · LockOnComponent (자동 타겟팅)
     │   │  · 입력 처리 (Move, Look, Jump, Interact, Ability)
     │   │  · 완드 메시 가시성 (전투/시전 상태 연동)
     │   │  · 콤보 큐잉 시스템
     │   │  · 스펠 VFX 큐잉 시스템
     │   │  · Pre-Cast Facing (시전 전 타겟 방향 회전)
     │   └─ APlayerCharacter
     │
     └─ AEnemyCharacterBase (IAbilitySystemInterface)
         │  · 자체 ASC 호스팅
         │  · BehaviorTree 기반 AI
         │  · 활성화 모드 (Immediate / WaitForSignal)
         │  · EnemyDamageHandler
         │
         ├─ AMeleeEnemyCharacterBase
         │   ├─ AGoblinEnemyCharacter
         │   └─ ATrollEnemyCharacter
         └─ ADementorEnemyCharacter
```

> **ASC 호스팅 전략**: Player는 `PlayerState`에, Enemy는 자기 자신에 ASC를 배치합니다.  
> `BaseCharacter::GetCharacterAbilitySystemComponent()`가 이 차이를 투명하게 추상화합니다.

---

### Gameplay Ability System (GAS)

#### AttributeSet

| 속성 | 설명 |
|---|---|
| `Health` | 현재 체력 |
| `MaxHealth` | 최대 체력 |
| `AttackPower` | 공격력 |

- `PreAttributeChange`에서 값 클램핑, `PostGameplayEffectExecute`에서 사망 판정 처리
- **DataTable 기반 초기화**: `UnitTag`로 Row를 검색하여 캐릭터별 초기 스탯 적용

#### Ability 계층

```
UGameplayAbility
 └─ UGA_Base
     │  · InputTag 슬롯
     │  · HOG ASC / Controller / Pawn / PlayerState 접근 헬퍼
     │  · 활성화/종료 디버그 로깅
     │
     ├─ UGA_SpellBase (Abstract)
     │   · SpellDefinition 조회 (GameInstance SpellRegistry)
     │   · 시전 검증 (CanCastSpell → CheckResult)
     │   · 시전 맥락 분리 (Normal / ParryCounter / SpecialFreeCast)
     │   · Pre-Cast Facing (시전 전 타겟 방향 회전 대기)
     │   · LineTrace Beam VFX Queue (노티파이 시점에 VFX 생성)
     │   · LockOnComponent 연동 타겟 소모
     │
     ├─ UGA_HitReact
     └─ UGA_EnemyBase
```

#### AbilitySystemComponent

`UHOGAbilitySystemComponent`는 **InputTag 기반 Ability 라우팅**을 구현합니다:

```
InputTag Press → AbilityInputTagPressed()
                     → InputPressedSpecHandles에 수집
                         → ProcessAbilityInput()에서 일괄 TryActivate
```

---

### 스펠 데이터 파이프라인

```
[DA_SpellDefinition]          [HOG_GameInstance]              [GA_SpellBase]
  SpellID (Tag)        ──▶   SpellRegistry                ──▶  GetSpellDefinition()
  DisplayName                TMap<Tag, Definition>              GetCooldownSeconds()
  Icon                       BuildSpellRegistry()               GetBaseDamage()
  CooldownSeconds                                               GetCastRange()
  BaseDamage                                                    DoesTargetMeetRequirements()
  CastRange
  TargetRequiredTags
  TargetBlockedTags
```

**핵심 원칙**: 스펠의 "진실의 원천(SSOT)"은 `DA_SpellDefinition`(DataAsset)입니다.  
Ability 클래스는 하드코딩을 최소화하고, Definition을 런타임에 조회하여 동작합니다.

---

### 전투 시스템

#### CombatComponent

캐릭터에 부착되는 전투 처리 컴포넌트입니다.

**데미지 파이프라인:**
```
FDamageRequest 수신
 → ValidateDamageRequest (유효성)
 → ShouldIgnoreDamage (무적·아군 판별)
 → TryHandleProtegoDefense (패링/블록 판정)
 → ApplyDamageEffect (GE 적용)
 → HandleDamageResult (사망 체크, 이벤트 브로드캐스트)
```

**프로테고(Protego) 방어 시스템:**
- **Parry Window**: 시전 직후 짧은 시간 — 패링 성공 시 반격 기회
- **Block Window**: Parry 이후 더 긴 시간 — 데미지 무효화
- 시간 기반 윈도우 관리 (`GetWorld()->GetTimeSeconds()`)

#### SpellComponent

`PlayerState`에 부착되는 스펠 런타임 관리 컴포넌트입니다.

| 기능 | 설명 |
|---|---|
| 쿨타임 관리 | `TMap<SpellID, RemainingTime>` — Tick 기반 실시간 갱신 |
| 시전 검증 | SpellID 유효성 → Owner 유효성 → 전체 잠금 → 상태 태그 차단 → 쿨타임 |
| 시전 맥락 정책 | Normal(쿨타임 O), ParryCounter(쿨타임 X), SpecialFreeCast(쿨타임 X) |
| 차단 태그 | `State.Dead`, `State.Debuff.Stunned`, `State.Debuff.Silenced` 등 |

#### LockOnComponent

플레이어 캐릭터의 자동 타겟팅 시스템입니다.

- **실시간 갱신**: 설정 간격(0.05s)마다 최적 타겟 재계산
- **스코어링 공식**: `AngleWeight(0.7) × 각도 + DistanceWeight(0.3) × 거리`
- **LOS(Line of Sight)** 검증 지원
- **타겟 외곽선**: Custom Stencil Value로 적(111) / 오브젝트(112) 구분 아웃라인 표시
- **타겟 변경 이벤트**: `OnLockOnTarget` / `OnLockOnReleased` 델리게이트

---

### 입력 시스템

**EnhancedInput + DataAsset 주도 바인딩** 구조입니다.

```
DA_InputConfig (DataAsset)
  │  GameplayTag ↔ InputAction 매핑 테이블
  │  DefaultMappingContext
  │
  ▼
HOG_PlayerController::SetupInputComponent()
  ├─ 기본 액션 바인딩 (Move, Look, Jump, Interact)
  │    → PlayerCharacterBase::Input_Move() 등 직접 호출
  │
  └─ 어빌리티 액션 바인딩 (Tag 기반)
       → HOGAbilitySystemComponent::AbilityInputTagPressed()
       → ProcessAbilityInput()에서 일괄 TryActivateAbility
```

---

### 상호작용 시스템

`AInteractableBase`를 상속받는 월드 오브젝트들로 구성됩니다.

| 클래스 | 상태 태그 | 상호작용 |
|---|---|---|
| `InteractableChest` | `Chest_Opened` / `Chest_Closed` | 접근하여 열기 |
| `InteractableBurnable` | `Burnable_Unlit` / `Burnable_Lit` | Incendio로 점화 |
| `InteractableLevitatable` | `Levitatable_Grounded` | Leviosa로 공중 부양 |
| `InteractableAccioPlatform` | `AccioPlatform` | Accio 퍼즐 플랫폼 |
| `InteractableAccioTarget` | `AccioTarget` | Accio 당기기 대상 |

모든 상호작용 오브젝트는 자체 ASC를 보유하며, `IInteractableInterface`를 통해 일관된 상호작용 API를 제공합니다.  
`BeginPlay` 시 `Team.Object` 태그가 자동 부여되어 LockOn 시스템의 타겟 후보로 인식됩니다.

---

### 구현 마법 목록

| 마법 | GameplayTag | 주요 기능 |
|---|---|---|
| **기본 공격** | `Spell.BasicAttack` | 콤보 시스템 연동, AnimNotify 기반 발사 타이밍 |
| **프로테고** | `Spell.Protego` | Parry/Block Window, 패링 성공 시 반격 트리거 |
| **아씨오** | `Spell.Accio` | AccioPlatform/AccioTarget 상호작용, 오브젝트 당기기 |
| **인센디오** | `Spell.Incendio` | 화염 데미지, Burnable 오브젝트 점화, 화상 상태이상 |
| **레비오사** | `Spell.Leviosa` | Levitatable 오브젝트 부양, 적 공중 부양 |
| **스투페파이** | `Spell.Stupefy` | 기절(Stunned) 효과 부여 |
| **루모스** | `Spell.Lumos` | 조명 마법, 토글 방식 활성화 |

---

### AI 시스템

적 캐릭터는 **BehaviorTree** 기반 AI로 동작합니다.

```
Source/HogwartsLegacyClone/Public/Character/Enemy/
├── AIController/       # 커스텀 AI 컨트롤러
├── BTTask/             # BehaviorTree 커스텀 태스크
├── Anim/               # 적 애니메이션 관련
├── Handler/            # 데미지 핸들러
├── Helper/             # AI 유틸리티
├── Notify/             # 적 애니메이션 노티파이
└── Interface/          # AI 인터페이스
```

| 적 유형 | 클래스 | 전투 스타일 |
|---|---|---|
| 고블린 | `GoblinEnemyCharacter` | 근접 공격 (MeleeAttack 1~5) |
| 트롤 | `TrollEnemyCharacter` | 근접 공격 + 대쉬 |
| 디멘터 | `DementorEnemyCharacter` | 원거리/특수 공격 |

**활성화 모드**: `Immediate`(즉시 활성) 또는 `WaitForSignal`(시그널 대기)로 설정 가능합니다.

---

### UI 시스템

```
HOG_PlayerController
 └─ HOG_WidgetController (중앙 위젯 컨트롤러)
     ├─ HOGPlayerHUDBase
     │   └─ HOGPlayerWidget (체력, 스펠 슬롯, 쿨타임 표시)
     ├─ HOGEnemyHUDBase
     │   └─ HOGEnemyWidget (적 체력바)
     ├─ DamageNumberWidget (데미지 숫자 — 오브젝트 풀링)
     ├─ MinimapWidget (미니맵)
     ├─ SubtitleWidget (대화 자막)
     ├─ HpWidget (체력 바)
     └─ CharacterInfoWidget (캐릭터 정보)
```

- **DamageNumberPool**: 데미지 숫자를 위젯 풀링으로 관리하여 GC 부담 최소화
- **스펠 해금 UI**: `UnlockSpellUI(SpellID)` — 런타임 마법 해금 시 UI 동기화

---

### 기타 시스템

#### 미니맵
`MinimapSubsystem`과 전용 컴포넌트로 구성됩니다.

| 클래스 | 역할 |
|---|---|
| `MinimapCaptureComponent` | 씬 캡처 (탑뷰 렌더링) |
| `MinimapMarkerComponent` | 마커 등록 (플레이어, 적, POI) |
| `MinimapIconOverlay` | 아이콘 오버레이 렌더링 |
| `MinimapData` | 미니맵 설정 데이터 |
| `MinimapWidget` | HUD 미니맵 위젯 |

#### BGM 시스템
`HOG_PlayerController`에 내장된 배경음악 관리 시스템입니다.

- `PlayBGMWithFade(NewBGM, FadeIn, FadeOut)` — 크로스페이드 전환
- `StopBGMWithFade(FadeOut)` — 페이드아웃 정지
- `HOG_MusicTrigger` — 영역 진입 시 자동 BGM 전환

#### 대화 시스템
- `HOG_DialogTrigger` — 트리거 볼륨 기반 대화 시작
- `SubtitleWidget` — 화면 하단 자막 표시

---

### GameplayTag 체계

```
Team.*                     # 팀 구분 (Player, Enemy, Object)
Input.*                    # 입력 바인딩 (Move, Look, Jump, Interact, Primary, Defense, Skill1~5)
State.*                    # 상태 (Dead, Hit, Attacking, Stunned, Burned)
State.Combat.*             # 전투 상태 (Active, Inactive)
State.Casting.*            # 시전 상태 (Active, Inactive)
State.Spell.*              # 마법별 상태 (Protego_Active, Lumos_Active, Leviosa_Levitated 등)
Spell.*                    # 마법 식별 (BasicAttack, Protego, Accio, Incendio, Leviosa, Stupefy, Lumos)
Ability.Enemy.*            # 적 어빌리티 (MeleeAttack1~5, Dash)
Damage.*                   # 데미지 타입 (Melee)
Event.*                    # 이벤트 (Weapon_Hit)
Interactable.*             # 상호작용 상태 (Chest, Burnable, Levitatable, AccioPlatform 등)
Interaction.*              # 상호작용 타입 (Burn)
Unit.*                     # 유닛 식별 (Player, Enemy_Goblin, Enemy_Troll, Enemy_Dementor)
Minimap.*                  # 미니맵 (BossArea)
```

---

## 설계 원칙

- **데이터 주도(Data-Driven)**: 스펠 정의, 입력 설정, 어빌리티 셋, 속성 초기값 모두 DataAsset/DataTable로 분리하여 코드 수정 없이 밸런싱 가능
- **컴포넌트 분리**: Combat, Spell, LockOn, Weapon을 독립 컴포넌트로 분리하여 관심사 격리
- **GAS 정석 패턴**: PlayerState ASC 호스팅, InputTag 라우팅, AttributeSet 값 클램핑, GE 기반 데미지 적용
- **확장 가능한 상호작용**: ASC + GameplayTag 기반 오브젝트 상호작용으로 새 오브젝트 추가 용이
- **유연한 시전 정책**: Normal / ParryCounter / SpecialFreeCast 맥락 분리로 다양한 발동 조건 지원

---

## 빌드

```bash
# Unreal Engine 5.4 설치 필요
# Visual Studio 2022 또는 Rider 권장
# .uproject 파일을 엔진에서 열어 빌드
```

## 콘텐츠 에셋

| 디렉토리 | 내용 |
|---|---|
| `Content/_BP/` | 블루프린트 (캐릭터, GA, GE, UI, 입력, 상호작용) |
| `Content/Assets/` | 폰트, VFX 팩, 포탈 시스템, 환경 에셋 |
| `Content/Maps/` | 레벨 맵 |
| `Content/ParagonKhaimera/` | 캐릭터 메시 에셋 |
