# UE5 当前验证记录

## 最新：本地核心争夺与回归（2026-09-20 19:39）

- 搬运模式：1 玩家 + 5 Bot，G 拿取/抛出、己方投递、敌方命中掉球、越界回收、3 分或超时结束、回车重开；独立灰盒入口不覆盖旧模式。
- 16/16 规则通过，扩展探针通过，Computer Use 实按 G 与回车验证；正常 Bot 局自行以 3:2 结束。不是真人联机或乐趣验收。
- 最终构建十二阶段移动/上墙/泡泡/Bloom 可见回归 PASS，源码副本一致。详细限制、修复和日志见 [`搬运切片验证`](2026-09-20-carry-slice.md)。

## 最新增量：液态上墙与棱花盛放（2026-09-20 18:36）

- 可见 standalone 联合回归 `WallBloom-visible.log`：十二阶段全部通过；Computer Use 观察了实际游戏。普通面/液态速度仍为 600/960，潜行 1200，低顶板保护与相机下移通过。
- 墙面四阶段：从地面接入液态墙，松键下落、液态被清除后下落、空格蹬墙、墙上受击回场全部通过；阶段最大升高约 852/850/712/590 cm。只允许接近竖直的液态墙，不把地板/天花板当墙。未实现跨墙角、翻越墙顶或完整多人移动预测。
- 新增独立 `WallTraversalComponent`，按住 Ctrl + W 接入，W/S 上下、A/D 沿墙横移，空格朝外蹬跳；受击前取消贴墙模式，防止泡泡回场恢复成无重力悬空。当前控制与碰撞是单机原型，非最终液态化动画。
- 棱花喷绘器新增 E“盛放”。真实武器特殊命令生成临时弹射花：`spawned=1 cooldown=1 bubbleGuard=1 expired=1 playerVz=942.2 propVz=932.6`。实测弹起玩家和 10 kg 方块，冷却拒绝重复触发且不重置，泡泡期间不能触发，8 秒效果到期。当前视觉为橙色圆盘占位，可通过数据定义的 Actor Class 换外观；其他五把武器特殊技尚未接入。
- `WallBloom-Rules.log`：13/13 Success，`GIsCriticalError=0`。墙面与盛放两次增量构建均 11/11 成功。日志路径均在 `D:\UEProjects\CainengPlayground 5.8\Saved\Logs`。
- 复现仍用 `-CanergyTraversalProbe`，现十二阶段约 53 秒；日志新增 `CANERGY_WALL`、`CANERGY_BLOOM`。开发驱动不是物理 Ctrl/W/E 长按验收，不应据此声称真人手感、完整比赛或联机已完成。

## 最新增量：Ctrl 地面潜行与低通道（2026-09-20 18:18）

- UE5.8.2 可见 standalone，Computer Use 已观察最终“开发移动实测通过”画面。开发演示使用真实角色、移动组件、碰撞与摄像机，不是预录动画；不是编辑器 PIE，也不代表物理键盘长按验收。
- 新增左 Ctrl 按住潜行，玩家与测试驱动共用 `SetLiquidDiveHeld`。仅落地接触液态表面时启用；离地、松键、液态消失或泡泡状态会退出。基础速度参数 600，液态 960，潜行实际峰值 1200 cm/s；潜行不叠加冲刺倍数。
- `LiquidDive-Ceiling-visible.log`：`entered=1 released=1 reentered=1 surfaceExit=1`，站立胶囊半高 96 cm；低通道 `ceilingSafe=1 ceilingExit=1`。有顶板时松键只取消潜行加速，不强行站起；移走顶板后恢复站立碰撞。
- 模板第一人称相机附着于头部，单独缩胶囊不足以降低视点。新增可关闭的原型第一人称姿态适配，实测 `cameraDrop=56.0` cm；它不是最终潜行动画，后续接入角色视觉档案/正式动画时需替换或关闭。
- 全套运行回归 `CANERGY_TRAVERSAL PASS`：普通/液态阶段位移 1414.5/2180.1 cm，弹跳速度 839.8 cm/s，离面飘浮与重力恢复均通过；泡泡 3.21 秒回场、欢乐值 7→7。
- 最终增量构建 8/8 成功；`LiquidDive-FinalRules.log` 的 `Caineng.Gameplay` 12/12 Success（9 项新核心、3 项旧中继），`GIsCriticalError=0`。本次六个源文件与测试副本 SHA-256 相同。
- 仍未验收：潜行中受击/泡泡的整套视觉、头顶复杂动态障碍、真实键盘持续输入与纯鼠标视角、墙面潜行、多人同步/预测。当前潜行是单机地面低姿态高速原型，不是完整液态变形或隐身。
- 重现仍用下方 `-CanergyTraversalProbe` 命令；现有七阶段演示约 28 秒，新增日志关键词 `CANERGY_DIVE`。结束清理临时地面/顶板并恢复原位置与控制；正常游戏不生成这些测试物体。

## 前一增量

## 最新增量：移动与泡泡运行验证（2026-09-20 18:06）

- 运行环境：UE 5.8.2，`D:\UEProjects\CainengPlayground 5.8` 测试副本，`Lvl_Shooter` 可见 standalone 游戏窗口。不是编辑器 PIE；本轮通过 Computer Use 观察真实画面。
- 构建：玩法修正增量 9/9 actions 成功；测试驱动最后改为实际受击路径后增量 4/4 成功。引擎弃用和工具链版本提示仍存在。
- 规则：`TraversalBubble-Verified.log` 中 `Caineng.Gameplay` 11/11 Success，`GIsCriticalError=0`；包含新增落地触发、连续接触不重复触发、离面飘浮、过期与泡泡清除规则。
- 真实运行：`TraversalBubble-visible.log` 记录 `CANERGY_TRAVERSAL PASS` 与 `CANERGY_BUBBLE PASS`。普通面最大速度/实际末速度 600 cm/s，液态面 960 cm/s；对应位移 1409.7/2184.5 cm；弹跳向上速度约 838 cm/s；离面高度超过 100 cm 时低重力仍生效，之后恢复基础重力。
- 泡泡：用真实角色 `TakeDamage(100000)` 触发；角色未销毁，原连发中断，重新开火被阻止，主动技能被拒绝且冷却未消耗；3.22 秒回场，欢乐值 7→7，移动模式与胶囊碰撞恢复。
- 首轮演示使用世界 X 位移、未隔离人工输入，液态加速比较未通过。改为二维距离并在演示期间暂停人工输入后，实际记录到 600/960 的速度差。失败日志保留为 `TraversalProbe-visible.log`，独立移动复测为 `TraversalProbe-visible-2.log`。
- 喷绘输入与颜色循环在此前可见运行已观察到：左键命中改变表面，R 切换液态/弹跳/飘浮。最新代码保留该路径；本次演示直接配置测试表面，不把它算成再次验证射击铺路。
- 本轮九个修改/新增源文件与测试副本 SHA-256 一致。演示结束销毁临时测试平台、恢复原位置和人工输入；正常启动不运行演示，Shipping 不启用其启动分支。

### 重现

在 PowerShell 启动可见开发演示（约 19 秒，自动结束并回到场景）：

```powershell
& 'D:\代码\caineng-playground\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\UEProjects\CainengPlayground 5.8\CainengPlayground.uproject' -game -windowed -ResX=1280 -ResY=720 -CanergyTraversalProbe
```

日志位于测试副本 `Saved/Logs`；搜索 `CANERGY_TRAVERSAL` 和 `CANERGY_BUBBLE`。演示通过正式 `DoMove`/跳跃、表面组件、伤害与回场接口运行真实物理，测试时暂时隔离用户输入以保证可重复性。

### 尚未验收

真实键盘持续 WASD、无按键鼠标观察、完整地图路线、液态潜行和墙面移动、真实多人输入/状态复制、六把武器全部特殊玩法、五个 Bot 与完整对局、正式角色/UI。当前回场只有运行状态和原型提示，泡泡视觉表现尚未完成。此前 GameOnly 输入模式代码已构建，但不把自动演示当作纯鼠标操作验收。

## 历史基线（下文为此前记录，当前状态以本页最新增量为准）

验证日期：2026-09-20  
引擎：Unreal Engine 5.8  目标关卡：`Lvl_Shooter`  
仓库工程：`Unreal/CainengPlayground/CainengPlayground.uproject`

## 结果摘要

| 检查项 | 结果 | 证据边界 |
|---|---|---|
| C++ Editor 构建 | 通过（测试副本） | 本轮 Clean 全量 33/33 actions、其后增量 4/4 成功；新玩法规则改动增量 4/4 成功 |
| UE 编辑器启动与 PIE | 部分通过（旧玩法历史） | `Lvl_Shooter` 曾在 PIE 可见运行；用户实际点亮 3/3 中继并看到闸门开启。新玩法尚未接入 PIE；为了构建已关闭空的项目启动器 |
| 自动化玩法规则 | 通过 | `Caineng.Gameplay.Core` 新规则 6/6 Success；连同旧中继历史规则共 `Caineng.Gameplay` 9/9 Success |
| 运行错误检查 | 未发现游戏致命错误 | 自动化进程 `GIsCriticalError=0`、退出码 0；启动阶段有 UE 自带 UnifiedErrorTest 的 `LogAutomationTest: Error: Condition failed` 输出，需与游戏测试结果区分 |
| HUD 与操作反馈 | 部分通过 | 常驻目标/操作面板、R/F、F 脉冲命中反馈与不可喷绘提示均在本轮 PIE 观察 |
| 鼠标视角 | 代码链路通过，纯鼠标移动待现场复验 | `MouseLookAction → LookInput → AddControllerYawInput`，默认鼠标捕获开启；游戏不要求右键。Computer Use 没有无按键的鼠标移动动作，先前右键/拖动只是临时测试手段，不是最终控制方案 |
| 移动与有效喷绘命中 | 未签收 | 单次 W 短按没有可靠位移证据；实际射击只观察到非目标表面的提示，进度仍为 0/3 |
| 三色顺序、错误反馈和开门 | 部分人工验证 + 自动化通过 | 用户实际点击三个球无误；UE 画面显示“中继 3/3 · 闸门已开”；错序/重复/非法目标由规则测试覆盖 |
| 空中越梁与绿色出口通关 | 规则通过，PIE 完整路线待验 | 自动化验证只能证明门槛逻辑；尚未在 PIE 实际跳过横梁并抵达绿色出口 |
| 退出 PIE 后重新开始 | 待验 | 本轮为编译释放 DLL 停止了 PIE，尚未重新进入并复测状态重置 |

## 新玩法整体重写验证（2026-09-20）

- 按已确认玩法重新规划：6 人、6–8 分钟第一人称欢乐混战；喷涂移动表面、非致命玩具互动、欢乐值、可选公共目标、狂欢槽/全场事件、快速泡泡回场。中继试炼被降为旧教程/诊断路线，不再是主循环。
- 新增反射可用的彩能表面/互动效果/得分/比赛状态类型；新规则支持效果按类型共存、同类按策略叠加、时长结束清除、约 3 秒泡泡门槛、欢乐行为计分、比赛阶段切换、可选目标完成/超时、狂欢阈值和事件时长。
- 新增 `WeaponDefinition` 与独立武器外观定义；角色玩法定义与独立视觉档案；技能、公共目标、狂欢事件与比赛定义资产。定义字段可由 UE 编辑器序列化；此次尚未创建资产实例或接线到角色/武器运行时。
- 新增的自动化覆盖：`BubbleRespawn`、`CarnivalMeter`、`InteractionEffects`、`JoyScore`、`MatchLoop`、`ObjectiveTimeout`，六项全通过；旧中继三项也继续通过。全套 UE 自动化共 9/9 Success。
- 本轮测试使用 `D:\UEProjects\CainengPlayground 5.8` ASCII 路径副本。Clean 全量构建 33/33 actions，反射类型与规则编译成功；后续增量构建 4/4 actions。仓库与测试副本对应六个本轮源文件 SHA-256 一致。
- 这些是规则/构建验证，不是游戏内玩法验证。没有启动新玩法 PIE，也未证明玩家能在场景中实际喷涂、移动或触发事件；UI/人物美术仍按用户要求后置。

## 本轮 UE 观察（2026-09-20）

- UE 编辑器标题为 `CainengPlayground`。这次会话中曾观察到 `Lvl_Shooter` PIE；用户操作三个中继球后，画面显示“中继 3/3 · 闸门已开”。随后为重新编译释放模块而停止 PIE。
- 当前 UE 项目启动器可见；Computer Use 重开操作返回 `foreground window did not report a process id`，因此本轮没有重新进入 PIE。旧的 3/3 画面证据仍有效，但当前并非运行中的游戏窗口。
- 在 PIE 中观察到 R 切换提示；F 脉冲提示明确反馈“击退 1 名角色、点亮 0 块弹跳面”；对非目标表面射击时出现引导提示，目标进度保持 0/3。短暂反馈约 2.5 秒后恢复主线文本。
- Mouse Look 代码绑定到模板鼠标输入并通过 `AddControllerYawInput` 旋转控制器，默认设置会捕获鼠标；游戏没有右键门槛。Computer Use 无法只移动鼠标而不按键，故不以右键/拖动结果作为最终鼠标控制验收。空格跳跃曾在 PIE 观察到视角高度变化；持续移动仍未签收。
- HUD 是可读性占位，不是最终 UI；模板生命条/比分仍在，场景几何仍为粗灰盒。

## 实现检查

- `ShooterCharacter` 绑定 R 切换喷绘模式、F 脉冲、Shift 冲刺和左键喷绘；射线命中会交给 GameMode 判断中继颜色与顺序。基础移动沿用 First Person 模板 Enhanced Input。
- `ShooterGameMode` 按青、橙、洋红处理目标；颜色不匹配提示目标颜色，顺序错误会重置进度并恢复目标。
- 三个目标完成后隐藏中继门；玩家必须从校准梁前侧起跳，在空中向前越过，并位于横向有效范围内，出口距离判定才会生效。
- 将中继序列、越梁条件、出口通关门槛抽为无世界依赖的规则函数，新增 `Caineng.Gameplay.RelaySequence`、`CalibrationHurdle`、`TrialExit` 三个 UE 自动化测试；三项均成功。出口测试覆盖未完成中继、未越梁、半径内/边界/半径外。
- 上述条目是代码审阅结论，不等于人工实机完整流程已验证。
- 新增 `RelayObjectiveWidget` 常驻中文主线面板，角色和 GameMode 的临时反馈统一写入该面板，避免依赖调试屏幕文字。

## 构建与日志

- 构建目标：UE 5.8.2、`TP_FirstPersonEditor Win64 Development`。测试副本 Clean 全量 33/33、最新增量 4/4 actions 成功；仍有引擎 `GetMovementBase` 弃用提示和 MSVC 14.51 高于建议 14.50 的工具链提示。
- 六个本轮新玩法规则/资产定义文件在仓库与测试副本逐字一致，自动化运行使用该测试副本。仓库原中文路径直接构建的 SharedPCH C1083 尚未解决；不能把测试副本成功误记为仓库路径构建成功。
- 最新全套玩法规则自动化退出码 0，`Caineng.Gameplay` 9/9 Success、`GIsCriticalError=0`。UE 启动时出现的 UnifiedErrorTest 自测 `Condition failed` 行来自引擎自测阶段；本项目测试随后全部 Success。
- 日志检查只用于排查本轮启动/自动化过程，未发现游戏 `Fatal` 或 `Unhandled Exception`；这不代表所有普通 Warning 均已清理。
- 先前日志出现过模板资源引用缺失提示：`SM_DoorFrame_Edge` 和 `MI_Intro_Colorway`；尚未确认是否影响 `Lvl_Shooter`，仍列为待清理项。

## 下一轮验证

1. 建立运行时交互效果/欢乐值组件和通用交互解析器，让以上纯规则由玩家、武器与 Bot 共用。
2. 构建通用武器控制器，先接棱花喷绘器和三种移动表面，并用新玩法角色进入可观察 PIE。
3. PIE 观察鼠标自由转视角（无需右键）、持续 WASD、喷涂命中、弹跳/漂浮、效果解除与约 3 秒安全回场；按实测修正再重复测试。
4. 然后接入其他玩具武器、技能、可选目标和狂欢事件，构成本地玩家 + 5 Bot 的完整一局。

旧灰盒的鼠标/中继/出口复验仍可用作历史教学关卡检查，但不阻塞新玩法主线，也不能替代新玩法的 PIE 实测。
