# 彩能游乐场进度

> 这个文件是当前项目的单一进度入口。每完成一个阶段就更新，不用口头猜测状态。

## 当前状态

- 当前里程碑：**M2 第一把枪垂直切片（逻辑优先）**
- 当前步骤：**Unity 6.6.2f1 规则、玩家移动、泡泡重生与灰盒场景已通过自动化验证**
- 总体状态：**进行中**
- 最近基线：**v12 六枪波普漫画 2D 概念板**
- 下一步：接入棱花喷绘器的喷射、命中与表面着色循环

## 阶段清单

- [x] M0：核心 PvP 设计规格
- [x] M0：垂直切片实现计划
- [x] M0：v1–v11 概念图迭代
- [x] M1：武器视觉与玩法规范（初稿）
- [x] M1：六把枪 v12 概念板（初稿，待审阅）
- [x] M2：Unity 工程骨架与核心规则
- [ ] M2：棱花喷绘器第一人称可玩原型
- [ ] M3：彩能表面与非致命互动
- [ ] M4：六把枪全部接入
- [ ] M5：6 人混战地图、Bot、目标和狂欢事件
- [ ] M6：角色、枪械和场景视觉统一
- [ ] M7：网络同步与构建发布

## 已完成资产

| 资产 | 路径 | 状态 |
|---|---|---|
| 核心设计规格 | `docs/superpowers/specs/2026-09-18-caineng-playground-design.md` | 已完成 |
| 垂直切片计划 | `docs/superpowers/plans/2026-09-18-caineng-playground-vertical-slice.md` | 已完成 |
| 当前概念图 | `docs/concepts/caineng-playground-key-art-v11-pop-comic.png` | 已完成 |
| 六枪 v12 概念板 | `docs/concepts/caineng-playground-key-art-v12-weapons.png` | 初稿，待审阅 |
| Unity 工程、核心规则和灰盒入口 | `ProjectSettings/`、`Packages/`、`Assets/Scripts/Runtime/`、`Assets/Scenes/Main.unity` | Unity 6.6 验证通过 |
| 玩家输入动作资产 | `Assets/Input/PlayerInput.inputactions` | 已创建，7 个动作 |
| 武器方向规格 | `docs/superpowers/specs/2026-09-18-caineng-playground-weapon-direction-design.md` | 本次新增 |
| 项目总路线 | `docs/ROADMAP.md` | 本次新增 |

## 变更记录

### 2026-09-18

- [x] 固定 v11 波普漫画 2D 视觉基线。
- [x] 为首发六把枪定义夸张外观、基础玩法和特殊玩法。
- [x] 定义泡泡、折射、磁吸、传导、弹簧、镜像六类词条。
- [x] 定义 2D 概念、低模占位、Unity 预制体和验收的资产流水线。
- [x] 建立里程碑、提交和进度更新规则。
- [x] 生成六把枪 v12 波普漫画概念板，确认六种主形状可区分。
- [x] 创建 Unity 项目骨架、程序集边界、彩能表面网格和三种移动规则。

### 2026-09-19

- [x] 升级并锁定 Unity 6.6.2f1、Input System 1.20、URP 17.6 与 Test Framework 1.8。
- [x] 修复无效的 `com.unity.modules.ugui` 依赖并完成项目首次导入。
- [x] 实现输入路由、玩家移动状态机、3 秒泡泡重生和跌落自动回场。
- [x] 将彩色平台、斜坡和边界墙接入 `Main` 灰盒场景。
- [x] EditMode 8/8 与 PlayMode 4/4 测试通过。

## 验证记录

- Markdown 文件：使用 Git diff 和路径检查验证。
- v12 概念板：已保存到仓库并检查文件存在，待用户进行视觉审阅。
- JSON 配置：`manifest.json` 与三个 `.asmdef` 已通过解析检查。
- Input System：`PlayerInput.inputactions` 已通过解析检查，包含 7 个动作和 11 个绑定。
- Unity 编译与 EditMode 测试：Unity 6.6.2f1 编译通过，8/8 通过。
- Unity PlayMode：玩家移动、泡泡锁定/重生、3 秒默认规则与 `Main` 灰盒场景加载，4/4 通过。
- 网络多人：尚未开始，因此暂不报告网络同步通过。
