import re
import datetime
import subprocess
import requests
import sys

# --- 配置 ---
# 你的本地Ollama API地址
LLM_API_URL = "http://localhost:11434/api/generate" 
# 你希望使用的本地模型名称 (例如 "llama3", "gemma", "qwen")
LLM_MODEL_NAME = "mistral" # <-- 已更新为您已安装的 mistral 模型
# 你的C语言游戏可执行文件的路径
GAME_EXECUTABLE = "./QI"


def query_llm(prompt: str) -> str:
    """
    向本地Ollama LLM发送请求，并将prompt写入日志文件。
    """
    # 将prompt写入一个带时间戳的日志文件
    with open("llm_prompts.log", "a", encoding="utf-8") as f:
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"--- Prompt at {timestamp} ---\n")
        f.write(prompt)
        f.write("\n\n")

    print(f"\n--- [Bridge] Prompt logged. Thinking...")
    # --- END REFACTOR ---
    
    try:
        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": 0.2
                }
            },
            timeout=120
        )
        response.raise_for_status()
        
        llm_output = response.json().get("response", "").strip()
        # 我们可以保留这个原始回复的打印，因为它对于理解AI的“思路”很有价值
        print(f"--- [Bridge] Raw LLM Response: '{llm_output}' ---")

        numeric_ids = re.findall(r'\b\d+\b', llm_output)
        
        if numeric_ids:
            action_id = numeric_ids[0]
        else:
            action_id = "0"
        
        return action_id
        
    except requests.RequestException as e:
        print(f"--- [Bridge] ERROR: Could not query LLM: {e} ---", file=sys.stderr)
        return "0"
    
def main():
    """
    主函数：启动C游戏作为子进程，并充当C游戏与LLM之间的双向控制器。
    """
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
    is_buffering_for_llm = False
    llm_prompt_start_trigger = "You are a master strategist"
    human_input_trigger = "INPUT_REQUIRED" # <-- 新的、精确的触发器

    while game_process.poll() is None:
        try:
            line = game_process.stdout.readline()
            if not line:
                break
            
            # --- 状态转换点 1: 侦测到LLM Prompt的开始 ---
            if llm_prompt_start_trigger in line:
                is_buffering_for_llm = True
            
            # --- 核心逻辑: 根据状态决定是“显示”还是“静默缓冲” ---
            if is_buffering_for_llm:
                prompt_buffer.append(line)
            else:
                # --- 关键修正: 绝不打印我们的内部信号 ---
                if human_input_trigger not in line:
                    print(line, end='')

            # --- 信号处理点 1: 人类玩家输入 ---
            if human_input_trigger in line:
                user_input = input()
                game_process.stdin.write(user_input + "\n")
                game_process.stdin.flush()

            # --- 状态转换点 2 & 信号处理点 2: LLM Prompt结束 ---
            elif "END_OF_PROMPT" in line:
                if is_buffering_for_llm:
                    full_prompt = "".join(prompt_buffer)
                    prompt_buffer = []
                    
                    action_id = query_llm(full_prompt)
                    
                    game_process.stdin.write(action_id + "\n")
                    game_process.stdin.flush()
                    
                    is_buffering_for_llm = False

        except (BrokenPipeError, KeyboardInterrupt):
            # ... (异常处理保持不变)
            break
        except Exception as e:
            # ... (异常处理保持不变)
            break
        
if __name__ == "__main__":
    main()