# 异常街区 · caineng-playground

实验事故改变了人物、设备与街区。玩家用有完整伤害能力、也能改变战局的枪械，配合异能和趣味道具进行轻压力多人对战，再逐步扩展到其他玩法。

**统一入口：[项目总纲与 AI 执行交接](docs/MASTER_PLAN.md)。** 新 AI 先读根目录 [AGENTS.md](AGENTS.md)，再完整阅读总纲。

## 当前基线 v0.1

- 精致风格化 3D，参考发明家人物图；人物与场景有材质和体积。取消旧二维动漫、波普漫画方向。
- 第三人称，先 4 人设备对抗，6 分钟计分、快速复活。
- 两枪随身：矢量脉冲器与相变凝胶枪；空中冲刺、三件道具、研究站互动地图。
- 先验证双人联机与四人乐趣，再做完整美术、异常试跑、公网及六人测试。
- UE 负责游戏实现，Blender 负责主要资产加工，Hyper3D 可选生成初稿。

## 当前完成到哪里

已整合设计和开发文档。尚未创建 UE 工程，尚未完成新风格模型、生成流程或多人实测。具体以 [PROGRESS](docs/PROGRESS.md) 与实际仓库文件为准。

## 专项入口

| 想看什么 | 文档 |
|---|---|
| 所有要做的内容、优先级、验收 | [MASTER_PLAN](docs/MASTER_PLAN.md) |
| 实际进度和下一步 | [PROGRESS](docs/PROGRESS.md) |
| 简版阶段路线 | [ROADMAP](docs/ROADMAP.md) |
| 世界观、角色、画风 | [世界设计](docs/superpowers/specs/2026-09-21-anomaly-district-design.md) |
| 枪械攻击、功能、反制、计分 | [枪械专项](docs/superpowers/specs/2026-09-21-weapon-combat-function-design.md) |
| Blender、Hyper3D、UE 资产流程 | [资产规范](docs/assets/ASSET_PIPELINE.md) |
| UE 工程结构和阶段实现任务 | [UE 计划](docs/superpowers/plans/2026-09-21-anomaly-district-ue.md) |

总纲明确文档分工。数值是原型起点，需要实际试玩调整；文档完成不等于玩法或美术已实现。
