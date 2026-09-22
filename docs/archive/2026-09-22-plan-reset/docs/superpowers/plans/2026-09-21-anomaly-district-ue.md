# 异常街区 UE 修订阶段计划

> **2026-09-22执行覆盖**：本文v0.1阶段保留作工程对照，不再独立决定下一任务。当前按 [TODO最新队列](../../TODO.md) 和 [v0.2整合方案](../specs/2026-09-22-anomaly-playground-revision.md) 实施第一人称、独立资源/封存模式、样本车、藏组件和局内构筑。核心战斗可复用；正式UI另确认，表现小样前置。Computer Use按关联批次集中验收，规则和事务优先自动化；不因文档修改重跑UE。

> 2026-09-21，v0.1 当前执行计划。当前实现分支已有 UE 5.8.2 旧原型工程，但本计划对应的第三人称战斗内容尚未开始验收；复用与停用边界见 [`../../MIGRATION_TO_ANOMALY_DISTRICT.md`](../../MIGRATION_TO_ANOMALY_DISTRICT.md)。
> 总体范围与共用规则见 [MASTER_PLAN](../../MASTER_PLAN.md)，实际状态以 [PROGRESS](../../PROGRESS.md) 为准；本文勾选任务需有真实验证证据。

**目标：** 验证两把能改变战局的枪，再完成风格化 3D 对战切片和一个可切换的试跑原型。

**枪械规则来源：** [战斗与功能完整规格](../specs/2026-09-21-weapon-combat-function-design.md)。两枪各自能量/冷却、直接火力、功能代价和有效击退助攻均按该文实现；后两把枪仍属扩展。

**资产流程：** 按 [Blender + Hyper3D + UE 规范](../../assets/ASSET_PIPELINE.md) 执行。规范文档已存在，P0 需验证并补入实际版本与结果，不要重新覆盖成另一套流程。

**架构：** 复用角色、武器、道具、场景交互；按模式单独管理目标、伤害和结算。服务器统一判定状态，客户端表现结果。

**技术方向：** UE5、蓝图优先、必要的 C++、CharacterMovement、Enhanced Input、Niagara、UMG。P0 锁定实际可用版本。官方资料链接放在本文末尾，不再要求接手者从失效计划寻找执行依据。

## 文件与职责

建议工程根目录保持 `unreal/CainengPlayground/`，不因工作名变化重命名仓库。

| 计划路径（工程内） | 责任 |
|---|---|
| `Content/Caineng/Core/BP_CPGGameModeBase.uasset` | 出生、离开、开始/结束的共用流程 |
| `Content/Caineng/Core/BP_CPGGameState.uasset` | 模式、阶段、服务器时间 |
| `Content/Caineng/Core/BP_CPGPlayerState.uasset` | 身份、分数、检查点进度 |
| `Content/Caineng/Modes/BP_CombatMode.uasset` | 伤害、击败、助攻、战斗排名 |
| `Content/Caineng/Modes/BP_TrialMode.uasset` | 顺序检查点、圈数、试跑排名 |
| `Content/Caineng/Characters/BP_CPGCharacter.uasset` | 基础移动、镜头、冲刺入口 |
| `Content/Caineng/Characters/BP_CombatComponent.uasset` | 生命与受击、控制减弱 |
| `Content/Caineng/Characters/BP_EquipmentComponent.uasset` | 两枪切换、保留各枪资源和攻击间隔 |
| `Content/Caineng/Core/BP_ContributionTracker.uasset` | 按受害者生命 ID 记录伤害与有效击退贡献，服务器去重计分 |
| `Content/Caineng/Weapons/BP_VectorWeapon.uasset` | 脉冲射击与受限推动 |
| `Content/Caineng/Weapons/BP_GelWeapon.uasset` | 凝胶攻击与普通合法静态地面液池部署（用户后续确认修订） |
| `Content/Caineng/World/BPI_AnomalyInteractable.uasset` | 有类型的交互入口，不对所有物件自由施力 |
| `Content/Caineng/World/` | 弹性地块、移动屏障、转动桥板 |
| `Content/Caineng/Items/` | 脉冲球、反应罐、折叠屏障 |
| `Content/Caineng/Data/` | 武器、道具、模式参数 |
| `Content/Caineng/Maps/L_NetworkLab.umap` | 两人同步验证 |
| `Content/Caineng/Maps/L_ResearchStation.umap` | 共用空间及模式专属对象 |
| `Content/Caineng/Art/` | 风格化 3D 角色、设备与环境 |
| `docs/testing/`（仓库内） | 按阶段保存真实测试记录 |

模式不把“能否扣血”散写进每个枪械。服务器根据当前模式决定玩家伤害、推力限制和计分；物件推动继续由统一交互层处理。回合切换清理临时物件、重置机关与统计，载入模式专属检查点。客户端不能自行切换规则。

## P0 环境

- [ ] 核实 UE 与编译环境，固定版本并写 `docs/UE_ENVIRONMENT.md`。
- [ ] 第三人称模板在 `unreal/CainengPlayground/` 能启动、打包并从干净检出打开。
- [ ] 配置 Git LFS 及生成目录忽略规则，记录跨电脑资产同步操作。
- [ ] 执行已有 `docs/assets/ASSET_PIPELINE.md`，核实参考图与工具环境，补入实际骨骼、握持点、来源和许可记录。
验收：模板实际打包成功，干净检出可打开。参考图或生成服务缺失不阻塞能独立开展的灰盒实现。

## A0 早期美术支线（P0–P4 间，P5A 前通过）

- [ ] 核实并登记新参考图永久副本，完成原创发明家的完整服装、正侧背设计。
- [ ] 制作风格化 3D 同屏小样：发明家占位、设备、走廊与材质球，确认金属、布料、塑料的区别。
- [ ] 按资产流程完成一件实验仪器的生成或直接建模、Blender 加工与 UE 查看，记录总工时和实际费用。

验收：风格样件可见体积与材质，比例不幼儿化；一件资产流程可复现。完整主角不在本阶段强制制作。记录 `docs/testing/A0-art-pipeline.md`。

## P1 最小双人对局

- [ ] 建立角色、GameMode、GameState、PlayerState 和 CombatComponent 的职责。
- [ ] 两个独立进程双向测试射击、生命、一次性击败计分、3 秒重生和出生保护。
- [ ] 实现基本输入、第三人称镜头避障和 HUD；复活资源重置、出生保护提前结束条件按总纲第 5 节。
- [ ] 6 分钟结束、锁分并原房间重开，检查旧物件和计时器已清理。
- [ ] 约 100ms 往返延迟和 1% 丢包下记录输入、命中和结果一致性。

验收：客户端不能独自决定伤害或分数，没有重复结算或复活失败。保存 `docs/testing/P1-network-loop.md`。

## P2 两把枪证明战局变化

- [ ] 先按枪械专项参数实现两把基础攻击，在无机关靶场验证伤害、射速、能量和独立击败能力。
- [ ] 接入两枪切换，验证不能通过切换刷新能量、跳过攻击间隔或清除特殊冷却。

- [ ] 实现矢量宽幅攻击、遮挡检查、有限特殊脉冲、能量与冷却。
- [ ] 实现凝胶弹与两块地块上限，部署前检查目标类型、距离、视线；失败不扣资源。
- [ ] 实现弹性地块的 8 秒恢复和可破坏节点；实现一个可移动屏障。
- [ ] 实现一次空中冲刺，单独测服务器推动与客户端移动修正，未通过就保留普通移动定位问题。
- [ ] 按总纲第 7 节实现独立角色能量、空中次数和冷却，不与枪械能量混用。
- [ ] 双人反复测试“推走掩体后进攻”“搭弹床换高点”“破坏节点阻断路线”。
- [ ] 记录弹体宽度、射程、击败用时、特殊功能使用情况。若特殊用法始终劣于直接射击，调整能量与地图，不靠增加枪械掩盖问题。

验收：两人能解释为什么此刻选特殊功能，防守者能指出一种应对。保存 `docs/testing/P2-battle-changing-weapons.md`。

## P3 道具和场景连锁

- [ ] 实现脉冲球、延时反应罐、折叠屏障，使用服务器倒计时与唯一归属。
- [ ] 道具伤害/范围、速度、屏障生命、桥板行为和配额统一读取总纲第 7 节初值，接入可配置数据。
- [ ] 测试炸弹回扔不延时、两人抢拾只有一人成功、持有者断线后落地继续倒计时。
- [ ] 测试脉冲打偏炸弹只改轨迹不改归属，屏障阻挡对应爆炸判定。
- [ ] 加入转动桥板和全场道具数量限制，严格限制运动范围；背景物件只做装饰物理。
- [ ] 连控和重复击退测试，确保浮空能操作，地图不存在无限弹射牢笼。

验收：每种强效果有提示、结束条件和反制；模拟延迟下重复 20 次关键互动无重复引爆。保存 `docs/testing/P3-anomaly-interactions.md`。

## P4 四人设备对抗

- [ ] 实现枪械专项第 10 节的伤害与有效击退助攻，分别测试窗口外、位移不足、碰撞阻挡、自主移动、多股外力和复活去重案例。

- [ ] 搭建研究站灰盒，保证高点两条接近路线和边缘安全出生点。
- [ ] 接入局域网创建/加入/满房/离开/房主退出提示；房主退出则结束房间，不实现主机迁移。
- [ ] 接通结算与原房间重开，测试先后加入、到时锁分和环境击败归属。
- [ ] 4 位真实玩家连续三局，至少 2 位射击新手，记录主动改变战局的实例、被控时间和再玩意愿。

验收：玩法清楚、双方可反制、第三局仍有自主尝试。未通过就修改 P2/P3。保存 `docs/testing/P4-combat-playtest.md`。

## P5A 3D 成品切片

- [ ] 确认 A0 与 P4 验收通过，按资产流程制作一个完整发明家角色、两把枪、三件道具和模块化环境。
- [ ] 接入握枪、跑跳、冲刺、受击动画及对应音效/VFX，验证轮廓、材质、镜头遮挡和碰撞。
- [ ] 按总纲实现缓降与独立角色能量，单独验证，不和模式规则一次性加入。

验收：实机符合参考方向，模型动作与功能一致。记录 `docs/testing/P5A-art-slice.md`。

## P5B 异常试跑与基础社交

- [ ] 复用研究站场景布置检查点，建立 TrialMode；两圈完赛后其他人继续，到全员完赛或 4 分钟截止，排序按总纲第 8 节。
- [ ] 玩家伤害关闭、推力受限、掉落返回最近检查点；验证混战分数和机关状态不会泄漏到试跑。
- [ ] 房主在回合间切换模式，所有客户端看见下局规则；测试连续切换 10 次。
- [ ] 加最小危险标记和表情选择，限制重复发送频率，不做语音、公会和自动精彩视频。

验收：同套枪械与能力在非战斗目标中仍有用途，连续切换不泄漏伤害、计分和物件状态。保存 `docs/testing/P5B-trial-mode.md`。

## P6 公网、六人和后续内容选择

- [ ] 在锁定的 UE 版本下接入一个会话方案，两处网络真实验证邀请、加入、断线与错误提示。
- [ ] 扩到 6 人，验证出生密度、物件数量和可读性；效果变差时保留 4 人上限。
- [ ] 在记录配置的电脑测 1080p/60fps 目标和网络表现，注明实测未达项。
- [ ] 交付带提交号的 Windows 测试包、操作说明和已知问题。
- [ ] 根据测试决定只推进一个扩展：第二角色、极性枪、折射枪或联合抢修。每个新增内容单独验证。

验收：不能用本地成功代替公网通过，也不能用两人结果声称六人稳定。记录在 `docs/testing/P6-delivery.md`。

## 风险优先级

先验证网络物理，再扩展异常数量；先验证两把枪确实改变选择，再做武器目录；先让一个模式成立，再验证切换。完整建模和大批资产排在玩法可行之后，但风格小样应尽早做，避免最后才发现视觉方向错了。

以上为阶段设计，不包含已经生成的 UE 二进制资产或完整实现代码。实际进度统一写入 `../../PROGRESS.md`。

## 官方技术参考

执行时切换到 P0 实际锁定的引擎版本；网页默认版本不代表本项目版本。

- [联网总览](https://dev.epicgames.com/documentation/unreal-engine/networking-overview-for-unreal-engine)
- [角色网络移动](https://dev.epicgames.com/documentation/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine)
- [联网物理](https://dev.epicgames.com/documentation/unreal-engine/networked-physics-overview)
- [Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine)
