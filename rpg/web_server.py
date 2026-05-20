#!/usr/bin/env python3
import json
import os
import re
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parent
UI_DIR = ROOT / "ui"
CUSTOM_CONTENT_PATH = ROOT / "custom_content.json"
CUSTOM_INC_PATH = ROOT / "rpg_custom_content.inc"
RUN_RECORDS_PATH = ROOT / "run_records.json"
STATE_PREFIX = "##UI_JSON##:"
ANSI_RE = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")

ACTION_TYPES = {
    "gain_qi": "ACTION_TYPE_GAIN_QI",
    "melee": "ACTION_TYPE_MELEE",
    "ranged": "ACTION_TYPE_RANGED",
    "defend": "ACTION_TYPE_DEFEND",
    "heal": "ACTION_TYPE_HEAL",
    "counter": "ACTION_TYPE_COUNTER",
    "boost": "ACTION_TYPE_BOOST",
    "smite": "ACTION_TYPE_SMITE",
    "burst": "ACTION_TYPE_BURST",
    "terminate": "ACTION_TYPE_TERMINATE",
}
TYPE_IDS = {
    "slash": "TYPE_SLASH",
    "smash": "TYPE_SMASH",
    "pierce": "TYPE_PIERCE",
    "burst": "TYPE_BURST",
    "blast": "TYPE_BLAST",
    "project": "TYPE_PROJECT",
    "resist": "TYPE_RESIST",
    "shield": "TYPE_SHIELD",
    "forcefield": "TYPE_FORCEFIELD",
    "parry": "TYPE_PARRY",
    "reflect": "TYPE_REFLECT",
    "heal": "TYPE_HEAL",
    "buff": "TYPE_BUFF",
    "debuff": "TYPE_DEBUFF",
}
ATTR_IDS = {
    "none": "ATTR_NONE",
    "physical": "ATTR_PHYSICAL",
    "fire": "ATTR_FIRE",
    "ice": "ATTR_ICE",
    "wind": "ATTR_WIND",
    "wood": "ATTR_WOOD",
    "metal": "ATTR_METAL",
    "thunder": "ATTR_THUNDER",
    "earth": "ATTR_EARTH",
    "light": "ATTR_LIGHT",
    "dark": "ATTR_DARK",
    "blood": "ATTR_BLOOD",
    "spiritual": "ATTR_SPIRITUAL",
    "karma": "ATTR_KARMA",
    "space": "ATTR_SPACE",
}
TARGET_TYPES = {"self": "TARGET_SELF", "enemy": "TARGET_ENEMY", "none": "TARGET_NONE"}
SCHOOLS = {
    "basic": "SCHOOL_BASIC",
    "fire": "SCHOOL_FIRE",
    "sword": "SCHOOL_SWORD",
    "blood": "SCHOOL_BLOOD",
    "defense": "SCHOOL_DEFENSE",
    "qi": "SCHOOL_QI",
    "thunder": "SCHOOL_THUNDER",
    "dark": "SCHOOL_DARK",
}
REALMS = {
    "mortal": "MORTAL",
    "refining": "REFINING",
    "foundation": "FOUNDATION",
    "core": "CORE_FORM",
    "nascent": "NASCENT_SOUL",
    "severing": "SEVERING",
    "refinement": "REFINEMENT",
    "unity": "UNITY",
    "great": "GREAT",
    "ascension": "ASCENSION",
}
RARITIES = {
    "common": "RARITY_COMMON",
    "rare": "RARITY_RARE",
    "epic": "RARITY_EPIC",
    "legendary": "RARITY_LEGENDARY",
}
EFFECTS = {
    "none": "CUSTOM_EFFECT_NONE",
    "damage": "CUSTOM_EFFECT_DAMAGE",
    "heal_hp_pct": "CUSTOM_EFFECT_HEAL_HP_PCT",
    "gain_qi_pct": "CUSTOM_EFFECT_GAIN_QI_PCT",
    "bleed": "CUSTOM_EFFECT_BLEED",
    "curse": "CUSTOM_EFFECT_CURSE",
    "enrage": "CUSTOM_EFFECT_ENRAGE",
    "cleanse": "CUSTOM_EFFECT_CLEANSE",
    "cultivation": "CUSTOM_EFFECT_CULTIVATION",
}
SKILL_EFFECTS = {
    "none": "SKILL_EFFECT_NONE",
    "damage": "SKILL_EFFECT_DAMAGE_PCT",
    "heal_hp_pct": "SKILL_EFFECT_HEAL_HP_PCT",
    "gain_qi_pct": "SKILL_EFFECT_GAIN_QI_PCT",
    "bleed": "SKILL_EFFECT_BLEED",
    "curse": "SKILL_EFFECT_CURSE",
    "enrage": "SKILL_EFFECT_ENRAGE",
    "cleanse": "SKILL_EFFECT_CLEANSE",
    "cultivation": "SKILL_EFFECT_CULTIVATION",
}


def clean_log_text(text):
    return ANSI_RE.sub("", text).strip()


def c_string(value):
    value = str(value or "")
    value = value[:240]
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\r", "") + '"'


def clamp_int(value, low, high, default=0):
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        return default
    return max(low, min(high, parsed))


def content_default():
    return {"skills": [], "artifacts": [], "elixirs": []}


def load_custom_content():
    if not CUSTOM_CONTENT_PATH.exists():
        return content_default()
    try:
        data = json.loads(CUSTOM_CONTENT_PATH.read_text(encoding="utf-8"))
    except Exception:
        return content_default()
    if not isinstance(data, dict):
        return content_default()
    return {
        "skills": data.get("skills", []) if isinstance(data.get("skills", []), list) else [],
        "artifacts": data.get("artifacts", []) if isinstance(data.get("artifacts", []), list) else [],
        "elixirs": data.get("elixirs", []) if isinstance(data.get("elixirs", []), list) else [],
    }


def choice(mapping, value, default):
    key = str(value or default)
    return mapping.get(key, mapping[default])


def validate_custom_content(data):
    errors = []
    if not isinstance(data, dict):
        raise ValueError("内容包必须是对象。")
    clean = content_default()
    for section, limit in (("skills", 16), ("artifacts", 12), ("elixirs", 12)):
        items = data.get(section, [])
        if not isinstance(items, list):
            errors.append(f"{section} 必须是数组。")
            items = []
        if len(items) > limit:
            errors.append(f"{section} 最多 {limit} 项。")
        clean[section] = items[:limit]
    if errors:
        raise ValueError("；".join(errors))
    return clean


def skill_effect_initializer(effect_key, value):
    effect = choice(SKILL_EFFECTS, effect_key, "none")
    if effect == "SKILL_EFFECT_NONE":
        return "", 0
    return f", .effects = {{{{{effect}, {value}, 1.0f}}}}, .effect_count = 1", 1


def generate_custom_inc(data):
    data = validate_custom_content(data)
    lines = [
        "// Generated by web_server.py from custom_content.json.",
        "static void Initialize_CustomContent(void)",
        "{",
    ]
    for idx, item in enumerate(data["skills"]):
        if not isinstance(item, dict):
            continue
        name = str(item.get("name") or f"自创功法{idx + 1}")[:32]
        desc = str(item.get("desc") or "<%s 施展自创功法。>")[:180]
        if "%s" not in desc:
            desc = "<%s 施展" + name + "。>"
        skill_id = f"CUSTOM_SKILL_{idx:02d}"
        lines.append(f"    g_skill_database[{skill_id}] = (Skill){{")
        lines.append(f"        .skill_id = {skill_id}, .action_type = {choice(ACTION_TYPES, item.get('action_type'), 'melee')}, .name_chn = {c_string(name)}, .name_eng = {c_string(name)},")
        lines.append(f"        .hotkey = 'U', .cost = {clamp_int(item.get('cost'), 0, 240)}, .rank = {clamp_int(item.get('rank'), 0, 9)}, .type_id = {choice(TYPE_IDS, item.get('type_id'), 'slash')}, .attribute_id = {choice(ATTR_IDS, item.get('attribute'), 'physical')}, .base_power = {clamp_int(item.get('base_power'), 0, 120, 10) / 10.0:.1f}f,")
        lines.append(f"        .effect_strength = {clamp_int(item.get('effect_strength'), 0, 50)}, .effect_chance = {clamp_int(item.get('effect_chance'), 0, 100) / 100.0:.2f}f, .target_type = {choice(TARGET_TYPES, item.get('target'), 'enemy')},")
        lines.append(f"        .prompt_chn = {c_string(desc)}, .prompt_eng = {c_string(desc)},")
        effect_key = item.get('custom_effect')
        effect_value = clamp_int(item.get('custom_value'), 0, 50)
        effect_init, _ = skill_effect_initializer(effect_key, effect_value)
        lines.append(f"        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = {choice(REALMS, item.get('realm'), 'mortal')}, .school_tag = {choice(SCHOOLS, item.get('school'), 'basic')},")
        lines.append(f"        .is_custom = true, .custom_effect = {choice(EFFECTS, effect_key, 'none')}, .custom_value = {effect_value}{effect_init}")
        lines.append("    };")
    for idx, item in enumerate(data["artifacts"]):
        if not isinstance(item, dict):
            continue
        artifact_id = f"CUSTOM_ARTIFACT_{idx:02d}"
        name = str(item.get("name") or f"自创法器{idx + 1}")[:32]
        desc = str(item.get("desc") or "玩家自创法器。")[:180]
        lines.append(f"    g_artifact_database[{artifact_id}] = (Artifact){{")
        lines.append(f"        .id = {artifact_id}, .name_chn = {c_string(name)}, .name_eng = {c_string(name)}, .desc_chn = {c_string(desc)}, .desc_eng = {c_string(desc)}, .rarity = {choice(RARITIES, item.get('rarity'), 'common')},")
        lines.append(f"        .is_custom = true, .max_hp_pct = {clamp_int(item.get('max_hp_pct'), 0, 60)}, .max_qi_pct = {clamp_int(item.get('max_qi_pct'), 0, 60)}, .breakthrough_pct = {clamp_int(item.get('breakthrough_pct'), 0, 25)}, .post_battle_cultivation = {clamp_int(item.get('post_battle_cultivation'), 0, 20)}, .reward_school_bias = {choice(SCHOOLS, item.get('reward_school_bias'), 'basic')}")
        lines.append("    };")
    for idx, item in enumerate(data["elixirs"]):
        if not isinstance(item, dict):
            continue
        elixir_id = f"CUSTOM_ELIXIR_{idx:02d}"
        name = str(item.get("name") or f"自创丹药{idx + 1}")[:32]
        desc = str(item.get("desc") or "玩家自创丹药。")[:180]
        lines.append(f"    g_elixir_database[{elixir_id}] = (Elixir){{")
        lines.append(f"        .id = {elixir_id}, .name_chn = {c_string(name)}, .name_eng = {c_string(name)}, .desc_chn = {c_string(desc)}, .desc_eng = {c_string(desc)}, .rarity = {choice(RARITIES, item.get('rarity'), 'common')},")
        lines.append(f"        .is_custom = true, .heal_hp_pct = {clamp_int(item.get('heal_hp_pct'), 0, 80)}, .gain_qi_pct = {clamp_int(item.get('gain_qi_pct'), 0, 100)}, .clear_negative = {1 if item.get('clear_negative') else 0}, .breakthrough_pct = {clamp_int(item.get('breakthrough_pct'), 0, 30)}, .cultivation_gain = {clamp_int(item.get('cultivation_gain'), 0, 120)}")
        lines.append("    };")
    lines.append("}")
    return "\n".join(lines) + "\n"


def ensure_custom_inc():
    if not CUSTOM_INC_PATH.exists():
        CUSTOM_INC_PATH.write_text(generate_custom_inc(content_default()), encoding="utf-8")


def content_status(error=None):
    data = load_custom_content()
    count = len(data.get("skills", [])) + len(data.get("artifacts", [])) + len(data.get("elixirs", []))
    return {
        "enabled": count > 0,
        "counts": {
            "skills": len(data.get("skills", [])),
            "artifacts": len(data.get("artifacts", [])),
            "elixirs": len(data.get("elixirs", [])),
        },
        "compiled": CUSTOM_INC_PATH.exists(),
        "error": error,
    }


def load_run_records():
    if not RUN_RECORDS_PATH.exists():
        return {"ascensions": [], "memorials": []}
    try:
        data = json.loads(RUN_RECORDS_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {"ascensions": [], "memorials": []}
    if not isinstance(data, dict):
        return {"ascensions": [], "memorials": []}
    return {
        "ascensions": data.get("ascensions", []) if isinstance(data.get("ascensions", []), list) else [],
        "memorials": data.get("memorials", []) if isinstance(data.get("memorials", []), list) else [],
    }


def save_run_records(records):
    records["ascensions"] = sorted(records.get("ascensions", []), key=lambda item: (item.get("age", 999999), item.get("battle_index", 999999)))[:20]
    records["memorials"] = sorted(records.get("memorials", []), key=lambda item: (-item.get("realm_id", 0), item.get("age", 999999)))[:30]
    RUN_RECORDS_PATH.write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding="utf-8")


def record_from_snapshot(snapshot):
    ending = snapshot.get("ending") or {}
    if not ending.get("active"):
        return None
    return {
        "type": ending.get("type", ""),
        "title": ending.get("title", ""),
        "summary": ending.get("summary", ""),
        "age": int(ending.get("age", 0) or 0),
        "realm": ending.get("realm", ""),
        "realm_id": int(ending.get("realm_id", 0) or 0),
        "battle_index": int(ending.get("battle_index", 0) or 0),
        "boss_kills": int(ending.get("boss_kills", 0) or 0),
        "primary_school": ending.get("primary_school", ""),
        "talent_count": int(ending.get("talent_count", 0) or 0),
        "soul_state": ending.get("soul_state", ""),
        "reborn": bool(ending.get("reborn")),
        "created_at": int(time.time()),
    }


def save_custom_content(data):
    clean = validate_custom_content(data)
    inc_text = generate_custom_inc(clean)
    CUSTOM_CONTENT_PATH.write_text(json.dumps(clean, ensure_ascii=False, indent=2), encoding="utf-8")
    CUSTOM_INC_PATH.write_text(inc_text, encoding="utf-8")
    return clean


class QIRPGProcess:
    def __init__(self):
        self.lock = threading.Lock()
        self.process = None
        self.state = None
        self.logs = []
        self.last_error = None
        self.version = 0
        self.recorded_ending_key = None
        self.start_process()

    def ensure_binary(self):
        ensure_custom_inc()
        binary = ROOT / "qi_rpg"
        sources = [ROOT / "QI.c", ROOT / "QI.h", *ROOT.glob("rpg_*.inc")]
        newest_source = max(path.stat().st_mtime for path in sources)
        if binary.exists() and binary.stat().st_mtime >= newest_source:
            return
        result = subprocess.run(
            ["gcc", "-Wall", "-Wextra", "-o", str(binary), "QI.c", "-lm"],
            cwd=ROOT,
            text=True,
            capture_output=True,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "C build failed")

    def start_process(self):
        self.stop_process()
        self.ensure_binary()
        with self.lock:
            self.state = None
            self.logs = []
            self.last_error = None
            self.recorded_ending_key = None
            self.version += 1
        self.process = subprocess.Popen(
            [str(ROOT / "qi_rpg"), "--ui-json"],
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        threading.Thread(target=self._read_loop, daemon=True).start()

    def stop_process(self):
        if self.process and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                self.process.kill()
        self.process = None

    def _append_log(self, line):
        clean = clean_log_text(line)
        if not clean:
            return
        with self.lock:
            self.logs.append(clean)
            self.logs = self.logs[-80:]
            if self.state is not None:
                bridge_logs = self.state.setdefault("bridge_logs", [])
                bridge_logs.append(clean)
                self.state["bridge_logs"] = bridge_logs[-30:]

    def _read_loop(self):
        try:
            for line in self.process.stdout:
                if line.startswith(STATE_PREFIX):
                    try:
                        snapshot = json.loads(line[len(STATE_PREFIX):])
                        snapshot["logs"] = [clean_log_text(item) for item in snapshot.get("logs", []) if clean_log_text(item)]
                        snapshot["content_status"] = content_status()
                        with self.lock:
                            snapshot["bridge_logs"] = self.logs[-30:]
                            snapshot["server_time"] = time.time()
                            self.state = snapshot
                            self._maybe_record_ending_locked(snapshot)
                            self.last_error = None
                            self.version += 1
                    except json.JSONDecodeError as exc:
                        self._append_log(f"JSON parse error: {exc}")
                else:
                    self._append_log(line)
        except Exception as exc:
            with self.lock:
                self.last_error = str(exc)

    def _maybe_record_ending_locked(self, snapshot):
        record = record_from_snapshot(snapshot)
        if not record:
            return
        key = (record["type"], record["age"], record["realm_id"], record["battle_index"], record["boss_kills"])
        if self.recorded_ending_key == key:
            return
        records = load_run_records()
        bucket = "ascensions" if record["type"] == "ascension" else "memorials"
        records[bucket].append(record)
        save_run_records(records)
        self.recorded_ending_key = key

    def command(self, text):
        if self.process is None or self.process.poll() is not None:
            self.start_process()
        with self.lock:
            previous_version = self.version
        try:
            self.process.stdin.write(text + "\n")
            self.process.stdin.flush()
        except BrokenPipeError:
            self.start_process()
            with self.lock:
                previous_version = self.version
            self.process.stdin.write(text + "\n")
            self.process.stdin.flush()
        return self.get_state(wait=True, after_version=previous_version)

    def reset(self):
        self.start_process()
        return self.get_state(wait=True)

    def get_state(self, wait=False, after_version=None):
        deadline = time.time() + (2.0 if wait else 0.0)
        while wait and time.time() < deadline:
            with self.lock:
                if self.state is not None and (after_version is None or self.version > after_version):
                    return dict(self.state)
            time.sleep(0.02)
        with self.lock:
            if self.state is not None:
                return dict(self.state)
            return {"phase": "run_start", "logs": [], "bridge_logs": self.logs[-30:], "error": self.last_error}


GAME = QIRPGProcess()


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        return

    def _send_json(self, payload, status=200):
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _read_json(self):
        length = int(self.headers.get("Content-Length", "0") or 0)
        if length <= 0:
            return {}
        raw = self.rfile.read(length).decode("utf-8")
        return json.loads(raw or "{}")

    def do_GET(self):
        if self.path == "/api/state":
            self._send_json(GAME.get_state(wait=True))
            return
        if self.path == "/api/records":
            self._send_json(load_run_records())
            return
        if self.path == "/api/content":
            self._send_json({"content": load_custom_content(), "status": content_status()})
            return
        path = self.path.split("?", 1)[0]
        if path == "/":
            path = "/index.html"
        target = (UI_DIR / path.lstrip("/")).resolve()
        if UI_DIR not in target.parents and target != UI_DIR:
            self.send_error(403)
            return
        if not target.exists() or not target.is_file():
            self.send_error(404)
            return
        content_type = "text/html; charset=utf-8"
        if target.suffix == ".css":
            content_type = "text/css; charset=utf-8"
        elif target.suffix == ".js":
            content_type = "application/javascript; charset=utf-8"
        data = target.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        try:
            body = self._read_json()
            if self.path == "/api/start":
                payload = GAME.command("start_run")
            elif self.path == "/api/action":
                if "slot" in body:
                    payload = GAME.command(f"choose_action slot={int(body['slot'])}")
                else:
                    payload = GAME.command(f"choose_action action_type={int(body.get('action_type', 0))}")
            elif self.path == "/api/elixir":
                payload = GAME.command(f"use_elixir slot={int(body.get('slot', -1))}")
            elif self.path == "/api/reward":
                payload = GAME.command(f"choose_reward index={int(body.get('index', 0))}")
            elif self.path == "/api/replacement":
                payload = GAME.command(f"choose_replacement slot={int(body.get('slot', -1))}")
            elif self.path == "/api/artifact/upgrade":
                payload = GAME.command(f"upgrade_artifact slot={int(body.get('slot', -1))}")
            elif self.path == "/api/elixir/brew":
                payload = GAME.command(f"brew_elixir recipe={int(body.get('recipe', -1))}")
            elif self.path == "/api/preparation/skip":
                payload = GAME.command("skip_preparation")
            elif self.path == "/api/equip_skill":
                payload = GAME.command(f"equip_skill id={int(body.get('id', -1))}")
            elif self.path == "/api/skill/refine":
                payload = GAME.command(f"refine_skill id={int(body.get('id', -1))}")
            elif self.path == "/api/breakthrough":
                payload = GAME.command("attempt_breakthrough")
            elif self.path == "/api/heart_demon":
                payload = GAME.command(f"choose_heart_demon mode={int(body.get('mode', 0))}")
            elif self.path == "/api/skip_breakthrough":
                payload = GAME.command("skip_breakthrough")
            elif self.path == "/api/talent":
                payload = GAME.command(f"choose_talent index={int(body.get('index', 0))}")
            elif self.path == "/api/near_death":
                payload = GAME.command(f"choose_near_death index={int(body.get('index', 0))}")
            elif self.path == "/api/route":
                payload = GAME.command(f"choose_route index={int(body.get('index', 0))}")
            elif self.path == "/api/continue":
                payload = GAME.command("continue")
            elif self.path == "/api/reset":
                payload = GAME.command("reset_run")
            elif self.path == "/api/content/save":
                content = save_custom_content(body.get("content", body))
                GAME.start_process()
                payload = {"content": content, "status": content_status(), "state": GAME.get_state(wait=True)}
            elif self.path == "/api/content/reset":
                content = save_custom_content(content_default())
                GAME.start_process()
                payload = {"content": content, "status": content_status(), "state": GAME.get_state(wait=True)}
            elif self.path == "/api/records/reset":
                save_run_records({"ascensions": [], "memorials": []})
                payload = load_run_records()
            else:
                self.send_error(404)
                return
            self._send_json(payload)
        except Exception as exc:
            self._send_json({"error": str(exc)}, status=500)


def main():
    host = os.environ.get("QI_RPG_HOST", "localhost")
    port = int(os.environ.get("QI_RPG_PORT", "8000"))
    server = ThreadingHTTPServer((host, port), Handler)
    print(f"QI RPG Web UI: http://{host}:{port}", flush=True)
    try:
        server.serve_forever()
    finally:
        GAME.stop_process()


if __name__ == "__main__":
    main()
