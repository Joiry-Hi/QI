# QI RPG Agent Notes

本文件面向接手 `/home/joiry/Data/Projects/QI/rpg` 的代码 agent。目标是快速理解当前 RPG/Roguelike 分支的文件职责、修改边界和验证方式。

## Project Boundary

- 只修改 `rpg/` 目录内文件。
- 不要修改根目录版本的 `QI.c`、`QI.h`、`config.txt`，根目录会回归原版 QI 对战核心。
- RPG 版本当前重点是 Web UI 游玩体验；CLI 只要求保持可编译和基本兼容。
- 不要引入动画桥接系统，不要改旧 `--bridge` 协议。
- 不要引入 npm、Flask、FastAPI、SDL、raylib 或外部图片资源。Web UI 保持零前端依赖，Python 服务只用标准库。

## Current Completeness Assessment

截至当前版本，QI RPG 已经从“战斗核心的 RPG 外壳”推进到一个基本完整的 Web roguelike 修仙 run：

- 核心闭环：路线三选一 -> 战斗 -> 战后奖励 -> 心魔/突破/天赋 -> 整备 -> 再选路已经成立。
- 成长系统：功法获取/替换、道基天赋、技能熟练度、流派循环、小境界理解、功法洗练、法器升级、炼丹、丹方解锁都已接入 run 内成长。
- Roguelike 压力：寿元、年龄、路线耗时、劫气、雷劫事件、濒死抉择、鬼修/还阳和排行榜共同形成长期风险。
- 构筑反馈：流派协同、法器共鸣、奖励倾向、路线主题和 Boss 原型能给玩家明确方向。
- 叙事骨架：道途年表、年龄/寿元、突破/渡劫/鬼修/飞升/死亡记录已经能支撑一局完整修仙故事。
- 内容工坊：本地模板化技能/法器/丹药编辑器已可用于扩内容，但仍应保持数值校验和生成代码安全。

当前完整度可粗略评估为：

- 系统完整度：约 80%-85%。主要玩法循环已经可完整通关并有多系统联动。
- 内容量完整度：约 45%-55%。路线、Boss、技能、法器、丹药已有骨架，但长期重复游玩仍需要更多事件、敌人、词条和叙事文本。
- 数值平衡完整度：约 55%-65%。大方向可玩，高风险高收益和前后期压力已经调整过，但新增系统多，仍需要大量实测微调。
- UI 完整度：约 70%-75%。Web UI 已能承载主要系统，侧栏/详情浮层/年表/整备面板可用，但信息密度仍偏高，后续新增系统要优先考虑折叠和分层。
- 工程结构完整度：约 60%-70%。`.inc` 拆分已经减轻 `QI.c` 压力，但 `rpg_ui_json.inc` 仍承担过多状态机、路线、事件、年表和 Boss 逻辑，后续继续扩展时建议逐步拆分。

最适合继续投入的方向：

- 扩内容，而不是继续堆新大系统。优先补路线事件、Boss 个性、敌人模板、年表文本、功法/法器/丹药内容。
- 做数据化拆分。路线模板、特殊事件、Boss 原型、年表文本池可逐步从 `rpg_ui_json.inc` 拆到专门 `.inc`。
- 做数值回归表。将路线风险、耗时、收益、雷劫概率、突破成本、法器升级成本整理成小表，便于长期调参。
- 增强终局和失败后的回顾。飞升榜、道途遗卷和完整年表是本项目很有味道的部分，值得继续强化。

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

- `rpg_growth.inc`
  - 成长和百分比收益相关 helper。
  - 处理 HP/QI 上限、突破率、战后修为、天赋/法器等通用成长加成时应优先检查这里。

- `rpg_talents.inc`
  - 道基天赋数据库与天赋相关辅助函数。
  - 天赋首选百分比/轻量钩子，不要在这里堆复杂每回合战斗触发器。

- `rpg_encounters.inc`
  - CLI 旧式战间奇遇逻辑。
  - Web UI 当前主要使用路线节点替代自动奇遇。

- `rpg_ui_json.inc`
  - Web UI 的核心状态机和 JSON snapshot。
  - 管理 `--ui-json` 模式、非阻塞命令、奖励替换、心魔/突破、天赋、整备、路线、雷劫事件、鬼修、洗点、年表、结构化战斗日志和 Boss 节奏。
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

- `minor_understanding` / `minor_realm_level`
  - 小境界理解与小境界层数。
  - 由战后、低风险路线、部分事件获得；突破成功后重置当前境界的小境界进度。

- `soul_state`
  - 肉身/鬼魂状态。
  - 鬼魂状态二次死亡才真正 defeat。

- `spirit_materials` / `herb_materials`
  - 灵材用于法器升级，药材用于炼丹。
  - 主要在 Web UI 的 `preparation` 阶段消费。

- `skill_mastery` / `school_cycle`
  - Web run 中玩家施放技能会积累熟练度，并推进流派小循环。
  - 熟练度和循环效果在 `rpg_mastery.inc`，snapshot 输出给行动按钮和功法树。

- `boss_phase` / `boss_archetype`
  - Boss 战保留 `蓄势 -> 压迫 -> 破绽` 三相，但当前已经按路线主题派生不同 Boss 原型。
  - 现有原型包括：剑冢守灵、血池魔胎、雷劫化身、宗门执法、无相心魔、飞升残影。
  - Boss 设计目标是“可读、可反制、有记忆点”，不要通过无限抬境界制造难度。

- `calamity_debt` / `tribulation_pressure`
  - 劫气是路线风险债：提高奖励稀有度和雷劫事件概率。
  - 雷劫压力是长期天命压力：不再直接抬敌人境界，而是在战间触发 `tribulation_event`。

- `chronicle`
  - Web run 年表，当前容量为 `UI_CHRONICLE_CAPACITY` 64 条。
  - 记录入道、路线、岁月、突破、天赋、雷劫、鬼修、还阳、飞升/死亡等关键节点。

- Web UI phases
  - 常见阶段包括：`battle_action`、`battle_result`、`reward_choice`、`reward_replace`、`heart_demon_choice`、`breakthrough_choice`、`talent_choice`、`preparation`、`route_choice`、`tribulation_event`、`between_battles`、`near_death_choice`、`victory`、`defeat`。
  - 新阶段要保证刷新页面后 `/api/state` 能恢复当前 snapshot。

## Web Run Systems

### Life, Time, And Chronicle

- 年龄、寿元、路线耗时和年表都在 `rpg_ui_json.inc`。
- 常用调参入口：
  - `UI_LifespanForRealm()`：每个境界寿元。
  - `UI_RealmTimeBase()`：每个境界的基础时间尺度。
  - `UI_RouteBaseYears()`：不同路线的主要耗时。
  - `UI_RouteYearVariance()`：不同路线的耗时随机浮动。
  - `UI_AdvanceYearsForRoute()`：实际推进年龄、雷劫压力并写年表。
- 当前设计倾向：
  - 凡人开局年龄为 12-38 岁，凡人寿元 70。
  - 炼气寿元较保守，低境界战间耗时较短。
  - 宗门试炼约 2-3 年，内门大比约 10 年起步。
  - 清修/凡尘耗时较长但稳；妖兽/魔修/天劫/星陨耗时短但劫气和风险更高。
- 前端详情浮层默认显示最近年表；完整年表在 `ui/index.html` 的 `chronicle-modal` 和 `ui/app.js` 的 `renderChronicle()`。

### Route Chains And Special Routes

- 路线模板在 `UI_ROUTE_TEMPLATES`，鬼魂路线在 `UI_GHOST_ROUTE_TEMPLATES`。
- 特殊路线模板在 `UI_SPECIAL_ROUTE_*`。
- 路线连续性状态在 `g_ui_*_chain`：
  - `g_ui_sword_tomb_chain` -> `剑修试炼`
  - `g_ui_beast_den_chain` -> `血煞兽窟`
  - `g_ui_ghost_market_chain` -> `阴市法会`
  - `g_ui_herb_valley_chain` -> `丹炉开劫`
  - `g_ui_demon_ruins_chain` -> `魔心旧契`
  - `g_ui_sect_trial_chain` -> `内门大比`
  - `g_ui_mortal_world_chain` -> `故人归尘`
- 改路线时通常要检查：
  - `UI_GenerateRouteOptions()` 是否能抽到或注入。
  - `UI_ChooseRoute()` 是否正确更新链条、劫气、年龄和雷劫。
  - `UI_RouteYieldFor()` 是否同步材料预览与实际结算。
  - `UI_RouteRewardPreviewText()` / `UI_RouteStoryHint()` / `UI_RouteChainPreview()` 是否给前端明确文案。
  - `UI_RouteChronicleText()` 是否写入年表叙事。

### Tribulation Event

- 雷劫是战斗外事件，不应通过敌人境界膨胀体现。
- 触发入口在 `UI_MaybeTriggerTribulation()`，处理入口在 `UI_ResolveTribulation()`。
- 低境界主要表现为业力/小雷劫惩罚，不应频繁直接身死道销；后期，尤其炼虚以后，才更强调审判性质。
- 三个选择：
  - 正面渡劫：成功清空劫气和压力，奖励更高；失败 `tribulation_death`。
  - 护身避劫：消耗修为/灵材，安全性更高，削减部分劫气压力。
  - 强行压劫：短期避开死亡判定，但增加劫气/雷劫压力并损伤状态。
- Boss 预告不应再提示“劫气提高敌人境界”；劫气只影响奖励和雷劫概率。

### Preparation, Equipment, And Alchemy

- `preparation` 是 Web UI 局后整备阶段。
- 法器升级逻辑在 `rpg_equipment.inc`；Web 命令处理在 `rpg_ui_json.inc`。
- 炼丹配方由 `Alchemy_GetRecipe()` 等 helper 管理，药材不足时前端按钮应禁用。
- 替换法器时对应槽位等级要清零。

### Skill Mastery And Refine

- 熟练度和流派循环在 `rpg_mastery.inc`。
- 功法洗练字段在 `Player.skill_refines[]`，成本和效果在技能相关 helper 中。
- 行动按钮、功法树、奖励卡需要展示熟练度和洗练摘要，避免玩家看不见成长反馈。

### AI And Route Bosses

- 普通/精英 AI 在 `QI.c` 的 `AIThreatProfile` 和 `EvaluateAction()` 里读取玩家构筑、HP/QI、负面状态、丹药/法器数量和当前资源。
- Web UI 普通敌人应优先走更可读的 V2 评分逻辑，保留少量随机扰动，避免完全机械。
- Boss 原型在 `rpg_ui_json.inc`：
  - `UI_BossArchetypeForRoute()` / `UI_PredictBossArchetype()` 决定 Boss 主题。
  - `UI_ChooseBossAction()` 根据三相和原型选择行动偏好。
  - Snapshot 输出 `boss_phase`、`boss_archetype`、`next_battle.boss_name` 和 `next_battle.boss_hint`。
- 调 Boss 时要同时检查：
  - 意图文案是否给出反制线索。
  - 三相是否真的有破绽窗口。
  - 胜利奖励是否和 Boss 主题一致。
  - 是否意外把路线/雷劫压力重新变成敌人境界膨胀。

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
  - `POST /api/artifact/upgrade`
  - `POST /api/elixir/brew`
  - `POST /api/preparation/skip`
  - `POST /api/route`
  - `POST /api/tribulation`
  - `POST /api/near_death`
  - `POST /api/skill/equip`
  - `POST /api/skill/refine`
  - `POST /api/continue`
  - `POST /api/reset`
  - `GET /api/records`
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
- 调整雷劫时又把敌人境界抬高，导致后期频繁遇到离谱高境界敌人。
- 改路线耗时只改 `UI_RouteYearAdvance()`，忘记同步 `UI_RouteYearPreview()`，导致路线卡预览和实际结算不一致。
- 改材料奖励只改战后结算，忘记同步 `UI_RouteYieldFor()`，导致路线卡材料预览失真。
- 新增路线链条后忘记在 `UI_StartRun()` 重置计数，导致重开 run 继承旧事件链。
- 年表条目过长或过于机械，会削弱“修仙人生”的叙事感。优先写短句，有年龄和境界上下文即可。

## Suggested Next Steps

- 内容优先级最高：扩展路线事件、敌人模板、Boss 文案、年表文本、技能/法器/丹药数据库。
- Boss 可以继续扩个性，但必须保留阶段提示和破绽窗口，避免变成纯数值压制。
- 路线节点可以加入更多“有代价的构筑事件”，例如牺牲寿元换功法、用灵材修复古宝、压劫换高阶奖励。
- 扩展幽冥/还阳线，加入鬼修专属奖励、阴司事件和魂火风险。
- 丰富飞升/失败后的回顾：完整道途、关键战役、最高境界、主流派、渡劫记录。
- 把路线事件链、Boss 原型、年表文本继续拆分为更数据化的表，降低 `rpg_ui_json.inc` 膨胀。
- 丰富内容工坊模板，同时保持数值校验和生成代码安全。
- 建立一个轻量数值调参表，集中管理路线耗时、风险、材料、修为、雷劫概率和奖励稀有度。
