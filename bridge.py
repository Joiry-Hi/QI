import json
import re
import datetime
import subprocess
import requests
import sys
from concurrent.futures import ThreadPoolExecutor # <-- 新增导入

# --- 颜色常量定义 ---
COLOR_AI_THINKING = "\033[96m"  # 亮青色 (Light Cyan)
COLOR_AI_DECISION = "\033[93m"  # 亮黄色 (Light Yellow)
COLOR_RESET = "\033[0m"         # 重置所有颜色和样式

# --- 初始化线程池 ---
# 我们只需要一个后台线程来处理LLM请求
executor = ThreadPoolExecutor(max_workers=1)

# --- 配置 ---
# 你的本地Ollama API地址 (使用新的chat接口)
LLM_API_URL = "http://localhost:11434/api/chat"
# 你希望使用的本地模型名称
LLM_MODEL_NAME = "mistral"
# 你的C语言游戏可执行文件的路径
GAME_EXECUTABLE = "./QI"

# --- BLUEPRINT v2.0: 会话历史记录 ---
conversation_history = []


def query_llm(user_prompt: str) -> str:
    """
    向本地Ollama LLM发送包含完整对话历史的请求，并解析返回的JSON。
    """
    global conversation_history

    # 将当前回合的用户提示词追加到历史记录
    conversation_history.append({"role": "user", "content": user_prompt})

    # 将prompt写入日志文件
    with open("llm_prompts.log", "a", encoding="utf-8") as f:
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"--- Prompt at {timestamp} ---\n")
        # 记录完整的请求历史
        f.write(json.dumps(conversation_history, indent=2, ensure_ascii=False))
        f.write("\n\n")

    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Thinking with full context...{COLOR_RESET}")

    try:
        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME,
                "messages": conversation_history,  # <-- 发送完整的历史记录
                "stream": False,
                "format": "json",  # <-- 强制Ollama返回JSON格式
                "options": {"temperature": 0.2},
            },
            timeout=120,
        )
        response.raise_for_status()

        # 解析返回的JSON字符串
        response_data = response.json()
        llm_message_content = response_data.get("message", {}).get("content", "")

        # 将LLM的回复也追加到历史记录中，形成闭环
        conversation_history.append(
            {"role": "assistant", "content": llm_message_content}
        )

        # 解析LLM回复的JSON内容
        try:
            decision_json = json.loads(llm_message_content)
            action_id = str(decision_json.get("action_id", "0"))
            reasoning = decision_json.get("reasoning", "No reasoning provided.")

            return {"action_id": action_id, "reasoning": reasoning}

        except json.JSONDecodeError:
            print(
                f"--- [Bridge] ERROR: LLM returned malformed JSON: '{llm_message_content}'",
                file=sys.stderr,
            )
            # 尝试从非JSON字符串中提取数字作为备用方案
            numeric_ids = re.findall(r"\b\d+\b", llm_message_content)
            return numeric_ids[0] if numeric_ids else "0"

    except requests.RequestException as e:
        print(f"--- [Bridge] ERROR: Could not query LLM: {e}", file=sys.stderr)
        return "0"


def main():
    """
    主函数：启动C游戏作为子进程，并充当C游戏与LLM之间的状态化控制器（支持并发思考）。
    """
    global conversation_history
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

    while game_process.poll() is None:
        try:
            # --- 主循环的全部逻辑在这里 ---
            line = game_process.stdout.readline()
            if not line:
                break
            
            if line.startswith(command_prefix):
                command = line.strip().split(':', 1)[1]
                
                if command == "INPUT_REQUIRED":
                    print(f"{COLOR_AI_THINKING}--- [Bridge] LLM is thinking in the background while you make a choice...{COLOR_RESET}")
                    user_input = input()
                    game_process.stdin.write(user_input + "\n")
                    game_process.stdin.flush()
                    if llm_future:
                        # a. 提示正在获取结果
                        print(f"{COLOR_AI_THINKING}--- [Bridge] Retrieving LLM decision...{COLOR_RESET}")
                        
                        # b. 获取包含ID和理由的完整决策字典
                        decision = llm_future.result() 
                        
                        # c. 从字典中提取信息
                        action_id = decision.get("action_id", "0")
                        reasoning = decision.get("reasoning", "No reasoning provided.")

                        # d. 在这里打印（揭露）决策！
                        print(f"{COLOR_AI_DECISION}--- [Bridge] LLM Decided Action ID: {action_id}{COLOR_RESET}")
                        print(f"{COLOR_AI_DECISION}--- [Bridge] LLM Reasoning: {reasoning}{COLOR_RESET}")

                        # e. 将决策ID发送给C程序
                        game_process.stdin.write(action_id + "\n")
                        game_process.stdin.flush()
                        
                        llm_future = None
                
                elif command == "NEW_GAME_START":
                    print("\n--- [Bridge] New game detected, resetting conversation history. ---")
                    conversation_history = []
                    prompt_buffer = []
                    is_buffering_prompt = False
                    if llm_future:
                        llm_future.cancel()
                    llm_future = None
                continue
            
            if "You are a master strategist" in line or "== Turn Update" in line:
                is_buffering_prompt = True
            
            if is_buffering_prompt:
                prompt_buffer.append(line)
            else:
                print(line, end='')

            if prompt_end_trigger in line:
                if is_buffering_prompt:
                    full_prompt = "".join(prompt_buffer)
                    prompt_buffer = []
                    if "Communication Protocol" in full_prompt:
                         print("--- [Bridge] Genesis Prompt captured. Waiting for first turn... ---")
                         conversation_history.append({"role": "user", "content": full_prompt})
                    else:
                        llm_future = executor.submit(query_llm, full_prompt)
                is_buffering_prompt = False

        except (BrokenPipeError, KeyboardInterrupt):
            print("\n--- [Bridge] Process interrupted. Shutting down. ---")
            break
        except Exception as e:
            # 这是必不可少的“安全网”，用于捕捉所有未预料到的错误
            print(f"\n--- [Bridge] An unexpected error occurred: {e} ---", file=sys.stderr)
            break
    
    executor.shutdown()
    game_process.terminate()
    print("--- [Bridge] Game process terminated. ---")


if __name__ == "__main__":
    main()
