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
LLM_API_URL = "http://localhost:11434/api/chat"
GAME_EXECUTABLE = "./QI"
LLM_MODEL_NAME = "llama3:8b"

# --- 全局状态变量 ---
executor = ThreadPoolExecutor(max_workers=1)
conversation_history = []
marshal_manual = None
llm_mode = None


def query_llm_per_turn(user_prompt: str) -> dict:
    """“事无巨细”模式的查询函数"""
    global conversation_history
    conversation_history.append({"role": "user", "content": user_prompt})

    # 恢复日志写入
    with open("llm_prompts.log", "a", encoding="utf-8") as f:
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"--- Prompt at {timestamp} (Per-Turn Mode) ---\n")
        f.write(json.dumps(conversation_history, indent=2, ensure_ascii=False))
        f.write("\n\n")

    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Thinking with full context (Per-Turn Mode)...{COLOR_RESET}")
    try:
        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME, "messages": conversation_history,
                "stream": False, "format": "json", "options": {"temperature": 0.2}
            },
            timeout=120
        )
        response.raise_for_status()
        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")
        conversation_history.append({"role": "assistant", "content": llm_message_content})
        
        # 将LLM的响应也记录下来
        with open("llm_prompts.log", "a", encoding="utf-8") as f:
            f.write("--- LLM Response ---\n")
            f.write(f"Raw Response: {llm_message_content}\n\n")

        decision_json = json.loads(llm_message_content)
        return {
            "action_id": str(decision_json.get("action_id", "0")),
            "reasoning": decision_json.get("reasoning", "No reasoning.")
        }
    except Exception as e:
        with open("llm_prompts.log", "a", encoding="utf-8") as f:
            f.write(f"--- ERROR during LLM Query ---\nError: {e}\n\n")
        print(f"--- [Bridge] ERROR in Per-Turn Query: {e}", file=sys.stderr)
        return {"action_id": "0", "reasoning": "Error."}


def query_llm_strategy(prompt: str) -> str:
    """“将帅分级”模式的查询函数"""
    global marshal_manual
    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Sending battlefield report to Grand Marshal...{COLOR_RESET}")
    try:
        messages = [
            {"role": "system", "content": marshal_manual},
            {"role": "user", "content": prompt}
        ]

        with open("llm_prompts.log", "a", encoding="utf-8") as f:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            f.write(f"--- Prompt at {timestamp} (Marshal Mode) ---\n")
            f.write(json.dumps(messages, indent=2, ensure_ascii=False))
            f.write("\n")

        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME, "messages": messages,
                "stream": False, "format": "json", "options": {"temperature": 0.3}
            },
            timeout=120
        )
        response.raise_for_status()
        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")
        decision_json = json.loads(llm_message_content)
        general_id = str(decision_json.get("next_general_id", "0"))
        reasoning = decision_json.get("reasoning", "No reasoning.")

        print(f"{COLOR_AI_DECISION}--- [Bridge] Grand Marshal's Decision: Switch to General ID {general_id}{COLOR_RESET}")
        print(f"{COLOR_AI_DECISION}--- [Bridge] Reasoning: {reasoning}{COLOR_RESET}")
        
        with open("llm_prompts.log", "a", encoding="utf-8") as f:
            f.write("--- LLM Response and Parsed Decision ---\n")
            f.write(f"Raw Response: {llm_message_content}\n")
            f.write(f"Parsed Decision ID: {general_id}\n")
            f.write(f"Parsed Reasoning: {reasoning}\n\n")

        return general_id
    except Exception as e:
        with open("llm_prompts.log", "a", encoding="utf-8") as f:
            f.write(f"--- ERROR during LLM Query ---\nError: {e}\n\n")
        print(f"--- [Bridge] ERROR in Strategy Query: {e}", file=sys.stderr)
        return "0"


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