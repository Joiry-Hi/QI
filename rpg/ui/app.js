const state = {
  snapshot: null,
  busy: false,
  skillTreeOpen: false,
  workshopOpen: false,
  detailsOpen: false,
  chronicleOpen: false,
  workshopTab: "skills",
  content: { skills: [], artifacts: [], elixirs: [] },
  workshopDirty: false,
  workshopLoaded: false,
  records: { ascensions: [], memorials: [] },
  recordsLoading: false,
  recordsLastLoad: 0,
};

const phaseNames = {
  run_start: "启程",
  battle_action: "选择行动",
  battle_result: "回合结算",
  reward_choice: "选择奖励",
  reward_replace: "替换确认",
  heart_demon_choice: "心魔关",
  breakthrough_choice: "闭关突破",
  talent_choice: "道基天赋",
  preparation: "局后整备",
  route_choice: "选择路线",
  tribulation_event: "雷劫",
  near_death_choice: "濒死抉择",
  encounter: "奇遇",
  between_battles: "战斗间隙",
  victory: "飞升",
  defeat: "败北",
};

const $ = (id) => document.getElementById(id);

function esc(value) {
  return String(text(value)).replace(/[&<>"']/g, (ch) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;",
  }[ch]));
}

function pct(value, max) {
  if (!max || max <= 0) return 0;
  return Math.max(0, Math.min(100, Math.round((value / max) * 100)));
}

function text(value, fallback = "") {
  return value === undefined || value === null || value === "" ? fallback : value;
}

async function api(path, body = {}) {
  if (state.busy) return;
  state.busy = true;
  try {
    const res = await fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    });
    render(await res.json());
  } finally {
    state.busy = false;
  }
}

async function refresh() {
  const res = await fetch("/api/state");
  render(await res.json(), { preserveWorkshop: true });
}

async function loadContent() {
  const res = await fetch("/api/content");
  const payload = await res.json();
  state.content = payload.content || { skills: [], artifacts: [], elixirs: [] };
  state.workshopLoaded = true;
  state.workshopDirty = false;
  renderWorkshop(state.snapshot || {});
}

async function loadRecords(force = false) {
  if (state.recordsLoading) return;
  const now = Date.now();
  if (!force && now - state.recordsLastLoad < 5000) return;
  state.recordsLoading = true;
  try {
    const res = await fetch("/api/records");
    state.records = await res.json();
    state.recordsLastLoad = Date.now();
    renderRecords();
  } catch (_) {
    // 排行榜是辅助信息，读取失败不影响当前 run。
  } finally {
    state.recordsLoading = false;
  }
}

async function saveContent() {
  if (state.busy) return;
  state.busy = true;
  $("workshop-state").textContent = "编译中";
  try {
    const res = await fetch("/api/content/save", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ content: state.content }),
    });
    const payload = await res.json();
    if (payload.error) {
      $("workshop-state").textContent = payload.error;
    } else {
      state.content = payload.content || state.content;
      $("workshop-state").textContent = "已编译，重置后生效";
      state.workshopDirty = false;
      if (payload.state) render(payload.state, { preserveWorkshop: true });
    }
  } finally {
    state.busy = false;
    renderWorkshop(state.snapshot || {});
  }
}

async function resetContent() {
  if (state.busy) return;
  state.busy = true;
  try {
    const res = await fetch("/api/content/reset", { method: "POST", headers: { "Content-Type": "application/json" }, body: "{}" });
    const payload = await res.json();
    state.content = payload.content || { skills: [], artifacts: [], elixirs: [] };
    state.workshopDirty = false;
    state.workshopLoaded = true;
    $("workshop-state").textContent = "已清空并编译";
    if (payload.state) render(payload.state, { preserveWorkshop: true });
  } finally {
    state.busy = false;
    renderWorkshop(state.snapshot || {});
  }
}

function renderFighter(prefix, fighter) {
  if (!fighter) return;
  $(`${prefix}-name`).textContent = text(fighter.name, prefix === "player" ? "玩家" : "敌人");
  $(`${prefix}-realm`).textContent = `${text(fighter.realm, "凡人")} · ${text(fighter.root, "灵根")}`;
  $(`${prefix}-hp-text`).textContent = `${fighter.hp ?? 0} / ${fighter.max_hp ?? 0}`;
  $(`${prefix}-qi-text`).textContent = `${fighter.qi ?? 0} / ${fighter.max_qi ?? 0}`;
  $(`${prefix}-hp-bar`).style.width = `${pct(fighter.hp, fighter.max_hp)}%`;
  $(`${prefix}-qi-bar`).style.width = `${pct(fighter.qi, fighter.max_qi)}%`;
  const flags = [];
  if (fighter.bleeding) flags.push(`流血 ${fighter.bleeding}`);
  if (fighter.cursed) flags.push(`诅咒 ${fighter.cursed}`);
  if (fighter.enraged) flags.push(`狂怒 ${fighter.enraged}`);
  if (fighter.soul_state === "ghost") flags.push("鬼魂");
  $(`${prefix}-flags`).textContent = flags.length ? flags.join(" · ") : "状态平稳";
}

function feedItems(snapshot, side, limit = 3) {
  return (snapshot.combat_feed || []).filter((item) => item.side === side).slice(-limit);
}

function renderCombatFeed(snapshot) {
  const renderList = (id, items) => {
    const box = $(id);
    box.innerHTML = items.length
      ? items.map((item) => `<div class="feed-item ${esc(item.kind || "info")}">${esc(item.text || "")}</div>`).join("")
      : `<div class="feed-item muted">暂无动静</div>`;
  };
  renderList("player-feed", feedItems(snapshot, "player"));
  renderList("enemy-feed", feedItems(snapshot, "enemy"));
  renderList("system-feed", feedItems(snapshot, "system", 2));
  const intent = snapshot.enemy?.intent || {};
  const boss = snapshot.boss_phase || {};
  $("battle-intent").innerHTML = `
    <strong>${esc(intent.label || "敌意未明")}</strong>
    <span>${esc(intent.hint || "敌人仍在试探。")}</span>
    ${boss.key && boss.key !== "none" ? `<em>${esc(boss.name || "Boss")} · ${esc(boss.label || "")}</em>` : ""}
    ${boss.counter_hint && boss.key !== "none" ? `<span>${esc(boss.counter_hint)}</span>` : ""}
  `;
}

function renderActions(snapshot) {
  const wrap = $("actions");
  wrap.innerHTML = "";
  const enabled = snapshot.phase === "battle_action";
  (snapshot.actions || []).forEach((action) => {
    const btn = document.createElement("button");
    btn.className = "action-btn";
    btn.disabled = !enabled || !action.available;
    btn.innerHTML = `
      <div class="action-title"><span>${esc(action.name || "未知功法")}</span><span>${action.cost} 气</span></div>
      <div class="action-meta">${esc(action.action || "行动")} · ${esc(action.school || "无流派")} · ${action.rank || 0} 阶</div>
      <div class="action-meta">熟练 Lv${action.mastery_level || 0} · ${action.mastery_xp || 0}/${action.mastery_next || 3}</div>
      ${action.effect_summary ? `<div class="action-meta">${esc(action.effect_summary)}</div>` : ""}
    `;
    btn.addEventListener("click", () => api("/api/action", { slot: action.slot }));
    wrap.appendChild(btn);
  });
  if (!wrap.children.length) {
    wrap.innerHTML = `<div class="action-meta">暂无可用行动</div>`;
  }
}

function rewardLabel(reward) {
  if (!reward) return "";
  const typeNames = { skill: "技能", artifact: "法器", elixir: "丹药", cultivation: "修炼" };
  return `${typeNames[reward.type] || reward.type} · ${reward.rarity}`;
}

function renderRewards(snapshot) {
  const panel = $("reward-panel");
  const wrap = $("rewards");
  const visible = snapshot.phase === "reward_choice";
  panel.classList.toggle("hidden", !visible);
  wrap.innerHTML = "";
  if (!visible) return;
  (snapshot.rewards || []).forEach((reward) => {
    const card = document.createElement("button");
    card.className = "reward-card";
    const detail = reward.type === "skill"
      ? `${text(reward.action)} · ${text(reward.school)} · ${reward.rank || 0} 阶 · ${reward.cost || 0} 气`
      : reward.type === "cultivation"
        ? `修为 +${reward.amount || 0}`
        : rewardLabel(reward);
    card.innerHTML = `
      <div class="reward-title"><span>${esc(reward.name || "修炼资源")}</span><span>${esc(rewardLabel(reward))}</span></div>
      <div class="reward-meta">${esc(detail)}</div>
      <div class="fit-row">
        <span class="fit-badge">${esc(reward.fit_label || "构筑选择")}</span>
        <span class="fit-badge">评分 ${reward.score ?? 0}</span>
        ${reward.will_replace ? `<span class="fit-badge">需取舍</span>` : ""}
      </div>
      <div class="reward-desc">${esc(reward.desc || "收入囊中，继续前行。")}</div>
      ${reward.effect_summary ? `<div class="reward-desc">${esc(reward.effect_summary)}</div>` : ""}
      <div class="reward-desc">${esc(reward.fit_reason || "")}</div>
    `;
    card.addEventListener("click", () => api("/api/reward", { index: reward.index }));
    wrap.appendChild(card);
  });
}

function renderBreakthrough(snapshot) {
  const panel = $("breakthrough-panel");
  const data = snapshot.breakthrough || {};
  const visible = snapshot.phase === "breakthrough_choice";
  panel.classList.toggle("hidden", !visible);
  if (!visible) return;
  $("breakthrough-chance").textContent = `${data.chance || 0}%`;
  $("breakthrough-body").innerHTML = `
    <strong>${esc(data.current_realm || "当前境界")} -> ${esc(data.next_realm || "下一境界")}</strong>
    <div>消耗修为 ${data.cost || 0}，当前 ${data.cultivation || 0}。失败会返还部分修为并损失元神，但不会致死。</div>
  `;
  $("attempt-breakthrough-btn").disabled = !data.available;
}

function renderHeartDemon(snapshot) {
  const panel = $("heart-demon-panel");
  const wrap = $("heart-demon-options");
  const data = snapshot.heart_demon || {};
  const visible = snapshot.phase === "heart_demon_choice" && data.active;
  panel.classList.toggle("hidden", !visible);
  wrap.innerHTML = "";
  if (!visible) return;
  (data.options || []).forEach((option) => {
    const card = document.createElement("button");
    card.className = `heart-demon-card ${option.mode || ""}`;
    card.disabled = !option.available;
    card.innerHTML = `
      <div class="reward-title"><span>${esc(option.title || "心魔抉择")}</span><span>${option.chance || 0}%</span></div>
      <div class="fit-row">
        <span class="fit-badge">修为 ${option.cost || 0}</span>
        <span class="fit-badge">${esc(option.mode || "")}</span>
      </div>
      <div class="reward-desc">${esc(option.desc || "")}</div>
    `;
    card.addEventListener("click", () => api("/api/heart_demon", { mode: option.index }));
    wrap.appendChild(card);
  });
}

function renderTalentChoice(snapshot) {
  const panel = $("talent-panel");
  const wrap = $("talents-choice");
  const visible = snapshot.phase === "talent_choice";
  panel.classList.toggle("hidden", !visible);
  wrap.innerHTML = "";
  if (!visible) return;
  (snapshot.talent_options || []).forEach((talent) => {
    const card = document.createElement("button");
    card.className = "talent-card";
    card.innerHTML = `
      <div class="reward-title"><span>${esc(talent.name || "道基天赋")}</span><span>${esc(talent.school || "无")}</span></div>
      <div class="reward-desc">${esc(talent.desc || "让这次突破沉淀为长期成长。")}</div>
    `;
    card.addEventListener("click", () => api("/api/talent", { index: talent.index }));
    wrap.appendChild(card);
  });
}

function renderRoutes(snapshot) {
  const panel = $("route-panel");
  const wrap = $("routes");
  const visible = snapshot.phase === "route_choice";
  panel.classList.toggle("hidden", !visible);
  wrap.innerHTML = "";
  if (!visible) return;
  (snapshot.route_options || []).forEach((route) => {
    const reward = route.immediate_reward || {};
    const card = document.createElement("button");
    card.className = "route-card";
    card.innerHTML = `
      <div class="reward-title"><span>${esc(route.title || "未知路线")}</span><span>${esc(route.risk || "风险")}</span></div>
      <div class="reward-desc">${esc(route.desc || "")}</div>
      <div class="fit-row">
        <span class="fit-badge">${esc(route.enemy_realm || "凡人")}</span>
        <span class="fit-badge">${esc(route.reward_bias || "均衡")}</span>
        <span class="fit-badge">${route.years_preview || 0} 年</span>
        <span class="fit-badge">${esc(route.tribulation_preview || "雷劫风险低")}</span>
        ${route.hp_cost_pct ? `<span class="fit-badge">元神 -${route.hp_cost_pct}%</span>` : ""}
        ${route.cultivation_cost ? `<span class="fit-badge">修为 -${route.cultivation_cost}</span>` : ""}
        <span class="fit-badge">劫气 ${route.debt_delta > 0 ? "+" : ""}${route.debt_delta || 0}</span>
        ${route.special_rule ? `<span class="fit-badge">${esc(route.special_rule)}</span>` : ""}
      </div>
      <div class="reward-desc">${esc(route.story_hint || "")}</div>
      <div class="reward-desc">${esc(route.material_preview || "")} · ${esc(route.reward_preview || "")} · ${esc(route.enemy_hint || "")}</div>
      ${route.chain_preview ? `<div class="reward-desc route-chain">${esc(route.chain_preview)}</div>` : ""}
      ${route.special_rule ? `<div class="reward-desc">${esc(route.special_hint || "")}</div>` : ""}
      <div class="reward-desc">${route.reward_bias === "还阳" ? "若修为足够，将尝试还阳；否则转化为护魂修为。" : `即得：${esc(reward.name || "修炼资源")} ${reward.amount ? `x${reward.amount}` : ""}`}</div>
    `;
    card.addEventListener("click", () => api("/api/route", { index: route.index }));
    wrap.appendChild(card);
  });
}

function chronicleKindName(kind) {
  const names = {
    start: "入道",
    route: "路线",
    time: "岁月",
    breakthrough: "突破",
    talent: "天赋",
    tribulation: "渡劫",
    heart: "心魔",
    near_death: "濒死",
    rebirth: "还阳",
    ascension: "飞升",
    lifespan: "寿尽",
    soul_decay: "魂火",
    tribulation_death: "雷劫",
    defeat: "败亡",
  };
  return names[kind] || "事件";
}

function renderChronicle(snapshot) {
  const chronicle = snapshot.chronicle || [];
  $("chronicle-count").textContent = chronicle.length;
  $("chronicle-list").innerHTML = chronicle.length
    ? chronicle.slice(-8).reverse().map((item) => `
      <div class="timeline-item ${esc(item.kind || "event")}">
        <strong>${item.age || 0}岁 · ${esc(item.realm || "")}</strong>
        <span>${esc(item.text || "")}</span>
      </div>
    `).join("")
    : `<div class="action-meta">道途尚未留下更多痕迹。</div>`;
  $("chronicle-open").disabled = !chronicle.length;
  $("chronicle-modal").classList.toggle("hidden", !state.chronicleOpen);
  $("chronicle-full-list").innerHTML = chronicle.length
    ? chronicle.map((item, index) => `
      <article class="chronicle-full-item ${esc(item.kind || "event")}">
        <div class="chronicle-marker">${index + 1}</div>
        <div>
          <div class="chronicle-meta">
            <span>${item.age || 0}岁</span>
            <span>${esc(item.realm || "")}</span>
            <span>${esc(chronicleKindName(item.kind))}</span>
          </div>
          <p>${esc(item.text || "")}</p>
        </div>
      </article>
    `).join("")
    : `<div class="action-meta">暂无年表。</div>`;
}

function renderTribulation(snapshot) {
  const panel = $("tribulation-panel");
  const wrap = $("tribulation-options");
  const data = snapshot.tribulation_event || {};
  const visible = snapshot.phase === "tribulation_event" && data.active;
  panel.classList.toggle("hidden", !visible);
  wrap.innerHTML = "";
  if (!visible) return;
  $("tribulation-odds").textContent = `基础成功率 ${data.success_chance || 0}%`;
  $("tribulation-reason").textContent = data.reason || "劫云骤聚，天雷将落。";
  (data.options || []).forEach((option) => {
    const card = document.createElement("button");
    card.className = `tribulation-card ${option.mode || ""}`;
    card.disabled = !option.available;
    card.innerHTML = `
      <div class="reward-title"><span>${esc(option.title || "渡劫")}</span><span>${option.success_chance || 0}%</span></div>
      <div class="reward-desc">${esc(option.desc || "")}</div>
      <div class="fit-row"><span class="fit-badge">${esc(option.cost || "")}</span></div>
    `;
    card.addEventListener("click", () => api("/api/tribulation", { choice: option.index }));
    wrap.appendChild(card);
  });
}

function renderNearDeath(snapshot) {
  const panel = $("near-death-panel");
  const wrap = $("near-death-options");
  const data = snapshot.near_death || {};
  const visible = snapshot.phase === "near_death_choice" && data.active;
  panel.classList.toggle("hidden", !visible);
  wrap.innerHTML = "";
  if (!visible) return;
  $("near-death-odds").textContent = data.can_ghost ? `阴魂 ${data.ghost_chance || 0}%` : "生死一线";
  $("near-death-reason").innerHTML = `
    <strong>${esc(data.reason || "肉身濒毁")}</strong>
    <div>遁逃代价：修为 -${data.escape_cost || 0}。若修为不足，将以诅咒和重伤抵偿。</div>
  `;
  (data.options || []).forEach((option) => {
    const card = document.createElement("button");
    card.className = `near-death-card ${option.type || ""}`;
    card.innerHTML = `
      <div class="reward-title"><span>${esc(option.title || "抉择")}</span><span>${esc(option.type || "")}</span></div>
      <div class="reward-desc">${esc(option.desc || "")}</div>
      <div class="reward-desc">${esc(option.cost || "")}</div>
    `;
    card.addEventListener("click", () => api("/api/near_death", { index: option.index }));
    wrap.appendChild(card);
  });
}

function renderPreparation(snapshot) {
  const panel = $("preparation-panel");
  const visible = snapshot.phase === "preparation";
  panel.classList.toggle("hidden", !visible);
  const materials = snapshot.materials || {};
  $("preparation-materials").textContent = `灵材 ${materials.spirit || 0} · 药材 ${materials.herbs || 0}`;
  $("artifact-upgrades").innerHTML = "";
  $("alchemy-recipes").innerHTML = "";
  document.querySelectorAll(".alchemy-hint").forEach((node) => node.remove());
  if (!visible) return;

  const artifacts = snapshot.artifacts || [];
  if (!artifacts.length) {
    $("artifact-upgrades").innerHTML = `<div class="action-meta">暂无可强化法器</div>`;
  } else {
    artifacts.forEach((item) => {
      const btn = document.createElement("button");
      btn.className = "preparation-card";
      btn.disabled = !item.can_upgrade;
      const capped = (item.level || 0) >= (item.max_level || state.snapshot?.limits?.artifact_max_level || 3);
      btn.innerHTML = `
        <div class="reward-title"><span>${esc(item.name || "未知法器")} +${item.level || 0}</span><span>${capped ? "满级" : `灵材 ${item.upgrade_cost || 0}`}</span></div>
        <div class="fit-row">
          <span class="fit-badge">${esc(item.school || "无")}</span>
          <span class="fit-badge">${esc(item.resonance || "暂无共鸣")}</span>
        </div>
        <div class="reward-desc">${esc(item.desc || "")}</div>
      `;
      btn.addEventListener("click", () => api("/api/artifact/upgrade", { slot: item.slot }));
      $("artifact-upgrades").appendChild(btn);
    });
  }

  const recipes = snapshot.crafting?.recipes || [];
  const crafting = snapshot.crafting || {};
  recipes.forEach((recipe) => {
    const btn = document.createElement("button");
    btn.className = "preparation-card alchemy-card";
    btn.disabled = !recipe.can_brew;
    const status = recipe.can_brew ? "可炼" : (recipe.realm_ready ? "药材不足" : `需 ${esc(recipe.required_realm || "")}`);
    btn.innerHTML = `
      <div class="reward-title"><span>${esc(recipe.name || "丹方")}</span><span>药材 ${recipe.cost || 0}</span></div>
      <div class="fit-row">
        <span class="fit-badge">${status}</span>
        ${recipe.custom ? `<span class="fit-badge">自创</span>` : ""}
      </div>
      <div class="reward-desc">${esc(recipe.desc || "")}</div>
    `;
    btn.addEventListener("click", () => api("/api/elixir/brew", { recipe: recipe.index }));
    $("alchemy-recipes").appendChild(btn);
  });
  if (!recipes.length) $("alchemy-recipes").innerHTML = `<div class="action-meta">暂无丹方</div>`;
  if (crafting.locked_count) {
    $("alchemy-recipes").insertAdjacentHTML("afterend", `<div class="action-meta alchemy-hint">未掌握丹方 ${crafting.locked_count} · ${esc(crafting.unlock_hint || "")}</div>`);
  }
}

function renderReplacement(snapshot) {
  const panel = $("replacement-panel");
  const replacement = snapshot.replacement || {};
  const visible = snapshot.phase === "reward_replace" && replacement.active;
  panel.classList.toggle("hidden", !visible);
  $("replacement-options").innerHTML = "";
  $("replacement-new").innerHTML = "";
  if (!visible) return;

  const reward = replacement.reward || {};
  const kindNames = { skill: "技能槽", artifact: "法器槽", elixir: "丹药袋" };
  $("replacement-title").textContent = "替换确认";
  $("replacement-kind").textContent = kindNames[replacement.type] || "取舍";
  $("replacement-new").innerHTML = `
    <strong>新获得：${esc(reward.name || "奖励")}</strong>
    <div>${esc(reward.fit_reason || reward.desc || "选择一个槽位完成替换，或保留当前配置。")}</div>
  `;

  const addOption = (slot, title, desc, keep = false) => {
    const btn = document.createElement("button");
    btn.className = `replacement-card${keep ? " keep" : ""}`;
    btn.innerHTML = `
      <div class="reward-title"><span>${esc(title)}</span><span>${keep ? "保留" : "替换"}</span></div>
      <div class="reward-desc">${esc(desc)}</div>
    `;
    btn.addEventListener("click", () => api("/api/replacement", { slot }));
    $("replacement-options").appendChild(btn);
  };

  (replacement.options || []).forEach((option) => {
    const label = replacement.type === "skill"
      ? `${text(option.name, "当前技能")} · ${text(option.school, "无流派")}`
      : text(option.name, "当前物品");
    const desc = replacement.type === "skill"
      ? `${text(option.desc, "")} ${option.effect_summary ? ` · ${option.effect_summary}` : ""} ${option.rank !== undefined ? `· ${option.rank} 阶 · ${option.cost} 气` : ""}`
      : text(option.desc, "");
    addOption(option.slot, label, desc);
  });
  addOption(-1, "保留当前配置", "放弃这次替换，已领悟技能会留在技能池中；满槽物品会被放弃。", true);
}

function renderSkillTree(snapshot) {
  const panel = $("skill-tree-panel");
  const wrap = $("skill-tree-list");
  const tree = snapshot.skill_tree || {};
  const visible = state.skillTreeOpen;
  panel.classList.toggle("hidden", !visible);
  $("skill-tree-toggle").classList.toggle("active", visible);
  $("skill-tree-state").textContent = tree.can_respec ? "消耗修为换诀" : "局后可洗点";
  wrap.innerHTML = "";
  if (!visible) return;
  const skills = tree.skills || [];
  if (!skills.length) {
    wrap.innerHTML = `<div class="action-meta">暂无已悟功法</div>`;
    return;
  }
  skills.forEach((skill) => {
    const card = document.createElement("article");
    card.className = `skill-tree-card${skill.equipped ? " equipped" : ""}`;
    card.innerHTML = `
      <div class="reward-title"><span>${esc(skill.name || "未知功法")}</span><span>${skill.equipped ? "已装备" : `修为 ${skill.cost || 0}`}</span></div>
      <div class="fit-row">
        <span class="fit-badge">${esc(skill.action || "行动")}</span>
        <span class="fit-badge">${esc(skill.school || "无")}</span>
        <span class="fit-badge">${skill.rank || 0} 阶</span>
        <span class="fit-badge">${skill.effective_qi_cost ?? skill.qi_cost ?? 0} 气</span>
        <span class="fit-badge">熟练 Lv${skill.mastery_level || 0} · ${skill.mastery_xp || 0}/${skill.mastery_next || 3}</span>
        ${skill.refine && skill.refine !== "未洗练" ? `<span class="fit-badge">${esc(skill.refine)}</span>` : ""}
        ${skill.bypassed ? `<span class="fit-badge">破格</span>` : ""}
      </div>
      <div class="reward-desc">${esc(skill.desc || "")}</div>
      ${skill.effect_summary ? `<div class="reward-desc">${esc(skill.effect_summary)}</div>` : ""}
      ${skill.refine_summary ? `<div class="reward-desc">${esc(skill.refine_summary)}</div>` : ""}
      <div class="skill-tree-actions">
        <button class="ghost-btn equip-skill-btn" ${skill.equipped || !tree.can_respec || (snapshot.player?.cultivation || 0) < (skill.cost || 0) ? "disabled" : ""}>装备</button>
        <button class="ghost-btn refine-skill-btn" ${!tree.can_respec || (snapshot.player?.cultivation || 0) < (skill.refine_cost || 0) ? "disabled" : ""}>洗练 ${skill.refine_cost || 0}</button>
      </div>
    `;
    card.querySelector(".equip-skill-btn").addEventListener("click", () => api("/api/equip_skill", { id: skill.id }));
    card.querySelector(".refine-skill-btn").addEventListener("click", () => api("/api/skill/refine", { id: skill.id }));
    wrap.appendChild(card);
  });
}

function field(label, value, onInput, type = "text") {
  const wrap = document.createElement("label");
  wrap.className = "workshop-field";
  wrap.innerHTML = `<span>${esc(label)}</span>`;
  const input = document.createElement(type === "textarea" ? "textarea" : "input");
  if (type !== "textarea") input.type = type;
  input.value = value ?? "";
  input.addEventListener("input", () => {
    onInput(input.value);
    state.workshopDirty = true;
    syncWorkshopChrome(state.snapshot || {});
  });
  wrap.appendChild(input);
  return wrap;
}

function selectField(label, value, options, onInput) {
  const wrap = document.createElement("label");
  wrap.className = "workshop-field";
  wrap.innerHTML = `<span>${esc(label)}</span>`;
  const select = document.createElement("select");
  options.forEach(([id, name]) => {
    const option = document.createElement("option");
    option.value = id;
    option.textContent = name;
    option.selected = id === value;
    select.appendChild(option);
  });
  select.addEventListener("change", () => {
    onInput(select.value);
    state.workshopDirty = true;
    syncWorkshopChrome(state.snapshot || {});
  });
  wrap.appendChild(select);
  return wrap;
}

const actionOptions = [["melee", "近战"], ["ranged", "远程"], ["defend", "防御"], ["heal", "治疗"], ["counter", "反击"], ["boost", "增益"], ["smite", "重击"], ["burst", "爆发"], ["terminate", "终结"], ["gain_qi", "集气"]];
const schoolOptions = [["basic", "基础"], ["fire", "火法"], ["sword", "剑修"], ["blood", "血道"], ["defense", "护体"], ["qi", "气修"], ["thunder", "雷法"], ["dark", "幽冥"]];
const realmOptions = [["mortal", "凡人"], ["refining", "炼气"], ["foundation", "筑基"], ["core", "结丹"], ["nascent", "元婴"], ["severing", "化神"], ["refinement", "炼虚"], ["unity", "合体"], ["great", "大乘"], ["ascension", "飞升"]];
const rarityOptions = [["common", "普通"], ["rare", "稀有"], ["epic", "史诗"], ["legendary", "传说"]];
const effectOptions = [["none", "无"], ["damage", "追加伤害"], ["heal_hp_pct", "恢复生命%"], ["gain_qi_pct", "获得灵气%"], ["bleed", "流血"], ["curse", "诅咒"], ["enrage", "激怒"], ["cleanse", "净化"], ["cultivation", "获得修为"]];
const typeOptions = [["slash", "斩击"], ["smash", "粉碎"], ["pierce", "穿刺"], ["burst", "连发"], ["blast", "爆破"], ["project", "投射"], ["resist", "抵抗"], ["shield", "护盾"], ["forcefield", "力场"], ["parry", "格挡"], ["reflect", "反射"], ["heal", "治疗"], ["buff", "增益"], ["debuff", "减益"]];
const attrOptions = [["physical", "物理"], ["fire", "火"], ["ice", "冰"], ["wind", "风"], ["wood", "木"], ["metal", "金"], ["thunder", "雷"], ["earth", "土"], ["light", "光"], ["dark", "暗"], ["blood", "血"], ["spiritual", "灵"], ["karma", "因果"], ["space", "空间"], ["none", "无"]];
const targetOptions = [["enemy", "敌人"], ["self", "自身"], ["none", "无"]];

function defaultItem(tab) {
  if (tab === "skills") return { name: "自创功法", desc: "<%s 施展自创功法。>", action_type: "melee", school: "sword", realm: "mortal", cost: 1, rank: 0, base_power: 10, type_id: "slash", attribute: "physical", target: "enemy", custom_effect: "none", custom_value: 0 };
  if (tab === "artifacts") return { name: "自创法器", desc: "玩家自创法器。", rarity: "common", max_hp_pct: 0, max_qi_pct: 0, breakthrough_pct: 0, post_battle_cultivation: 0, reward_school_bias: "basic" };
  return { name: "自创丹药", desc: "玩家自创丹药。", rarity: "common", heal_hp_pct: 20, gain_qi_pct: 0, clear_negative: false, breakthrough_pct: 0, cultivation_gain: 0 };
}

function syncWorkshopChrome(snapshot) {
  const panel = $("workshop-panel");
  if (!panel) return;
  const visible = state.workshopOpen;
  panel.classList.toggle("hidden", !visible);
  $("workshop-toggle").classList.toggle("active", visible);
  if (!visible) return;
  ["skills", "artifacts", "elixirs"].forEach((tab) => {
    $(`workshop-tab-${tab}`).classList.toggle("active", state.workshopTab === tab);
  });
  if (state.workshopDirty) {
    $("workshop-state").textContent = "未保存";
    return;
  }
  const status = snapshot.content_status;
  if (status && !$("workshop-state").textContent.includes("编译")) {
    $("workshop-state").textContent = status.enabled
      ? `技能 ${status.counts.skills} · 法器 ${status.counts.artifacts} · 丹药 ${status.counts.elixirs}`
      : "暂无自创内容";
  }
}

function renderWorkshop(snapshot) {
  const panel = $("workshop-panel");
  if (!panel) return;
  syncWorkshopChrome(snapshot);
  if (!state.workshopOpen) return;
  const list = $("workshop-list");
  list.innerHTML = "";
  const items = state.content[state.workshopTab] || [];
  items.forEach((item, index) => {
    const card = document.createElement("div");
    card.className = "workshop-card";
    const remove = document.createElement("button");
    remove.className = "ghost-btn";
    remove.textContent = "删除";
    remove.addEventListener("click", () => {
      items.splice(index, 1);
      state.workshopDirty = true;
      renderWorkshop(state.snapshot || {});
    });
    const head = document.createElement("div");
    head.className = "reward-title";
    head.innerHTML = `<span>${esc(item.name || "未命名")}</span>`;
    head.appendChild(remove);
    card.appendChild(head);
    card.appendChild(field("名称", item.name, (v) => item.name = v));
    card.appendChild(field("描述", item.desc, (v) => item.desc = v, "textarea"));
    if (state.workshopTab === "skills") {
      card.appendChild(selectField("行动", item.action_type || "melee", actionOptions, (v) => item.action_type = v));
      card.appendChild(selectField("流派", item.school || "basic", schoolOptions, (v) => item.school = v));
      card.appendChild(selectField("境界", item.realm || "mortal", realmOptions, (v) => item.realm = v));
      card.appendChild(selectField("类型", item.type_id || "slash", typeOptions, (v) => item.type_id = v));
      card.appendChild(selectField("属性", item.attribute || "physical", attrOptions, (v) => item.attribute = v));
      card.appendChild(selectField("目标", item.target || "enemy", targetOptions, (v) => item.target = v));
      card.appendChild(selectField("模板效果", item.custom_effect || "none", effectOptions, (v) => item.custom_effect = v));
      card.appendChild(field("气耗", item.cost ?? 1, (v) => item.cost = Number(v), "number"));
      card.appendChild(field("阶数", item.rank ?? 0, (v) => item.rank = Number(v), "number"));
      card.appendChild(field("威力x10", item.base_power ?? 10, (v) => item.base_power = Number(v), "number"));
      card.appendChild(field("效果值", item.custom_value ?? 0, (v) => item.custom_value = Number(v), "number"));
    } else if (state.workshopTab === "artifacts") {
      card.appendChild(selectField("稀有度", item.rarity || "common", rarityOptions, (v) => item.rarity = v));
      card.appendChild(selectField("奖励倾向", item.reward_school_bias || "basic", schoolOptions, (v) => item.reward_school_bias = v));
      card.appendChild(field("最大生命%", item.max_hp_pct ?? 0, (v) => item.max_hp_pct = Number(v), "number"));
      card.appendChild(field("最大灵气%", item.max_qi_pct ?? 0, (v) => item.max_qi_pct = Number(v), "number"));
      card.appendChild(field("突破率%", item.breakthrough_pct ?? 0, (v) => item.breakthrough_pct = Number(v), "number"));
      card.appendChild(field("战后修为", item.post_battle_cultivation ?? 0, (v) => item.post_battle_cultivation = Number(v), "number"));
    } else {
      card.appendChild(selectField("稀有度", item.rarity || "common", rarityOptions, (v) => item.rarity = v));
      card.appendChild(field("恢复生命%", item.heal_hp_pct ?? 0, (v) => item.heal_hp_pct = Number(v), "number"));
      card.appendChild(field("获得灵气%", item.gain_qi_pct ?? 0, (v) => item.gain_qi_pct = Number(v), "number"));
      card.appendChild(field("突破率%", item.breakthrough_pct ?? 0, (v) => item.breakthrough_pct = Number(v), "number"));
      card.appendChild(field("修为", item.cultivation_gain ?? 0, (v) => item.cultivation_gain = Number(v), "number"));
    }
    list.appendChild(card);
  });
  if (!items.length) list.innerHTML = `<div class="action-meta">暂无内容，点击新增开始创作。</div>`;
}

function renderEncounter(snapshot) {
  const visible = snapshot.phase === "encounter" || snapshot.phase === "between_battles" || snapshot.phase === "defeat" || snapshot.phase === "victory";
  $("encounter-panel").classList.toggle("hidden", !visible);
  if (!visible) return;
  if (snapshot.phase === "defeat" || snapshot.phase === "victory") {
    const ending = snapshot.ending || {};
    $("encounter-title").textContent = text(ending.title, snapshot.phase === "victory" ? "羽化飞升" : "道途暂止");
    $("encounter-text").textContent = text(ending.summary, snapshot.phase === "victory" ? "劫尽道成，飞升上界。" : "这一次修行止步于此。重置后可重新开局。");
    $("encounter-penalty").textContent = "";
    $("encounter-reward").textContent = `终局：${esc(ending.realm || "")} · ${ending.age || 0} 岁`;
    $("next-battle").textContent = "";
    return;
  }
  const next = snapshot.next_battle || {};
  if (snapshot.phase === "between_battles") {
    $("encounter-title").textContent = "整备";
    const life = snapshot.life || {};
    $("encounter-text").textContent = life.last_years_elapsed
      ? `闭关行走 ${life.last_years_elapsed} 年，灵息渐稳，下一场战斗正在逼近。`
      : "灵息渐稳，下一场战斗正在逼近。";
    $("encounter-penalty").textContent = "";
    $("encounter-reward").textContent = "调整构筑后继续前行。";
    const bossLine = next.boss_name ? `<br>Boss：${esc(next.boss_name)} · ${esc(next.boss_hint || "")}` : "";
    $("next-battle").innerHTML = `<strong>下一战：第 ${next.index || 0} 战 · ${esc(next.type || "normal")} · ${esc(next.risk || "普通")}</strong><br>${esc(next.enemy || "未知对手")} · ${esc(next.tendency || "未知倾向")} · ${esc(next.enemy_realm || "凡人")}<br>奖励倾向：${esc(next.reward_bias || "均衡")}${bossLine}<br>${esc(next.threat || "")}`;
    return;
  }
  const enc = snapshot.encounter || {};
  $("encounter-title").textContent = text(enc.title, "奇遇");
  $("encounter-text").textContent = text(enc.text, "天地异象一闪而过。");
  $("encounter-penalty").textContent = enc.penalty ? `损失 ${enc.penalty} 生命` : "无损";
  $("encounter-reward").textContent = enc.reward ? `获得：${text(enc.reward.name, "修炼资源")} ${enc.reward.amount ? `x${enc.reward.amount}` : ""}` : "";
  const bossLine = next.boss_name ? `<br>Boss：${esc(next.boss_name)} · ${esc(next.boss_hint || "")}` : "";
  $("next-battle").innerHTML = `<strong>下一战：第 ${next.index || 0} 战 · ${esc(next.type || "normal")} · ${esc(next.risk || "普通")}</strong><br>${esc(next.enemy || "未知对手")} · ${esc(next.tendency || "未知倾向")} · ${esc(next.enemy_realm || "凡人")}<br>奖励倾向：${esc(next.reward_bias || "均衡")}${bossLine}<br>${esc(next.threat || "")}`;
}

function renderSide(snapshot) {
  const build = snapshot.build || {};
  const run = snapshot.run || {};
  $("battle-index").textContent = `第 ${run.battle_index || 0} 战`;
  $("build-archetype").textContent = text(build.archetype, "未成型");
  $("build-schools").innerHTML = `
    <span class="pill">主：${esc(build.primary || "无")} (${build.primary_count || 0})</span>
    <span class="pill">副：${esc(build.secondary || "无")} (${build.secondary_count || 0})</span>
    <span class="pill">${esc(build.synergy_label || "未成型")}</span>
    <span class="pill">技能：${build.equipped_skills || 0} / 已悟 ${build.unlocked_skills || 0}</span>
    <span class="pill">精英 ${run.elite_kills || 0} · Boss ${run.boss_kills || 0}</span>
  `;
  if (build.synergy_bonus) {
    $("build-schools").innerHTML += `<div class="action-meta">${esc(build.synergy_bonus)}</div>`;
  }
  if ((build.school_cycles || []).length) {
    $("build-schools").innerHTML += (build.school_cycles || [])
      .map((item) => `<span class="pill">${esc(item.summary || `${item.school} ${item.value}`)}</span>`)
      .join("");
  }
  const breakthrough = snapshot.breakthrough || {};
  const cultivation = snapshot.player?.cultivation || 0;
  const player = snapshot.player || {};
  $("cultivation-count").textContent = cultivation;
  $("breakthrough-side").innerHTML = `
    <span class="pill">突破：${esc(breakthrough.current_realm || "当前")} -> ${esc(breakthrough.next_realm || "下一境界")}</span>
    <span class="pill">需求：${breakthrough.cost || 0} · 成功率 ${breakthrough.chance || 0}%</span>
    <span class="pill">小境界：${player.minor_realm_level || 0}/3 · 理解 ${player.minor_understanding || 0}/${player.minor_threshold || 3}</span>
    <span class="pill">小境界加成：HP/QI +${player.minor_bonus_pct || 0}%</span>
  `;

  const routeMeta = snapshot.route_meta || {};
  $("build-schools").innerHTML += `
    <span class="pill">劫气：${routeMeta.calamity_debt || 0}</span>
    <span class="pill">连续：${esc(routeMeta.last_route || "未择路线")} x${routeMeta.route_streak || 0}</span>
    <div class="action-meta">${esc(routeMeta.hint || "")}</div>
  `;

  const materials = snapshot.materials || {};
  $("material-count").textContent = `${materials.spirit || 0} / ${materials.herbs || 0}`;
  $("materials-side").innerHTML = `
    <span class="pill">灵材：${materials.spirit || 0}</span>
    <span class="pill">药材：${materials.herbs || 0}</span>
    <div class="action-meta">灵材用于强化法器，药材用于局后炼丹。</div>
  `;

  const life = snapshot.life || {};
  const trib = snapshot.tribulation || {};
  $("life-count").textContent = `${life.age || 0} / ${life.lifespan || 0}`;
  $("life-side").innerHTML = `
    <span class="pill">年龄：${life.age || 0} 岁</span>
    <span class="pill">寿元：${life.lifespan || 0} 岁</span>
    <span class="pill">余寿：${life.years_remaining || 0} 年</span>
    <span class="pill">${esc(life.life_pressure_label || "寿元尚宽")}</span>
    <span class="pill">雷劫：${esc(trib.label || "天机平稳")} (${trib.pressure || 0})</span>
    <div class="action-meta">${esc(trib.hint || "")}</div>
  `;

  renderChronicle(snapshot);

  $("soul-state").textContent = text(player.soul_state_label, "肉身");
  $("soul-side").innerHTML = player.soul_state === "ghost"
    ? `<span class="pill ghost-pill">鬼魂状态</span><span class="pill">再死则魂飞魄散</span><div class="action-meta">寻找还阳坛并积累修为，可重凝肉身。</div>`
    : `<span class="pill">肉身尚存</span><div class="action-meta">濒死时可选择遁逃，或在稀有机缘下转为鬼修。</div>`;

  const talents = snapshot.talents || [];
  $("talent-count").textContent = `${talents.length} / 8`;
  $("talents").innerHTML = talents.length
    ? talents.map((item) => `<span class="pill" title="${esc(item.desc)}">${esc(item.name || "未知天赋")}</span>`).join("")
    : `<span class="pill">暂无天赋</span>`;

  const artifacts = snapshot.artifacts || [];
  $("artifact-count").textContent = `${artifacts.length} / ${snapshot.limits?.artifact_slots || artifacts[0]?.slot_limit || 4}`;
  $("artifacts").innerHTML = artifacts.length
    ? artifacts.map((item) => `<span class="pill" title="${esc(item.desc)}">${esc(item.name || "未知法器")} +${item.level || 0} · ${esc(item.resonance || "无共鸣")}</span>`).join("")
    : `<span class="pill">暂无法器</span>`;

  const elixirs = snapshot.elixirs || [];
  $("elixir-count").textContent = `${elixirs.length} / 6`;
  $("elixirs").innerHTML = "";
  elixirs.forEach((item) => {
    const btn = document.createElement("button");
    btn.className = "elixir-btn";
    btn.disabled = snapshot.phase !== "battle_action";
    btn.innerHTML = `<strong>${esc(item.name || "丹药")}</strong><div class="action-meta">${esc(item.desc)}</div>`;
    btn.addEventListener("click", () => api("/api/elixir", { slot: item.slot }));
    $("elixirs").appendChild(btn);
  });
  if (!elixirs.length) $("elixirs").innerHTML = `<div class="action-meta">丹囊为空</div>`;

  const logs = [...(snapshot.bridge_logs || []), ...(snapshot.logs || [])].slice(-6);
  $("log-count").textContent = logs.length;
  $("logs").innerHTML = logs.length
    ? logs.map((line) => `<div class="log-item">${esc(line)}</div>`).join("")
    : `<div class="log-item">暂无日志</div>`;
}

function renderRecords() {
  const asc = state.records.ascensions || [];
  const mem = state.records.memorials || [];
  $("records-count").textContent = `${asc.length} / ${mem.length}`;
  const ascHtml = asc.slice(0, 5).map((item, index) =>
    `<div class="record-item"><strong>${index + 1}. ${item.age || 0}岁飞升</strong><span>${esc(item.primary_school || "无流派")} · ${item.battle_index || 0}战 · Boss ${item.boss_kills || 0}</span></div>`
  ).join("");
  const memHtml = mem.slice(0, 4).map((item) =>
    `<div class="record-item muted"><strong>${esc(item.title || "道途遗卷")} · ${item.age || 0}岁</strong><span>${esc(item.realm || "")} · ${esc(item.primary_school || "无流派")}</span></div>`
  ).join("");
  $("records-list").innerHTML = ascHtml || memHtml
    ? `${ascHtml}${memHtml}`
    : `<div class="action-meta">暂无终局记录。</div>`;
}

function render(snapshot, options = {}) {
  state.snapshot = snapshot;
  if (snapshot.error) {
    $("phase-line").textContent = snapshot.error;
    return;
  }
  $("phase-line").textContent = `${phaseNames[snapshot.phase] || snapshot.phase} · ${text(snapshot.build?.archetype, "构筑未定")}`;
  $("details-modal").classList.toggle("hidden", !state.detailsOpen);
  $("details-toggle").classList.toggle("active", state.detailsOpen);
  $("chronicle-modal").classList.toggle("hidden", !state.chronicleOpen);
  $("round-number").textContent = `第 ${snapshot.round || 0} 回合`;
  renderFighter("player", snapshot.player);
  renderFighter("enemy", snapshot.enemy);
  renderCombatFeed(snapshot);
  renderActions(snapshot);
  renderRewards(snapshot);
  renderReplacement(snapshot);
  renderSkillTree(snapshot);
  if (options.preserveWorkshop) syncWorkshopChrome(snapshot);
  else renderWorkshop(snapshot);
  renderBreakthrough(snapshot);
  renderHeartDemon(snapshot);
  renderTalentChoice(snapshot);
  renderNearDeath(snapshot);
  renderTribulation(snapshot);
  renderPreparation(snapshot);
  renderRoutes(snapshot);
  renderEncounter(snapshot);
  renderSide(snapshot);
  $("continue-btn").disabled = !["battle_result", "encounter", "between_battles"].includes(snapshot.phase);
  loadRecords();
}

function initBackground() {
  const canvas = $("qi-bg");
  const ctx = canvas.getContext("2d");
  let w = 0;
  let h = 0;
  const motes = Array.from({ length: 58 }, () => ({
    x: Math.random(),
    y: Math.random(),
    r: 1 + Math.random() * 2.5,
    s: 0.12 + Math.random() * 0.4,
  }));
  function resize() {
    w = canvas.width = window.innerWidth * window.devicePixelRatio;
    h = canvas.height = window.innerHeight * window.devicePixelRatio;
  }
  function frame() {
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = "rgba(102, 207, 232, 0.5)";
    motes.forEach((mote) => {
      mote.y -= mote.s / 1000;
      if (mote.y < -0.05) mote.y = 1.05;
      ctx.beginPath();
      ctx.arc(mote.x * w, mote.y * h, mote.r * window.devicePixelRatio, 0, Math.PI * 2);
      ctx.fill();
    });
    requestAnimationFrame(frame);
  }
  window.addEventListener("resize", resize);
  resize();
  frame();
}

$("start-btn").addEventListener("click", () => api("/api/start"));
$("reset-btn").addEventListener("click", () => api("/api/reset"));
$("continue-btn").addEventListener("click", () => api("/api/continue"));
$("skill-tree-toggle").addEventListener("click", () => {
  state.skillTreeOpen = !state.skillTreeOpen;
  render(state.snapshot || {});
});
$("details-toggle").addEventListener("click", () => {
  state.detailsOpen = true;
  render(state.snapshot || {}, { preserveWorkshop: true });
});
$("details-close").addEventListener("click", () => {
  state.detailsOpen = false;
  render(state.snapshot || {}, { preserveWorkshop: true });
});
$("details-modal").addEventListener("click", (event) => {
  if (event.target.classList.contains("details-backdrop")) {
    state.detailsOpen = false;
    render(state.snapshot || {}, { preserveWorkshop: true });
  }
});
$("chronicle-open").addEventListener("click", () => {
  state.chronicleOpen = true;
  render(state.snapshot || {}, { preserveWorkshop: true });
});
$("chronicle-close").addEventListener("click", () => {
  state.chronicleOpen = false;
  render(state.snapshot || {}, { preserveWorkshop: true });
});
$("chronicle-modal").addEventListener("click", (event) => {
  if (event.target.classList.contains("chronicle-backdrop")) {
    state.chronicleOpen = false;
    render(state.snapshot || {}, { preserveWorkshop: true });
  }
});
$("workshop-toggle").addEventListener("click", () => {
  state.workshopOpen = !state.workshopOpen;
  if (state.workshopOpen && !state.workshopLoaded) loadContent();
  if (state.workshopOpen) renderWorkshop(state.snapshot || {});
  else render(state.snapshot || {}, { preserveWorkshop: true });
});
["skills", "artifacts", "elixirs"].forEach((tab) => {
  $(`workshop-tab-${tab}`).addEventListener("click", () => {
    state.workshopTab = tab;
    renderWorkshop(state.snapshot || {});
  });
});
$("workshop-add-btn").addEventListener("click", () => {
  const items = state.content[state.workshopTab] || [];
  items.push(defaultItem(state.workshopTab));
  state.content[state.workshopTab] = items;
  state.workshopDirty = true;
  renderWorkshop(state.snapshot || {});
});
$("workshop-save-btn").addEventListener("click", saveContent);
$("workshop-reset-btn").addEventListener("click", resetContent);
$("attempt-breakthrough-btn").addEventListener("click", () => api("/api/breakthrough"));
$("skip-breakthrough-btn").addEventListener("click", () => api("/api/skip_breakthrough"));
$("skip-preparation-btn").addEventListener("click", () => api("/api/preparation/skip"));
initBackground();
refresh();
loadContent();
loadRecords(true);
setInterval(refresh, 1800);
