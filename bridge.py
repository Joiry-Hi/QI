import json
import re
import datetime
import subprocess
import requests
import sys
from concurrent.futures import ThreadPoolExecutor

# --- 颜色常量定义 ---
COLOR_AI_THINKING = "\033[96m"
COLOR_AI_DECISION = "\033[93m"
COLOR_RESET = "\033[0m"

# --- 配置 ---
# 注意：我们将使用 chat 接口，因为它对两种模式都更健壮
LLM_API_URL = "http://localhost:11434/api/chat"
GAME_EXECUTABLE = "./QI"

# 原来的配置:
#LLM_MODEL_NAME = "mistral"

# 更换为Llama 3 (推荐):
LLM_MODEL_NAME = "llama3:8b"

# 或者更换为Gemma:
# LLM_MODEL_NAME = "gemma:7b"

# 或者更换为Phi-3 Mini:
# LLM_MODEL_NAME = "phi3:mini"

# --- 全局状态变量 ---
executor = ThreadPoolExecutor(max_workers=1)
conversation_history = []
# 新增：LLM工作模式 (None, "PER_TURN", "MARSHAL")
llm_mode = None


def query_llm_per_turn(user_prompt: str) -> dict:
    # 这是“事无巨细”模式的查询函数
    global conversation_history
    conversation_history.append({"role": "user", "content": user_prompt})

    print(
        f"\n{COLOR_AI_THINKING}--- [Bridge] Thinking with full context (Per-Turn Mode)...{COLOR_RESET}"
    )

    try:
        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME,
                "messages": conversation_history,
                "stream": False,
                "format": "json",
                "options": {"temperature": 0.2},
            },
            timeout=120,
        )
        response.raise_for_status()

        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")
        conversation_history.append(
            {"role": "assistant", "content": llm_message_content}
        )

        decision_json = json.loads(llm_message_content)
        return {
            "action_id": str(decision_json.get("action_id", "0")),
            "reasoning": decision_json.get("reasoning", "No reasoning."),
        }
    except Exception as e:
        print(f"--- [Bridge] ERROR in Per-Turn Query: {e}", file=sys.stderr)
        return {"action_id": "0", "reasoning": "Error."}


def query_llm_strategy(prompt: str) -> str:
    # 这是“将帅分级”模式的查询函数
    print(
        f"\n{COLOR_AI_THINKING}--- [Bridge] Sending battlefield report to Grand Marshal...{COLOR_RESET}"
    )

    try:
        # 战略决策不需要历史记录
        messages = [{"role": "user", "content": prompt}]
        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME,
                "messages": messages,
                "stream": False,
                "format": "json",
                "options": {"temperature": 0.3},
            },
            timeout=120,
        )
        response.raise_for_status()

        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")

        decision_json = json.loads(llm_message_content)
        general_id = str(decision_json.get("next_general_id", "0"))
        reasoning = decision_json.get("reasoning", "No reasoning.")

        print(
            f"{COLOR_AI_DECISION}--- [Bridge] Grand Marshal's Decision: Switch to General ID {general_id}{COLOR_RESET}"
        )
        print(f"{COLOR_AI_DECISION}--- [Bridge] Reasoning: {reasoning}{COLOR_RESET}")
        return general_id
    except Exception as e:
        print(f"--- [Bridge] ERROR in Strategy Query: {e}", file=sys.stderr)
        return "0"


def main():
    global llm_mode, conversation_history
    print(f"--- [Bridge] Starting game process: {GAME_EXECUTABLE} ---")
    
    game_process = subprocess.Popen(
        GAME_EXECUTABLE,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding='utf-8',
        bufsize=1
    )

    prompt_buffer = []
    is_buffering_prompt = False
    llm_future = None
    
    command_prefix = "##CMD##:"
    prompt_end_trigger = "END_OF_PROMPT"

    # 定义更精确的prompt触发器
    per_turn_triggers = ["You are a master strategist", "== Turn Update"]
    marshal_trigger = "Grand Marshal, this is the battlefield report"

    while game_process.poll() is None:
        try:
            line = game_process.stdout.readline()
            if not line: break

            if line.startswith(command_prefix):
                # ... (命令处理部分的代码完全不变) ...
                command = line.strip().split(':', 1)[1]
                
                if command == "NEW_GAME_START":
                    print("\n--- [Bridge] New game detected. Awaiting AI mode confirmation... ---")
                    llm_mode = None
                    conversation_history = []
                    if llm_future: llm_future.cancel()
                    llm_future = None
                
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
                            game_process.stdin.write(decision['action_id'] + "\n")
                            game_process.stdin.flush()
                            llm_future = None
                    else:
                        user_input = input()
                        game_process.stdin.write(user_input + "\n")
                        game_process.stdin.flush()

                continue
            
            if any(trigger in line for trigger in per_turn_triggers) or marshal_trigger in line:
                is_buffering_prompt = True
            
            if is_buffering_prompt:
                prompt_buffer.append(line)
            else:
                print(line, end='')

            if prompt_end_trigger in line and is_buffering_prompt:
                full_prompt = "".join(prompt_buffer)
                prompt_buffer = []
                is_buffering_prompt = False
                
                # --- 模式自动检测 ---
                if llm_mode is None:
                    # 使用精确的触发器来检测模式
                    if any(trigger in full_prompt for trigger in per_turn_triggers):
                        llm_mode = "PER_TURN"
                        print(f"--- [Bridge] AI Mode confirmed: {llm_mode} ---")
                        # 只有在创世prompt时才加入历史
                        if "You are a master strategist" in full_prompt:
                            conversation_history.append({"role": "user", "content": full_prompt})
                    elif marshal_trigger in full_prompt:
                        llm_mode = "MARSHAL"
                        print(f"--- [Bridge] AI Mode confirmed: {llm_mode} ---")
                
                # --- 根据模式执行任务 ---
                if llm_mode == "PER_TURN":
                    # 在PER_TURN模式下，创世prompt不立即查询
                    if "== Turn Update" in full_prompt:
                        llm_future = executor.submit(query_llm_per_turn, full_prompt)
                elif llm_mode == "MARSHAL":
                    new_general_id = query_llm_strategy(full_prompt)
                    game_process.stdin.write(new_general_id + "\n")
                    game_process.stdin.flush()

        except (BrokenPipeError, KeyboardInterrupt): break
        except Exception as e:
            print(f"\n--- [Bridge] An unexpected error occurred: {e} ---", file=sys.stderr)
            break
    
    executor.shutdown()
    game_process.terminate()
    print("--- [Bridge] Game process terminated. ---")

if __name__ == "__main__":
    main()
