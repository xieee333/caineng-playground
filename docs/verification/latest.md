# Unity 可观测验证报告

> **历史报告（2026-09-19，Unity 6.6.2f1）**：此报告记录当时 Unity 工程的测试结果，不是当前活跃 UE 工程的最新验证。当前 UE 状态见 [`ue5-current.md`](ue5-current.md)。

验证日期：2026-09-19  
Unity：6.6.2f1（6000.6.2f1）  
验证工程：`D:\代码\github-repos\caineng-playground`

## 自动化结果

| 测试平台 | 结果 | 数量 |
|---|---:|---:|
| EditMode | Passed | 9/9 |
| PlayMode | Passed | 11/11 |
| 合计 | Passed | 20/20 |

PlayMode 覆盖玩家移动与相机相对方向、重生锁定/倒计时/回到出生点、三类彩能表面、喷涂命中、盛放弹射与表面恢复、灰盒场景/HUD，以及中继门顺序和通关条件。

新增反馈验证：

- 盛放命中时生成扩散圈并有音效占位；结束后命中圈清理、表面恢复。
- 彩能折返具备回收和归场音效占位；玩家归场时生成青色扩散圈，约定时长后自动清理。
- EditMode 验证程序化提示音生成单声道、22050 Hz、时长/采样数正确且样本非静音。
- 中继门必须按青液态→黄弹跳→洋红飘浮顺序喷涂；错序不推进，完成后门板滑开。
- 错序时 HUD 短暂显示红色“顺序不符 · 校准退回 n/3”；后续正确输入清除错误提示并可正常开门。
- 中继门后有实体校准横梁；未跳跃前进会被挡住，PlayMode 驱动真实 `PlayerMotor` 跳过横梁并进入出口触发区。
- 角色使用 `CharacterController` 实际穿过出口触发区后才完成试炼；门未开时出口不会误判完成。

## 可观测运行

- Main 场景可在 Unity Play Mode 运行；HUD 包含三色选择、准星、移动/表面/冷却状态、新手引导和中继门目标；出口完成时显示通关面板。
- 操作：WASD 移动、Space 跳跃、1/2/3 切换彩能、鼠标左键喷涂、Q 触发盛放；ESC 释放鼠标，离开平台会折返。
- 中继流程：对门依序喷青/黄/洋红，门滑开后穿过门洞并抵达出口标记。
- Unity Game View 1600×900 截图已目视确认中文 HUD、中心准星、门前通路与目标提示；另有自动化断言验证错序 HUD 文案。computer-use 本轮仍未提供 Windows 原生窗口用于人工键鼠操作，因此 30 秒上手尚未签收。
- 当前仍为灰盒和占位反馈；背景美术与精细枪械模型按既定计划后续制作。
- 当前运行截图：`Assets/Screenshots/gameplay-hud-current.png`。

## 结果文件

- 最新 PlayMode 结果：`TestResults/playmode-current.xml`（11/11）。
- 最新 EditMode 结果：`TestResults/editmode-current.xml`（9/9）；两者由 Unity 6.6.2f1 无界面测试运行通过，共 20/20。

## 本轮实现

- `BloomAbility`：范围弹射附近玩家、临时改写表面、花瓣/扩散圈与音效占位及清理。
- `RespawnController`：回收/归场生成式音效占位、落点扩散圈与清理。
- `PrototypeAudioCueFactory`：程序化扫频音效生成。
- `PrismRelayObjective` 与 `RelayExitTrigger`：三色顺序、错误重置、滑门动画、出口触发与一次性完成状态。
- `GrayboxArenaBootstrap`：修正门板位置、清理起步中路视线，并在出口设置青/洋红标记门架。
- `PrototypeHud`：保留现有 Canvas 结构，增加中文通关面板、中继进度和错序校准反馈。
- 本轮 EditMode 9/9、PlayMode 11/11 全部通过；PlayMode 覆盖错序提示、正确序列、横梁阻挡、真实跳跃越障和实际穿越出口。HUD 使用 Windows 系统动态字体，仅供本地原型，不是跨平台字体交付。
- Unity SceneView 偶发断言通过 `Tools/Caineng/Recover Scene View` 菜单恢复后再跑全量测试；它是编辑器视图恢复操作，不是玩法逻辑的一部分。
