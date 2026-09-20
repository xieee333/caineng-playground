# 核心搬运争夺：本地切片验证

## 范围与入口

- UE 5.8.2，测试项目 `D:\UEProjects\CainengPlayground 5.8\CainengPlayground.uproject`；源码保存在仓库 `Unreal/CainengPlayground/Source`，部署测试时复制源码，不覆盖用户其他资产。
- 默认旧模式不变。通过 `AgentScripts/Run-CarryPrototype.ps1` 开启新的可见独立运行入口；`-Practice` 是机器人静止的按键检查入口，`-Probe` 是自动运行探针，不得同时使用。
- UE 参数：`-CanergyCarryPrototype`；可选 `-CanergyCarryPractice` 或 `-CanergyCarryProbe`。
- 主切片仍是 **1 玩家 + 5 Bot、本地 3v3 灰盒**；另已完成两个 UE 进程的最小联机拿取/抛出验证，但不是完整真人联机对局。为不破坏模板地图，测试场地运行时生成于旧场地上方，未保存成正式地图资产。

## 当前内容

- 两队投递圆台、中央金色核心、中央通道与两侧掩体，队伍 A/B 占位标记。
- G 近距离且无遮挡拿取；携带时 G 朝观察方向抛出；敌方玩具枪命中或传统伤害可使核心掉落；友方玩具枪命中不强制掉球。
- 携带者进入己方圆台计 1 分；不能隔空投球直接得分。先得 3 分或 180 秒结束，回车重新加载一局。
- 唯一持有人、释放短锁定、得分后复位、出界/长期闲置回收、持球位置墙体扫掠、超时停止拿取/计分。
- 泡泡回场后 2 秒内不能抢核心或被强制掉球；持球角色失效时核心复位。
- 紫色裂光薄墙支持 X 潜相：只有明确标记、厚度合规且出口无阻挡时才能穿越；短暂隐匿会因开火、受击或到期而结束。
- Bot 现分为搬运、拦截、支援三个意图，会使用中央、北侧高路和南侧潜相捷径；仍只是规则运行伙伴，不是成熟寻路、合作战术或乐趣证明。

## 证据（日志目录为测试项目 Saved/Logs）

- `FunPlan-BaselineRules.log`：原有新增称号规则在内共 14/14 Success。
- `FunPlan-QuickMatch-visible.log`：20 秒开发用快速局，6 人各自称号结算；通过 Computer Use 在结算页实按回车，记录 `CANERGY_RESULTS restart requested`，画面回到开局。
- `Carry-Rules.log` 与 `Carry-FinalRules.log`：16/16 Success，包括搬运归属、非法请求、抛出锁定、拦截、一次计分、3 分结束、超时、重置等规则。
- `Carry-FirstProbe-visible.log`：首轮运行探针 PASS（拿取、抛出、出界回收、投递）；保留初版证据，不作为全部修复后最终结果。
- `Carry-ExtendedProbe-visible.log`：扩展探针 PASS，覆盖距离拒绝、拿取、抛出、回收、投递、敌方/友方命中接口、传统伤害掉球、超时后拿取拒绝。命中项由开发驱动直接调用接口，不冒充人工瞄准射击验收。
- `Carry-Manual-visible.log`：通过 Computer Use 实按 G 拿取（11:35:16 UTC）和再次 G 抛出（11:35:26 UTC）。观察到持球状态提示与右下方球体，中央视野无球体遮挡；再次按键球体离手，日志一致。
- 搬运结算页实按回车后观察到重新准备场地，比分回归初始化；不是只靠源码推断。
- `Carry-BotsRound-visible.log`：未使用 Probe/Practice，1 玩家旁观 + 5 Bot 正常驱动，双方自行拿取、受击掉球与投递，约 37 秒后以青队 3:2 达到胜利条件；Computer Use 实际观察结算页。短局长与简单追球也提示 Bot 战术/地图节奏仍需改进，不把此结果算作耐玩性通过。
- `Carry-TraversalRegression-visible.log`：最终构建的十二阶段可见回归 PASS，普通/液态速度 600/960，潜行峰值 1200，低顶板安全与相机下移 56 cm，墙面释放/失去表面/蹬跳/泡泡取消及回场通过；泡泡回场 3.12 秒，Bloom 玩家/物件上冲速度 942.2/932.0，冷却、泡泡保护与到期通过。Computer Use 观察了运行窗口，测试动作由开发探针驱动。
- `Carry-SafetyProbe-visible.log`：扩展安全探针 PASS，额外覆盖实际泡泡回场与 2 秒争夺保护。
- `Carry-NetServer-RPC.log` / `Carry-NetClient-RPC.log`：监听服务器接入第二客户端，场内识别 2 名人类；客户端两次走与 G 键相同的 RPC，服务器依次记录 `take player=1` / `drop player=1`，客户端复制快照同步显示橙队持球再恢复无人持球。
- `Phase-Rules.log` 与 `Gameplay-All-Phase.log`：潜相规则单测及全套 `Caineng.Gameplay` 18/18 Success；`Phase-Runtime.log`：真实角色穿过 60 cm 标记墙，1.5 秒后结束隐匿并保留 6.5 秒冷却。
- `Phase-NetServer-3.log` / `Phase-NetClient-3.log`：第二客户端发送归一化瞄准方向，服务器收到 RPC 后独立验证并穿过 60 cm 标记墙；客户端复制状态依次为 `phased=1`、`phased=0`。此前一次因服务器沿用过期朝向被正确拒绝，修正为“客户端提供意图、服务器负责全部空间校验”后复测通过。
- `Carry-LargeMapProbe-visible.log`：灰盒由 36×26 m 扩为 54×40 m 后完整搬运安全探针通过；当前是中间尺寸，约 72×52 m 只是待节奏验证的工作目标。
- `Carry-FairBotMatch-visible.log`：开发专用全 Bot 3v3 公平自运行中，北侧和南侧路线均被角色意图使用，两次均以青队 3:2 结束，约 51–57 秒；证明路线和规则能运行，但局长明显短于最终 6–8 分钟目标，不作为耐玩性结论。
- `Phase-CarryLock-Clean.log`：真实携带核心时服务器拒绝潜相且核心仍归原持有者；出墙后 0.4 秒攻击前摇已接入。
- `Carry-ObjectiveToolProbe-visible.log`：首轮因核心未显式归类为物理物体而 FAIL；修正碰撞对象类型并重新构建后，Bloom 以 900 cm/s 弹射松散核心，0.6 秒后核心高度 310.5 cm；实际牵引风筒随后以宽锥判定把核心拉回 69.9 cm，组合探针 PASS。物件速度封顶 900 cm/s，携带时因关闭物理模拟而不能被隔空抢走。该证据证明“场景能力 + 玩具枪直接改变核心”的首条链路，不代表三工具和全部组合彩蛋已完成。
- `Gameplay-All-SocialObjective.log`：上述核心物理分类、牵引和速度封顶接入后，全套 `Caineng.Gameplay` 18/18 Success，`GIsCriticalError=0`。
- 最终增量构建成功，仓库与测试副本 Source 哈希核对差异为 0；启动脚本通过 PowerShell 语法解析。上述日志未发现 Fatal/Assertion/失败测试记录，不代表消除了所有引擎警告。

## 修复记录

- 初次 UBA 加速编译停滞，终止该次构建进程后使用 `-NoUBA` 成功，未改系统安全设置。
- 增加武器交互命中接口掉球，避免只支持旧 TakeDamage 而玩具枪无法拦截。
- 持球球体缩小并移到侧下方，增加携带位置碰撞扫掠；目标加占位识别线，机器人加队伍标记。
- 修正 HUD 短消息期间仍展示过期目标状态的问题。
- 队伍标签数组遍历曾触发 C3535/C2440，改成 TObjectPtr 引用遍历后构建成功；失败未记作通过。

## 未完成与验收边界

- 潜相已是权威联机功能原型，但尚无正式隐形材质/VFX、音效、角色专属数据和真人双端视觉体验验收。正式角色、枪体模型、UI 美术未定稿。
- 未完成完整真人联机对局、完整角色体系、三工具组合及真人重复试玩，不能据此声称好玩或可上架。
- 本轮探针不等于完整墙角/持球贴墙压力测试；跨墙角、翻顶、联机预测、回场保护及更成熟 Bot 导航仍需推进。
- 搬运输入已有服务器 RPC、服务端距离/资格校验和目标快照复制；移动预测、泡泡/潜相表现、结算、断线重连及完整玩法仍需双端推进。
