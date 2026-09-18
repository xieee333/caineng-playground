# Unity 可观测验证报告

验证日期：2026-09-19  
Unity：6.6.2f1（6000.6.2f1）  
验证工程：`D:\代码\caineng-playground-verify`  
主工程：`D:\代码\github-repos\caineng-playground`

## 自动化结果

| 测试平台 | 结果 | 数量 |
|---|---:|---:|
| EditMode | Passed | 8/8 |
| PlayMode | Passed | 9/9 |

PlayMode 用例覆盖：

- 玩家输入移动。
- 彩能折返锁定、倒计时和返回出生点。
- 默认折返时间为 3 秒。
- Main 灰盒场景、诊断 HUD、玩家输入和喷涂组件均存在。
- 玩家自动识别液体表面。
- 喷涂射线命中表面、更新表面类型、写入网格并生成可见标记。
- 反弹表面实际发射玩家。
- 飘浮表面使用较轻的贴地重力。
- Q 键盛放脉冲临时改变附近表面，并在持续时间后恢复。
- Main 场景生成第一人称棱花喷绘器占位模型，并能观察喷涂后坐反馈。

## 可观测验收

- [x] 运行画面显示 `LIVE DIAGNOSTICS`。
- [x] HUD 显示玩家状态、脚下表面、速度。
- [x] HUD 显示彩能折返状态和剩余时间。
- [x] HUD 显示喷涂次数和最近命中点。
- [x] 折返视觉为青/洋红/黄色分层残像，不再创建球形泡泡。
- [x] 测试确认 `Color Rewind Visual` 在折返时生成。
- [x] 测试确认 `Bloom Pulse` 在技能期间生成并在结束后销毁。
- [x] 测试确认 `Prism Sprayer View` 在 Main 场景中生成。

## 原始结果文件

- EditMode：`D:\代码\caineng-playground-verify\TestResults\editmode-observable.xml`
- PlayMode：`D:\代码\caineng-playground-verify\TestResults\playmode-weapon.xml`

## 本轮修改

- `RespawnController`：通用彩能折返控制器。
- `PlayerMotor`：将 Bubble 状态改为 Recalling 状态。
- `PrototypeHud`：加入运行时诊断信息。
- `BloomAbility`：加入 Q 键范围脉冲和临时表面改变。
- `PrismSprayerView`：加入第一人称棱花喷绘器占位和后坐反馈。
- `PlayerTraversalPlayModeTests`：加入可观测 HUD 和分层残像验收。
