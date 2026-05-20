# QI RPG Agent Notes

本文件面向接手 `/home/joiry/Data/Projects/QI/rpg` 的代码 agent。目标是快速理解当前 RPG/Roguelike 分支的文件职责、修改边界和验证方式。

## Project Boundary

- 只修改 `rpg/` 目录内文件。
- 不要修改根目录版本的 `QI.c`、`QI.h`、`config.txt`，根目录会回归原版 QI 对战核心。
- RPG 版本当前重点是 Web UI 游玩体验；CLI 只要求保持可编译和基本兼容。
- 不要引入动画桥接系统，不要改旧 `--bridge` 协议。
- 不要引入 npm、Flask、FastAPI、SDL、raylib 或外部图片资源。Web UI 保持零前端依赖，Python 服务只用标准库。

## Build And Run

常规编译：

```sh
cd /home/joiry/Data/Projects/QI/rpg
gcc -Wall -Wextra -o qi_rpg QI.c -lm
```

Web UI：

```sh
cd /home/joiry/Data/Projects/QI/rpg
gcc -Wall -Wextra -o qi_rpg QI.c -lm
python3 web_server.py
```

浏览器打开：

```text
http://localhost:8000
```

每次 C 侧改动后至少运行：

```sh
cd /home/joiry/Data/Projects/QI/rpg
gcc -Wall -Wextra -o /tmp/qi_rpg QI.c -lm
CCACHE_DISABLE=1 gcc -DQI_LIBRARY -c QI.c -o /tmp/QI_rpg.o -lm
```

涉及 Web/Python 时追加：

```sh
python3 -m py_compile web_server.py
node --check ui/app.js
```

当前项目仍有一些历史 `-Wswitch` 和未用参数警告。除非任务要求清理警告，不要为了消警告扩大重构范围。

## File Map

- `QI.h`
  - 公共枚举、宏、结构体、全局变量声明和函数原型。
  - `Player`、`Skill`、`Artifact`、`Elixir`、`RunState` 等核心数据结构在这里。
  - 新字段要谨慎加，注意初始化、snapshot 输出、run reset、CLI 兼容。

- `QI.c`
  - 仍是单文件编译入口。
  - 保留主循环、战斗核心、AI 逻辑、旧 bridge、CLI 行为和 `.inc` 包含。
  - 尽量不要继续把 RPG 数据库、Web UI 流程、奖励/路线等大块逻辑塞回这里。
  - 原 QI 的行动类型对抗味道仍要保留，`learned_skills` 仍是旧战斗/AI 的兼容层。

- `rpg_data.inc`
  - 技能、法器、丹药数据库初始化。
  - 添加固定内容时优先改这里。
  - 自定义内容槽位由 `rpg_custom_content.inc` 生成，不要手写覆盖生成区。

- `rpg_rewards.inc`
  - 战后奖励池、奖励评分、奖励说明、奖励应用。
  - CLI 的交互式奖励仍在这里保持兼容。
  - Web UI 有非阻塞奖励路径，涉及替换流程时要同时检查 `rpg_ui_json.inc`。

- `rpg_build.inc`
  - 构筑画像、流派命名、协同文案、自动奖励评分。
  - `SchoolTagName()` 等多处共享的命名逻辑也在这里。

- `rpg_talents.inc`
  - 道基天赋数据库与天赋相关辅助函数。
  - 天赋首选百分比/轻量钩子，不要在这里堆复杂每回合战斗触发器。

- `rpg_encounters.inc`
  - CLI 旧式战间奇遇逻辑。
  - Web UI 当前主要使用路线节点替代自动奇遇。

- `rpg_ui_json.inc`
  - Web UI 的核心状态机和 JSON snapshot。
  - 管理 `--ui-json` 模式、非阻塞命令、奖励替换、突破、路线、天赋、鬼修、洗点、结构化战斗日志和 Boss 节奏。
  - 新增 Web 阶段时通常要改：阶段枚举、`UI_PhaseName()`、snapshot、命令处理、前端渲染。

- `rpg_custom_content.inc`
  - 由 `web_server.py` 根据 `custom_content.json` 生成。
  - 不要手工维护业务逻辑；需要改生成格式时改 `web_server.py`。

- `custom_content.json`
  - 本地内容工坊的数据源。
  - 允许为空内容包；保存后由 Python 校验并生成 `.inc`。

- `web_server.py`
  - 标准库本地 HTTP 服务。
  - 启动/重启 `./qi_rpg --ui-json` 子进程，过滤协议行，提供 `/api/*`。
  - 也负责内容工坊 JSON 校验、生成 `rpg_custom_content.inc`、重新编译 C 核心。

- `ui/index.html`
  - Web UI 静态结构。
  - 只放界面容器，不引入第三方库。

- `ui/app.js`
  - 前端状态拉取、API 调用、渲染逻辑、canvas 背景。
  - 避免频繁重建正在编辑的工坊表单，否则会导致输入焦点和 hover 状态闪烁。

- `ui/style.css`
  - Web UI 样式。
  - 需要兼顾桌面和移动端，避免卡片、日志、按钮文字互相挤压。

- `config.txt`
  - RPG 版本配置。
  - Web UI 通常强制使用 run 模式体验；批量/训练配置仍要避免被交互流程阻塞。

## Core Concepts

- `unlocked_skills`
  - RPG 层真正拥有的功法池。

- `equipped_skills`
  - 每个 `ActionType` 当前装备的功法。

- `learned_skills`
  - 旧战斗核心和旧 AI 使用的兼容层，由装备技能同步生成。
  - 修改技能系统时不要破坏这层，否则旧行动解析和 AI 会异常。

- `RunState`
  - 管理局内进度、奖励节奏、精英/Boss 节奏等。

- `cultivation`
  - Web run 长期修为资源。
  - 突破、洗点、遁逃、还阳等都可能消耗它。

- `soul_state`
  - 肉身/鬼魂状态。
  - 鬼魂状态二次死亡才真正 defeat。

- Web UI phases
  - 常见阶段包括：`battle_action`、`battle_result`、`reward_choice`、`reward_replace`、`breakthrough_choice`、`talent_choice`、`route_choice`、`between_battles`、`near_death_choice`、`defeat`。
  - 新阶段要保证刷新页面后 `/api/state` 能恢复当前 snapshot。

## Modification Rules

- 小步改动，保持 `gcc QI.c -lm` 单文件编译方式。
- 数据内容优先放到 `.inc` 模块，避免 `QI.c` 继续膨胀。
- Web UI 的交互流程必须非阻塞，不要在 `--ui-json` 模式调用 `scanf`、`getchar` 等等待输入的 CLI 函数。
- 普通 CLI 和 `--bridge` 不作为体验重点，但必须保持可编译，不要删除入口。
- 新奖励/技能/法器/丹药要考虑：
  - 数据库初始化。
  - 奖励池是否会抽到。
  - 已拥有时是否避免重复。
  - Web snapshot 是否能展示名称、描述、自定义标记。
  - 满槽替换流程是否安全。
- HP/QI 上限相关逻辑统一使用 `Player_MaxHP()` / `Player_MaxQI()`，修改后注意 clamp，避免局中回退到基础上限。
- 集气、丹药、法器、天赋等获得 QI 后都应 clamp 到当前最大 QI。
- 天赋和通用词条优先使用百分比，避免硬编码固定数值导致高境界收益失效。
- AI/Boss 调整要增强可读性，不要只堆难度。Web snapshot 的 `enemy.intent` 和 `combat_feed` 应给玩家反制线索。

## Web UI Protocol Notes

- C 子进程通过 stdout 输出普通日志和协议行。
- 协议行前缀为：

```text
##UI_JSON##:
```

- Python 桥接层只把协议行解析成 state；普通 stdout 作为日志流兜底。
- 常见 API：
  - `GET /api/state`
  - `POST /api/start`
  - `POST /api/action`
  - `POST /api/elixir`
  - `POST /api/reward`
  - `POST /api/replacement`
  - `POST /api/breakthrough`
  - `POST /api/skip_breakthrough`
  - `POST /api/talent`
  - `POST /api/route`
  - `POST /api/near_death`
  - `POST /api/skill/equip`
  - `POST /api/continue`
  - `POST /api/reset`
  - `GET /api/content`
  - `POST /api/content/save`
  - `POST /api/content/reset`

如果新增命令，要同步修改：

- `rpg_ui_json.inc` 命令解析。
- `web_server.py` API 转发。
- `ui/app.js` 调用。
- `ui/index.html` / `ui/style.css` 展示。

## Content Workshop

- 内容工坊不允许玩家写 C 代码或脚本。
- 所有效果必须走预设模板和数值范围校验。
- `custom_content.json` 是源文件，`rpg_custom_content.inc` 是生成文件。
- 保存失败时不要覆盖出一个破损 `.inc`。
- 保存成功后提示重置 run 生效；不要在战斗中热替换 C 数据库。

## Common Pitfalls

- 修改技能装备但忘记同步 `learned_skills`。
- 在 Web UI 路径调用 CLI 交互函数导致子进程阻塞。
- 奖励池给出玩家已经拥有的唯一法器或重复技能。
- 满法器/满丹药/同动作技能奖励没有进入 `reward_replace`。
- 增加 HP/QI 上限后，后续战斗流程又按基础表 clamp 回去。
- 工坊表单渲染时反复重建 DOM，导致输入框焦点、注意点或按钮 hover 闪烁。
- 新增 JSON 字段时没有做字符串转义。C 侧应使用 `UI_PrintEscaped()`。
- 用长日志塞进头像框主体，遮挡 HP/QI 条。战斗反馈应走 `combat_feed` 分侧展示。

## Suggested Next Steps

- 继续扩展 Boss 个性，但保持阶段提示和破绽窗口。
- 给路线节点加入更多具有代价的构筑事件。
- 扩展幽冥/还阳线，加入鬼修专属奖励和风险。
- 丰富内容工坊模板，同时保持数值校验和生成代码安全。
- 逐步把库存、丹药使用、路线和天赋逻辑从 `QI.c` 继续拆到 RPG `.inc` 模块。
