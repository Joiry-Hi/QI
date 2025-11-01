import json
import re
import datetime
import subprocess
import requests
import sys
from concurrent.futures import ThreadPoolExecutor
import logging # <-- 新增导入
from logging.handlers import RotatingFileHandler # <-- 新增导入
import os # <-- 新增：用于读取环境变量
import google.generativeai as genai # <-- 新增：导入Gemini库

# --- 1. 设置一个专业的、可轮换的日志记录器 ---
log_file = "llm_prompts.log"
logger = logging.getLogger("QiGameAILogger")
logger.setLevel(logging.INFO)

# 创建一个“旋转”文件处理器：当文件达到5MB时，会自动轮换，最多保留5个备份
# 这样总日志大小会稳定在 5MB * (1 + 5) = 30MB 左右
handler = RotatingFileHandler(log_file, maxBytes=5*1024*1024, backupCount=5, encoding='utf-8')

# 定义日志的格式：时间 - 日志级别 - 消息
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)

# 将处理器添加到我们的日志记录器中
logger.addHandler(handler)

# --- 1. 新的LLM提供商配置 ---
LLM_PROVIDER = "CLOUD"  # 在这里切换: "OLLAMA" 或 "CLOUD"

# OLLAMA的配置
OLLAMA_API_URL = "http://localhost:11434/api/chat"
OLLAMA_MODEL_NAME = "llama3:8b" # 或者您想用的其他本地模型

# CLOUD (Gemini) 的配置
CLOUD_LLM_MODEL_NAME = "gemini-1.5-flash-latest" # 性价比极高的最新模型

# --- 2. 安全地配置云服务API密钥 ---
if LLM_PROVIDER == "CLOUD":
    try:
        api_key = os.getenv("GOOGLE_API_KEY")
        if not api_key:
            raise ValueError("环境变量 GOOGLE_API_KEY 未设置！")
        genai.configure(api_key=api_key)
        print("--- [Bridge] Google Gemini API configured successfully. ---")
    except Exception as e:
        print(f"--- [Bridge] FATAL ERROR: Could not configure Gemini API: {e}", file=sys.stderr)
        sys.exit(1) # 如果云服务配置失败，直接退出

# --- 颜色常量定义 ---
COLOR_AI_THINKING = "\033[96m"
COLOR_AI_DECISION = "\033[93m"
COLOR_RESET = "\033[0m"

# --- 配置 ---
GAME_EXECUTABLE = "./QI"

# --- 全局状态变量 ---
executor = ThreadPoolExecutor(max_workers=1)
conversation_history = []
marshal_manual = None
llm_mode = None

# “政委”的知识库 (The Commissar's Roster)
# ！！！与C语言的 switch 语句严格同步 ！！！
GENERAL_ROSTER = {
    "0": ["Disruptor", "Saboteur"],    # case 0
    "1": ["Berserker", "Vanguard"],    # case 1
    "2": ["Turtle", "Iron Wall"],      # case 2
    "3": ["Ascetic", "Monk"],          # case 3
    "4": ["Gambler", "Executioner"],   # case 4
    # 我们可以为 default case 也创建一个条目
    "5": ["Random", "Fool"]            # 假设我们用ID 5代表随机
}
# 我们还需要一个反向查找表，以便从名字快速找到ID
REVERSE_ROSTER = {name.lower(): id for id, names in GENERAL_ROSTER.items() for name in names}

# “政委2.0”的上下文分析工具
ACTION_KEYWORDS = ["switch to", "switching to", "deploy", "deploying", "change to", "use"]

def query_llm_per_turn(user_prompt: str) -> dict:
    """“事无巨细”模式的查询函数"""
    global conversation_history
    conversation_history.append({"role": "user", "content": user_prompt})

    # 构造要记录的多行日志内容
    log_content = (
        f"--- Prompt (Per-Turn Mode) ---\n"
        f"{json.dumps(conversation_history, indent=2, ensure_ascii=False)}"
    )
    logger.info(log_content)

    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Thinking with full context (Per-Turn Mode)...{COLOR_RESET}")
    try:
        response = requests.post(
            OLLAMA_API_URL,
            json={
                "model": OLLAMA_MODEL_NAME, "messages": conversation_history,
                "stream": False, "format": "json", "options": {"temperature": 0.2}
            },
            timeout=120
        )
        response.raise_for_status()
        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")
        conversation_history.append({"role": "assistant", "content": llm_message_content})
        
        log_content_resp = (
            f"--- LLM Response ---\n"
            f"Raw Response: {llm_message_content}"
        )
        logger.info(log_content_resp)

        decision_json = json.loads(llm_message_content)
        return {
            "action_id": str(decision_json.get("action_id", "0")),
            "reasoning": decision_json.get("reasoning", "No reasoning.")
        }
    except Exception as e:
        logger.error(f"--- ERROR during LLM Per-Turn Query ---\nError: {e}")
        print(f"--- [Bridge] ERROR in Per-Turn Query: {e}", file=sys.stderr)
        return {"action_id": "0", "reasoning": "Error."}


def query_cloud_llm_strategy(prompt: str) -> str:
    """使用Google Gemini API进行战略决策的查询函数"""
    global marshal_manual
    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Sending report to CLOUD Grand Marshal (Gemini)...{COLOR_RESET}")
    try:
        # 初始化Gemini模型
        model = genai.GenerativeModel(CLOUD_LLM_MODEL_NAME)
        
        messages = [
            {"role": "user", "parts": [marshal_manual]}, # Gemini的System Prompt用user角色的第一条消息模拟
            {"role": "model", "parts": ["Understood. I am the Grand Marshal. I will analyze the reports and issue commands in the required JSON format."]}, # 确认指令
            {"role": "user", "parts": [prompt]}
        ]

        # 强制Gemini输出JSON
        generation_config = genai.types.GenerationConfig(
            response_mime_type="application/json",
            temperature=0.3
        )

        response = model.generate_content(messages, generation_config=generation_config)
        
        # 记录和解析逻辑与Ollama版本类似
        llm_message_content = response.text
        decision_json = json.loads(llm_message_content)
        parsed_general_id = str(decision_json.get("next_general_id", "0"))
        reasoning = decision_json.get("reasoning", "No reasoning provided.")
        
        final_general_id = verify_and_correct_decision(parsed_general_id, reasoning)

        # ... (打印和记录日志的代码，可以复用或略作修改) ...
        print(f"{COLOR_AI_DECISION}--- [Bridge] Gemini Marshal's Decision: General ID {final_general_id}{COLOR_RESET}")
        print(f"{COLOR_AI_DECISION}--- [Bridge] Reasoning: {reasoning}{COLOR_RESET}")
        
        # ... (日志记录) ...
        log_content_resp = (
            f"--- LLM Response and Parsed Decision ---\n"
            f"Raw Response: {llm_message_content}\n"
            f"Parsed ID from JSON: {parsed_general_id}\n"
            f"Final Corrected ID: {final_general_id}\n"
            f"Parsed Reasoning: {reasoning}"
        )
        logger.info(log_content_resp)

        return final_general_id
    except Exception as e:
        # ... (异常处理和日志记录) ...
        print(f"--- [Bridge] ERROR in CLOUD Strategy Query: {e}", file=sys.stderr)
        return "0"


def query_ollama_llm_strategy(prompt: str) -> str:
    """“将帅分级”模式的查询函数"""
    global marshal_manual
    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Sending battlefield report to Grand Marshal...{COLOR_RESET}")
    try:
        messages = [
            {"role": "system", "content": marshal_manual},
            {"role": "user", "content": prompt}
        ]

        log_content = (
            f"--- Prompt (Marshal Mode) ---\n"
            f"{json.dumps(messages, indent=2, ensure_ascii=False)}"
        )
        logger.info(log_content)

        response = requests.post(
            OLLAMA_API_URL,
            json={
                "model": OLLAMA_MODEL_NAME, "messages": messages,
                "stream": False, "format": "json", "options": {"temperature": 0.3}
            },
            timeout=120
        )
        response.raise_for_status()
        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")

        decision_json = json.loads(llm_message_content)
        
        # 初始解析
        parsed_general_id = str(decision_json.get("next_general_id", "0"))
        reasoning = decision_json.get("reasoning", "No reasoning.")

        #         部署“政委”进行审查和纠正
        final_general_id = verify_and_correct_decision(parsed_general_id, reasoning)

        # 在终端打印最终的、经过纠正的决策
        print(f"{COLOR_AI_DECISION}--- [Bridge] Grand Marshal's Decision: Switch to General ID {final_general_id}{COLOR_RESET}")
        print(f"{COLOR_AI_DECISION}--- [Bridge] Reasoning: {reasoning}{COLOR_RESET}")
        
        log_content_resp = (
            f"--- LLM Response and Parsed Decision ---\n"
            f"Raw Response: {llm_message_content}\n"
            f"Parsed ID from JSON: {parsed_general_id}\n"
            f"Final Corrected ID: {final_general_id}\n"
            f"Parsed Reasoning: {reasoning}"
        )
        logger.info(log_content_resp)

        return final_general_id # <-- 返回最终纠正后的ID

    except Exception as e:
        logger.error(f"--- ERROR during LLM Strategy Query ---\nError: {e}")
        print(f"--- [Bridge] ERROR in Strategy Query: {e}", file=sys.stderr)
        return "0"
    
    
def query_llm_strategy(prompt: str) -> str:
    """
    战略决策的总调度函数。根据全局配置选择LLM提供商。
    """
    if LLM_PROVIDER == "OLLAMA":
        return query_ollama_llm_strategy(prompt)
    elif LLM_PROVIDER == "CLOUD":
        return query_cloud_llm_strategy(prompt)
    else:
        print(f"--- [Bridge] ERROR: Invalid LLM_PROVIDER '{LLM_PROVIDER}' configured.", file=sys.stderr)
        return "0" # 返回一个安全的默认值


def verify_and_correct_decision(parsed_id: str, reasoning: str) -> str:
    """
    “政委 3.0”审查函数：通过行动关键词和否定词检测，实现更精准的意图识别。
    """
    reasoning_lower = reasoning.lower()
    
    # 否定性上下文关键词
    NEGATIVE_KEYWORDS = ["current", "our current", "is not", "has not"]
    
    # 行动关键词 (保持不变)
    ACTION_KEYWORDS = ["switch to", "switching to", "deploy", "deploying", "change to", "use"]

    mentioned_generals = {}
    for name, id in REVERSE_ROSTER.items():
        if name in reasoning_lower:
            mentioned_generals[name] = id
    
    if not mentioned_generals:
        return parsed_id

    # 如果只提到一个，逻辑不变 (除非它在否定性上下文中)
    if len(mentioned_generals) == 1:
        name = list(mentioned_generals.keys())[0]
        # 检查这个唯一的将军是否在否定性上下文中被提及
        for neg_word in NEGATIVE_KEYWORDS:
            if f"{neg_word} {name}" in reasoning_lower or f"{name} strategy is not" in reasoning_lower:
                # 如果是，说明这不是意图，相信JSON
                return parsed_id
        
        # 如果不是否定性上下文，则这就是意图
        intended_id = mentioned_generals[name]
        if intended_id != parsed_id:
            print(f"{COLOR_AI_DECISION}--- [BRIDGE] INCONSISTENCY DETECTED! Reasoning mentioned '{name.capitalize()}' (ID {intended_id}) "
                  f"but JSON gave ID {parsed_id}. CORRECTING to {intended_id}.{COLOR_RESET}")
            return intended_id
        return parsed_id

    # 如果提到了多个将军，启动高级上下文分析
    if len(mentioned_generals) > 1:
        intended_generals = {}
        
        # 1. 긍정적 의도 찾기 (寻找积极意图)
        for keyword in ACTION_KEYWORDS:
            for name, id in mentioned_generals.items():
                if f"{keyword} {name}" in reasoning_lower:
                    intended_generals[name] = id
        
        # 2. 부정적 맥락 제거하기 (排除否定性上下文)
        generals_to_remove = set()
        for name in mentioned_generals:
            for neg_word in NEGATIVE_KEYWORDS:
                if f"{neg_word} {name}" in reasoning_lower or f"{name} strategy has not" in reasoning_lower:
                    generals_to_remove.add(name)

        # 从所有提及的将军中，移除那些在否定性上下文中出现的
        final_candidates = {name: id for name, id in mentioned_generals.items() if name not in generals_to_remove}
        
        # 将积极意图的将军加入最终候选（即使它也被否定提及，行动词优先级更高）
        final_candidates.update(intended_generals)

        # 3. 最终决策
        if len(final_candidates) == 1:
            intended_name = list(final_candidates.keys())[0]
            intended_id = list(final_candidates.values())[0]
            if intended_id != parsed_id:
                print(f"{COLOR_AI_DECISION}--- [BRIDGE] INCONSISTENCY DETECTED! Reasoning mentioned '{intended_name.capitalize()}' (ID {intended_id}) "
                      f"but JSON gave ID {parsed_id}. CORRECTING to {intended_id}.{COLOR_RESET}")
                return intended_id
            return parsed_id

    # 如果所有逻辑都走完，仍然无法确定唯一意图，则最后相信JSON
    return parsed_id


def main():
    # ... (main函数保持不变) ...
    global llm_mode, conversation_history, marshal_manual
    print(f"--- [Bridge] Starting game process: {GAME_EXECUTABLE} ---")

    game_process = subprocess.Popen(
        GAME_EXECUTABLE, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, encoding="utf-8", bufsize=1
    )

    prompt_buffer = []
    is_buffering_prompt = False
    llm_future = None
    command_prefix = "##CMD##:"
    prompt_end_trigger = "END_OF_PROMPT"

    while game_process.poll() is None:
        try:
            line = game_process.stdout.readline()
            if not line: break

            if line.startswith(command_prefix):
                command = line.strip().split(":", 1)[1]
                if command == "NEW_GAME_START":
                    print("\n--- [Bridge] New game detected. Awaiting AI mode confirmation... ---")
                    llm_mode, marshal_manual, conversation_history = None, None, []
                    if llm_future: llm_future.cancel()
                    llm_future = None
                elif command == "START_PROMPT":
                    is_buffering_prompt = True
                    prompt_buffer = []
                elif command == "INPUT_REQUIRED":
                    if llm_mode == "PER_TURN":
                        print(f"{COLOR_AI_THINKING}--- [Bridge] LLM (Per-Turn) is thinking...{COLOR_RESET}")
                        user_input = input()
                        game_process.stdin.write(user_input + "\n")
                        game_process.stdin.flush()
                        if llm_future:
                            print(f"{COLOR_AI_THINKING}--- [Bridge] Retrieving decision...{COLOR_RESET}")
                            decision = llm_future.result()
                            print(f"{COLOR_AI_DECISION}--- [Bridge] LLM Decided Action ID: {decision['action_id']}{COLOR_RESET}")
                            print(f"{COLOR_AI_DECISION}--- [Bridge] LLM Reasoning: {decision['reasoning']}{COLOR_RESET}")
                            game_process.stdin.write(decision["action_id"] + "\n")
                            game_process.stdin.flush()
                            llm_future = None
                    else:
                        user_input = input()
                        game_process.stdin.write(user_input + "\n")
                        game_process.stdin.flush()
                continue

            if is_buffering_prompt:
                prompt_buffer.append(line)
                if prompt_end_trigger in line:
                    full_prompt = "".join(prompt_buffer)
                    is_buffering_prompt = False
                    if llm_mode is None:
                        if "Grand Marshal's Principles of War" in full_prompt:
                            llm_mode = "MARSHAL"
                            marshal_manual = full_prompt
                            print(f"--- [Bridge] AI Mode: {llm_mode} (Manual Stored). Game starts. ---")
                        elif "You are a master strategist" in full_prompt:
                            llm_mode = "PER_TURN"
                            print(f"--- [Bridge] AI Mode: {llm_mode}. Game starts. ---")
                            conversation_history.append({"role": "user", "content": full_prompt})
                    elif llm_mode == "PER_TURN":
                        if "== Turn Update" in full_prompt:
                            llm_future = executor.submit(query_llm_per_turn, full_prompt)
                    elif llm_mode == "MARSHAL":
                        new_general_id = query_llm_strategy(full_prompt)
                        game_process.stdin.write(new_general_id + "\n")
                        game_process.stdin.flush()
            else:
                print(line, end="")

        except (BrokenPipeError, KeyboardInterrupt):
            print("\n--- [Bridge] Process interrupted. Shutting down. ---")
            break
        except Exception as e:
            print(f"\n--- [Bridge] An unexpected error occurred: {e} ---", file=sys.stderr)
            break

    executor.shutdown()
    game_process.terminate()
    print("--- [Bridge] Game process terminated. ---")

if __name__ == "__main__":
    main()