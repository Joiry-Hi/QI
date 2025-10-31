import subprocess
import requests # 或者使用本地LLM的Python库

# --- 配置 ---
LLM_API_URL = "http://localhost:11434/api/generate" # Ollama的API示例
LLM_MODEL_NAME = "llama3" # 您本地部署的模型名称
GAME_EXECUTABLE = "./QI.exe" # 您的C程序路径

def query_llm(prompt):
    """向本地LLM发送请求并获取回应"""
    try:
        response = requests.post(
            LLM_API_URL,
            json={
                "model": LLM_MODEL_NAME,
                "prompt": prompt,
                "stream": False
            }
        )
        response.raise_for_status()
        # 解析LLM的回应，只提取出数字ID
        llm_output = response.json().get("response", "").strip()
        action_id = ''.join(filter(str.isdigit, llm_output))
        return action_id if action_id else "0" # 如果没有数字，默认返回0
    except requests.RequestException as e:
        print(f"Error querying LLM: {e}")
        return "0" # 出错时返回一个安全的默认动作

def main():
    # 启动C语言游戏子进程，并建立管道
    game_process = subprocess.Popen(
        GAME_EXECUTABLE,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1 # 行缓冲
    )

    prompt_buffer = []
    while game_process.poll() is None:
        # 从C程序的stdout读取一行输出
        line = game_process.stdout.readline()
        if not line:
            break
        
        # 将输出收集到缓冲区，直到遇到结束标记
        if "END_OF_PROMPT" in line:
            full_prompt = "".join(prompt_buffer)
            prompt_buffer = [] # 清空缓冲区
            
            # 将收集到的完整Prompt发送给LLM
            print(f"--- Sending to LLM ---\n{full_prompt.strip()}")
            action_id = query_llm(full_prompt)
            print(f"--- LLM chose Action ID: {action_id} ---")
            
            # 将LLM的决策通过stdin“喂”给C程序
            game_process.stdin.write(action_id + "\n")
            game_process.stdin.flush()
        else:
            prompt_buffer.append(line)
            # 实时打印C程序的其他输出（比如战斗日志）
            print(line, end='')

    # 处理游戏结束后的剩余输出
    stdout, stderr = game_process.communicate()
    print(stdout)
    print(stderr)

if __name__ == "__main__":
    main()