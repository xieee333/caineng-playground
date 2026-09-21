# 彩能游乐场第一版垂直切片实现计划

> 历史计划：2026-09-21 起执行路线见 [UE 阶段计划](2026-09-21-ue-party-shooter.md)。本文 Unity 路线仅供追溯。

> **面向 AI 代理的工作者：** 必需子技能：使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在 Unity 中交付一个 6 人混战风格的可玩垂直切片：玩家可以喷射彩能、钻入表面、弹跳、短时滞空，使用非致命玩具枪械互相捣乱，完成可选公共目标并触发全场狂欢事件。

**架构：** 使用数据驱动的 ScriptableObject 定义枪械、表面材质、公共目标和狂欢事件；运行时由小型纯 C# 服务处理表面状态、效果解析、欢乐值和事件调度。第一阶段以本地玩家加 5 个可重复的 Bot 模拟 6 人混战，先验证娱乐性；所有判定通过接口隔离，便于第二阶段接入 Unity Netcode for GameObjects，而不把网络依赖写进核心规则。

**技术栈：** Unity LTS、C#、URP、Input System、Unity Test Framework、ScriptableObject、NavMesh/简化 Bot 导航。第一阶段不引入在线服务、账号系统或永久数值成长。

---

## 文件结构与职责

计划中会创建以下目录和文件，单个运行时文件只承担一个主要职责：

```text
Assets/
  GameData/
    Surfaces/              # 液态、弹跳、飘浮彩能定义
    Weapons/               # 6 种玩具枪械定义
    Abilities/             # 6 种主动技能定义
    Objectives/            # 公共目标定义
    CarnivalEvents/        # 狂欢事件定义
  Prefabs/
    Player.prefab
    Bot.prefab
    Projectiles/
    WorldInteractables/
  Scenes/Prototype.unity
  Scripts/
    Runtime/Core/          # 纯数据和规则接口
    Runtime/Surfaces/      # 表面网格、喷射和移动
    Runtime/Combat/        # 枪械、投射物、状态效果、泡泡重生
    Runtime/Objectives/    # 公共目标、欢乐值、狂欢槽和事件
    Runtime/Players/       # 玩家输入、移动、Bot
    Runtime/UI/            # HUD 和结算称号
  Tests/
    EditMode/
    PlayMode/
  Art/Prototype/           # URP 材质、粒子、占位模型
  Audio/Prototype/
  Input/PlayerInput.inputactions
  Settings/PrototypeTuning.asset
docs/superpowers/plans/...
```

第一阶段的网络同步不在本计划范围内；`IRuleEventSink`、`IPlayerStateView` 和 `IObjectiveStateView` 会在核心层定义，使网络层可以在第二个计划中替换本地实现。

## 任务 1：建立 Unity 工程和可测试程序集

**文件：**
- 创建：`ProjectSettings/ProjectVersion.txt`、`ProjectSettings/ProjectSettings.asset`、`Packages/manifest.json`（由 Unity LTS 生成）
- 创建：`Assets/Scripts/Runtime/Runtime.asmdef`
- 创建：`Assets/Tests/EditMode/Tests.EditMode.asmdef`
- 创建：`Assets/Tests/PlayMode/Tests.PlayMode.asmdef`
- 创建：`.editorconfig`

- [ ] **步骤 1：创建 Unity LTS 工程并固定项目路径**

在 PowerShell 中从仓库根目录执行：

```powershell
Unity.exe -batchmode -nographics -quit -createProject "D:\project\caineng-playground"
```

预期：命令返回 0，生成 `Assets`、`Packages` 和 `ProjectSettings`。

- [ ] **步骤 2：配置 URP、Input System 和测试包**

在 Unity Package Manager 中安装项目当前编辑器兼容的 URP、Input System 和 Test Framework；将包依赖写入 `Packages/manifest.json`，并关闭旧输入后端，使 `PlayerInput.inputactions` 成为唯一输入来源。

- [ ] **步骤 3：创建程序集边界**

`Assets/Scripts/Runtime/Runtime.asmdef` 只引用 Unity Runtime、Input System 和 Netcode 接口所需的最小程序集；编辑器测试程序集引用 Runtime，PlayMode 测试程序集引用 Runtime 与 Test Framework。

- [ ] **步骤 4：运行编译检查并提交**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -quit -logFile "D:\project\caineng-playground\Logs\bootstrap.log"
git add Assets Packages ProjectSettings .editorconfig
git commit -m "chore: bootstrap unity prototype"
```

预期：Unity 无编译错误，提交只包含工程骨架。

## 任务 2：定义纯规则模型和第一批失败测试

**文件：**
- 创建：`Assets/Scripts/Runtime/Core/InteractionEffectKind.cs`
- 创建：`Assets/Scripts/Runtime/Core/SurfaceKind.cs`
- 创建：`Assets/Scripts/Runtime/Core/InteractionEffect.cs`
- 创建：`Assets/Scripts/Runtime/Core/ScoreEvent.cs`
- 创建：`Assets/Scripts/Runtime/Core/RuleEvents.cs`
- 创建：`Assets/Tests/EditMode/InteractionEffectTests.cs`
- 创建：`Assets/Tests/EditMode/RuleEventsTests.cs`

- [ ] **步骤 1：先写枚举和效果值对象**

```csharp
namespace Caineng.Playground.Core;

public enum SurfaceKind { None, Liquid, Bounce, Float }
public enum InteractionEffectKind { Knockback, Pull, Bubble, Freeze, Tether, Teleport }

public readonly record struct InteractionEffect(
    InteractionEffectKind Kind,
    float Magnitude,
    float Duration,
    UnityEngine.Vector3 Direction,
    string SourceId);

public readonly record struct ScoreEvent(
    string ActorId,
    string Reason,
    int Amount,
    UnityEngine.Vector3 WorldPosition);
```

- [ ] **步骤 2：写失败测试，固定规则语义**

```csharp
[Test]
public void BubbleEffectHasPositiveDuration()
{
    var effect = new InteractionEffect(InteractionEffectKind.Bubble, 1f, 2f,
        UnityEngine.Vector3.up, "bubble-cannon");
    Assert.That(effect.Duration, Is.GreaterThan(0f));
}

[Test]
public void ScoreEventsPreserveReasonAndActor()
{
    var score = new ScoreEvent("p1", "bounce-save", 10, UnityEngine.Vector3.zero);
    Assert.That(score.ActorId, Is.EqualTo("p1"));
    Assert.That(score.Reason, Is.EqualTo("bounce-save"));
}
```

- [ ] **步骤 3：运行 EditMode 测试确认先失败或无法编译**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform editmode -testResults "D:\project\caineng-playground\TestResults\task-2-before.xml" -quit
```

- [ ] **步骤 4：补齐 `RuleEvents.cs` 和测试装配引用，让测试通过**

`RuleEvents.cs` 提供 `IRuleEventSink.Publish(ScoreEvent)` 与 `IRuleEventSink.Publish(InteractionEffect)` 两个方法；测试用内存实现记录事件数量和顺序。

- [ ] **步骤 5：提交**

```powershell
git add Assets/Scripts/Runtime/Core Assets/Tests/EditMode
git commit -m "feat: define core interaction rules"
```

## 任务 3：实现彩能表面网格和移动规则

**文件：**
- 创建：`Assets/Scripts/Runtime/Surfaces/SurfaceCell.cs`
- 创建：`Assets/Scripts/Runtime/Surfaces/PaintableSurfaceGrid.cs`
- 创建：`Assets/Scripts/Runtime/Surfaces/SprayTool.cs`
- 创建：`Assets/Scripts/Runtime/Surfaces/ColorTraversalMotor.cs`
- 创建：`Assets/Tests/EditMode/PaintableSurfaceGridTests.cs`
- 创建：`Assets/Tests/EditMode/ColorTraversalMotorTests.cs`

- [ ] **步骤 1：写表面网格失败测试**

```csharp
[Test]
public void SprayChangesTheNearestCell()
{
    var grid = new PaintableSurfaceGrid(4, 4, 1f, UnityEngine.Vector3.zero);
    grid.ApplySpray(new UnityEngine.Vector3(1.2f, 0f, 2.1f), SurfaceKind.Bounce);
    Assert.That(grid.GetCell(1, 2).Kind, Is.EqualTo(SurfaceKind.Bounce));
}

[Test]
public void EmptyCellDoesNotGrantTraversalBoost()
{
    var motor = new ColorTraversalMotor();
    var result = motor.Resolve(SurfaceKind.None, 1f);
    Assert.That(result.SpeedMultiplier, Is.EqualTo(1f));
}
```

- [ ] **步骤 2：实现确定性的世界坐标到网格坐标转换**

`PaintableSurfaceGrid.ApplySpray(Vector3 worldPoint, SurfaceKind kind)` 将点转换为边界内的整数格；超出网格的点忽略，不抛异常。`SurfaceCell` 只保存当前材质和最后修改时间，不保存 Unity 组件引用。

- [ ] **步骤 3：实现三种移动规则**

```csharp
public TraversalResult Resolve(SurfaceKind kind, float baseSpeed)
{
    return kind switch
    {
        SurfaceKind.Liquid => new TraversalResult(baseSpeed * 2.2f, canSubmerge: true, canWallRide: true),
        SurfaceKind.Bounce => new TraversalResult(baseSpeed * 1.1f, canSubmerge: false, canWallRide: false),
        SurfaceKind.Float => new TraversalResult(baseSpeed * 0.8f, canSubmerge: false, canWallRide: false, gravityScale: 0.25f),
        _ => new TraversalResult(baseSpeed, canSubmerge: false, canWallRide: false)
    };
}
```

- [ ] **步骤 4：运行 EditMode 测试并提交**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform editmode -testResults "D:\project\caineng-playground\TestResults\task-3.xml" -quit
git add Assets/Scripts/Runtime/Surfaces Assets/Tests/EditMode
git commit -m "feat: add paintable surfaces and traversal rules"
```

预期：网格边界、喷射定位和三种移动规则测试全部通过。

## 任务 4：实现玩家移动、泡泡重生和 Bot 移动

**文件：**
- 创建：`Assets/Input/PlayerInput.inputactions`
- 创建：`Assets/Scripts/Runtime/Players/PlayerMotor.cs`
- 创建：`Assets/Scripts/Runtime/Players/PlayerInputRouter.cs`
- 创建：`Assets/Scripts/Runtime/Combat/BubbleRespawnController.cs`
- 创建：`Assets/Scripts/Runtime/Players/BotBrain.cs`
- 创建：`Assets/Tests/PlayMode/PlayerTraversalPlayModeTests.cs`
- 创建：`Assets/Tests/PlayMode/PrototypeTestHarness.cs`

- [ ] **步骤 1：配置输入动作**

创建 `Move`、`Look`、`Jump`、`Spray`、`PrimaryTool`、`Ability`、`Interact` 七个动作；`PlayerInputRouter` 只把输入转换为无状态命令，不在输入层直接修改生命或分数。

- [ ] **步骤 2：先写 PlayMode 失败测试**

```csharp
[UnityTest]
public IEnumerator BubbleRespawnReturnsPlayerAfterThreeSeconds()
{
    var player = SpawnTestPlayer();
    player.GetComponent<BubbleRespawnController>().Bubbleize();
    yield return new WaitForSeconds(3.1f);
    Assert.That(player.activeSelf, Is.True);
}
```

- [ ] **步骤 3：实现移动状态机**

`PlayerMotor` 维护 `Grounded`、`Airborne`、`Submerged`、`Bubble` 四个状态；每帧从 `PaintableSurfaceGrid` 查询脚下/前方表面，调用 `ColorTraversalMotor.Resolve`，再更新速度、重力和墙面移动。所有状态切换发出 `RuleEvents`，供欢乐值和 HUD 使用。

- [ ] **步骤 4：实现五个 Bot 的简单行为**

Bot 只需要选择最近公共目标、最近彩泡或最近玩家，按固定权重决定喷射、追逐、逃离和干扰；不做复杂战术 AI。Bot 的导航路径必须通过同一套 `PlayerMotor`，以便验证玩家与 Bot 的移动规则一致。

- [ ] **步骤 5：运行 PlayMode 测试并提交**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform playmode -testResults "D:\project\caineng-playground\TestResults\task-4.xml" -quit
git add Assets/Input Assets/Scripts/Runtime/Players Assets/Scripts/Runtime/Combat/BubbleRespawnController.cs Assets/Tests/PlayMode
git commit -m "feat: add traversal players and bubble respawn"
```

## 任务 5：实现玩具枪械和非致命效果

**文件：**
- 创建：`Assets/Scripts/Runtime/Combat/WeaponDefinition.cs`
- 创建：`Assets/Scripts/Runtime/Combat/ToyWeaponController.cs`
- 创建：`Assets/Scripts/Runtime/Combat/ToyProjectile.cs`
- 创建：`Assets/Scripts/Runtime/Combat/StatusEffectController.cs`
- 创建：`Assets/Scripts/Runtime/Combat/InteractionResolver.cs`
- 创建：`Assets/GameData/Weapons/*.asset`（6 个定义）
- 创建：`Assets/Tests/EditMode/InteractionResolverTests.cs`

- [ ] **步骤 1：写效果解析失败测试**

```csharp
[Test]
public void KnockbackMovesTargetAwayFromImpact()
{
    var target = new TestTarget(UnityEngine.Vector3.zero);
    var resolver = new InteractionResolver();
    resolver.Apply(target, new InteractionEffect(InteractionEffectKind.Knockback, 4f, 0.2f,
        UnityEngine.Vector3.right, "firework"));
    Assert.That(target.LastVelocity.x, Is.GreaterThan(0f));
}

[Test]
public void BubbleEffectPreventsDamageUntilRespawn()
{
    var target = new TestTarget(UnityEngine.Vector3.zero);
    var resolver = new InteractionResolver();
    resolver.Apply(target, new InteractionEffect(InteractionEffectKind.Bubble, 1f, 2f,
        UnityEngine.Vector3.up, "bubble-cannon"));
    Assert.That(target.IsBubble, Is.True);
    Assert.That(target.CanReceiveDamage, Is.False);
}
```

- [ ] **步骤 2：实现 6 把工具武器**

每个 `WeaponDefinition` 指定 `primaryEffect`、`secondaryEffect`、射程、冷却和视觉预制体。实现泡泡炮、吸力炮、画线枪、气球炮、回旋枪和烟花炮；每把枪至少有一个不依赖精准爆头的空间/物理效果。

- [ ] **步骤 3：实现状态持续时间和解除规则**

`StatusEffectController` 保存按 `InteractionEffectKind` 索引的结束时间；同种效果刷新时间，不无限叠加；泡泡、冻结和绑缚有显式解除提示。伤害只作为低权重的打断和欢乐值来源，不作为主要胜负机制。

- [ ] **步骤 4：运行 EditMode 测试并提交**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform editmode -testResults "D:\project\caineng-playground\TestResults\task-5.xml" -quit
git add Assets/Scripts/Runtime/Combat Assets/GameData/Weapons Assets/Tests/EditMode/InteractionResolverTests.cs
git commit -m "feat: add nonlethal toy weapon interactions"
```

## 任务 6：实现主动技能、公共目标和欢乐值

**文件：**
- 创建：`Assets/Scripts/Runtime/Combat/AbilityDefinition.cs`
- 创建：`Assets/Scripts/Runtime/Combat/AbilityController.cs`
- 创建：`Assets/Scripts/Runtime/Objectives/PublicObjectiveController.cs`
- 创建：`Assets/Scripts/Runtime/Objectives/JoyScoreService.cs`
- 创建：`Assets/Scripts/Runtime/Objectives/CarnivalMeter.cs`
- 创建：`Assets/GameData/Abilities/*.asset`（6 个定义）
- 创建：`Assets/GameData/Objectives/*.asset`（4 个目标定义）
- 创建：`Assets/Tests/EditMode/JoyScoreServiceTests.cs`
- 创建：`Assets/Tests/EditMode/CarnivalMeterTests.cs`

- [ ] **步骤 1：先写欢乐值和狂欢槽测试**

```csharp
[Test]
public void AssistAndEnvironmentActionsBothGrantJoy()
{
    var score = new JoyScoreService();
    score.Add(new ScoreEvent("p1", "assist", 10, UnityEngine.Vector3.zero));
    score.Add(new ScoreEvent("p1", "environment-chain", 15, UnityEngine.Vector3.zero));
    Assert.That(score.Get("p1"), Is.EqualTo(25));
}

[Test]
public void CarnivalMeterClampsAndEmitsOnce()
{
    var meter = new CarnivalMeter(100);
    var fired = 0;
    meter.OnFull += () => fired++;
    meter.Add(140);
    Assert.That(meter.Value, Is.EqualTo(100));
    Assert.That(fired, Is.EqualTo(1));
}
```

- [ ] **步骤 2：实现六个主动技能定义**

实现炮弹化、墙内藏身、位置交换、彩能反转、召唤宠物和超级喷射。技能通过 `AbilityDefinition` 配置冷却、能量和 `InteractionEffect`，不直接依赖具体角色类。

- [ ] **步骤 3：实现公共目标**

实现给雕像上色、追逐奖励气球、帮助中立生物和激活喷射装置四个目标；每个目标可完成、超时或被忽略，均不会锁死玩家移动或清空玩家资源。

- [ ] **步骤 4：提交**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform editmode -testResults "D:\project\caineng-playground\TestResults\task-6.xml" -quit
git add Assets/Scripts/Runtime/Objectives Assets/Scripts/Runtime/Combat/Ability* Assets/GameData/Abilities Assets/GameData/Objectives Assets/Tests/EditMode
git commit -m "feat: add abilities objectives and joy scoring"
```

## 任务 7：实现狂欢事件和地图运行时事件

**文件：**
- 创建：`Assets/Scripts/Runtime/Objectives/CarnivalEventDefinition.cs`
- 创建：`Assets/Scripts/Runtime/Objectives/CarnivalEventScheduler.cs`
- 创建：`Assets/Scripts/Runtime/Objectives/CarnivalEventExecutor.cs`
- 创建：`Assets/GameData/CarnivalEvents/*.asset`（6 个事件定义）
- 创建：`Assets/Tests/EditMode/CarnivalEventSchedulerTests.cs`

- [ ] **步骤 1：写事件调度失败测试**

```csharp
[Test]
public void SchedulerChoosesOnlyEnabledEvents()
{
    var scheduler = new CarnivalEventScheduler(seed: 7);
    scheduler.SetDefinitions(new[] { Enabled("low-gravity"), Disabled("scene-rotate") });
    var selected = scheduler.SelectNext();
    Assert.That(selected.Id, Is.EqualTo("low-gravity"));
}
```

- [ ] **步骤 2：实现事件定义和可撤销执行器**

每个事件必须实现 `Begin(context)`、`Tick(deltaTime)` 和 `End(context)`；执行器记录开始前的世界状态，结束时恢复，保证低重力、彩能狂喷、气球雨、巨型生物、场景旋转和烟花派对不会永久污染地图。

- [ ] **步骤 3：运行测试并提交**

```powershell
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform editmode -testResults "D:\project\caineng-playground\TestResults\task-7.xml" -quit
git add Assets/Scripts/Runtime/Objectives/CarnivalEvent* Assets/GameData/CarnivalEvents Assets/Tests/EditMode/CarnivalEventSchedulerTests.cs
git commit -m "feat: add reversible carnival events"
```

## 任务 8：搭建原型地图、预制体和 HUD

**文件：**
- 创建：`Assets/Scenes/Prototype.unity`
- 创建：`Assets/Prefabs/Player.prefab`、`Assets/Prefabs/Bot.prefab`
- 创建：`Assets/Prefabs/Projectiles/*.prefab`
- 创建：`Assets/Prefabs/WorldInteractables/*.prefab`
- 创建：`Assets/Scripts/Runtime/UI/PrototypeHudController.cs`
- 创建：`Assets/Scripts/Runtime/UI/ResultBadgePresenter.cs`
- 创建：`Assets/Art/Prototype/Materials/*.mat`
- 创建：`Assets/Art/Prototype/Particles/*.prefab`

- [ ] **步骤 1：搭建一张可读的垂直地图**

地图包含中央广场、两条墙面路线、地下短隧道、三组弹跳平台和一个公共目标区；每条路线放置清晰颜色、图案和音效提示，不能只用颜色区分状态。

- [ ] **步骤 2：建立玩家、Bot、武器和目标预制体**

所有预制体只通过接口引用规则服务；枪口、投射物、表面网格和 HUD 通过 Inspector 绑定数据资产，不在场景脚本中硬编码武器参数。

- [ ] **步骤 3：实现 HUD**

HUD 显示：当前公共目标、目标剩余时间、狂欢槽、个人欢乐值、当前表面移动状态、技能冷却和最近事件。击败、助攻和连锁反应采用短时浮字与音效，不阻塞视野。

- [ ] **步骤 4：提交场景垂直切片**

```powershell
git add Assets/Scenes Assets/Prefabs Assets/Scripts/Runtime/UI Assets/Art/Prototype
git commit -m "feat: assemble playable prototype scene"
```

## 任务 9：端到端 PlayMode 验收

**文件：**
- 创建：`Assets/Tests/PlayMode/PrototypeLoopPlayModeTests.cs`
- 创建：`TestResults/README.md`

- [ ] **步骤 1：写端到端验收测试**

```csharp
[UnityTest]
public IEnumerator PrototypeLoopHasObjectiveCarnivalAndRespawn()
{
    var session = PrototypeSession.StartForTest(playerCount: 6);
    yield return session.Ready;
    Assert.That(session.Players.Count, Is.EqualTo(6));
    Assert.That(session.Objective.IsActive, Is.True);
    session.Carnival.Add(1000);
    yield return null;
    Assert.That(session.EventScheduler.IsRunning, Is.True);
    session.Players[0].Bubbleize();
    yield return new WaitForSeconds(3.1f);
    Assert.That(session.Players[0].IsActive, Is.True);
}
```

- [ ] **步骤 2：运行完整 EditMode 和 PlayMode 套件**

```powershell
New-Item -ItemType Directory -Force -Path "D:\project\caineng-playground\TestResults" | Out-Null
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform editmode -testResults "D:\project\caineng-playground\TestResults\editmode.xml" -quit
& Unity.exe -batchmode -nographics -projectPath "D:\project\caineng-playground" -runTests -testPlatform playmode -testResults "D:\project\caineng-playground\TestResults\playmode.xml" -quit
```

预期：所有测试通过，日志中没有 `Error`、`Assert failed` 或未处理异常。

- [ ] **步骤 3：执行人工试玩检查**

逐项记录：30 秒内能否学会喷射移动；是否能不靠精准瞄准制造有效互动；6 人是否能在同一张地图保持可见；公共目标是否可忽略但仍有吸引力；狂欢事件是否在 1–2 秒内看懂；被击败后是否愿意立刻再次尝试；结算是否让非第一名玩家仍有成就感。

- [ ] **步骤 4：提交验收结果**

```powershell
git add Assets/Tests/PlayMode/PrototypeLoopPlayModeTests.cs TestResults/README.md
git commit -m "test: verify first playable loop"
```

## 自检结果

### 规格覆盖度

- 彩能表面、飞天遁地和三种基础材质：任务 3、8、9。
- 非致命 PvP、泡泡重生和六种工具枪械：任务 4、5、8、9。
- 六种主动技能：任务 6。
- 公共目标、欢乐值和狂欢槽：任务 6。
- 六种狂欢事件：任务 7。
- 低竞技、正向结算和清晰视觉反馈：任务 8、9。
- 可替换网络边界：任务 1 的接口边界；在线网络本身不纳入第一阶段垂直切片。

### 占位符扫描

计划不包含 `TODO`、`待定`、`TBD`、`FIXME` 或“添加适当处理”等未执行描述。

### 类型一致性

核心效果统一使用 `InteractionEffect`；表面统一使用 `SurfaceKind`；规则通知统一通过 `IRuleEventSink`；所有任务中使用相同的 `CarnivalMeter`、`JoyScoreService` 和 `CarnivalEventScheduler` 名称。
