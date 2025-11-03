import json
import re
import datetime
import subprocess
import requests
import sys
import functools
from concurrent.futures import ThreadPoolExecutor
import logging # <-- 新增导入
from logging.handlers import RotatingFileHandler # <-- 新增导入
import os # <-- 新增：用于读取环境变量
import google.generativeai as genai # <-- 新增：导入Gemini库
from dotenv import load_dotenv # <-- 新增导入
import dashscope # <-- 新增导入

load_dotenv()

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

# --- 1. 升级LLM提供商配置 ---
# 在这里切换: "OLLAMA", "GOOGLE_GEMINI", 或 "DASHSCOPE"
LLM_PROVIDER = "DASHSCOPE"

# OLLAMA的配置
OLLAMA_API_URL = "http://localhost:11434/api/chat"
OLLAMA_MODEL_NAME = "llama3:8b" # 或者您想用的其他本地模型

# CLOUD (Gemini) 的配置
CLOUD_LLM_MODEL_NAME = "gemini-1.5-flash-latest" # 性价比极高的最新模型

# DASHSCOPE (通义千问) 的配置
DASHSCOPE_MODEL_NAME = "qwen-max" # 这是通义千问最强大的模型之一

# --- 2. 安全地配置云服务API密钥 ---
if LLM_PROVIDER == "GOOGLE_GEMINI":
    try:
        # 这里的 os.getenv 会成功读取到由 load_dotenv() 加载进来的密钥
        api_key = os.getenv("GOOGLE_API_KEY")
        if not api_key:
            # 这条错误现在只会在 .env 文件丢失或内容错误时触发
            raise ValueError("环境变量 GOOGLE_API_KEY 未在 .env 文件中设置！")
        genai.configure(api_key=api_key)
        print("--- [Bridge] Google Gemini API configured successfully. ---")
    except Exception as e:
        print(f"--- [Bridge] FATAL ERROR: Could not configure Gemini API: {e}", file=sys.stderr)
        sys.exit(1)
elif LLM_PROVIDER == "DASHSCOPE":
    try:
        api_key = os.getenv("DASHSCOPE_API_KEY")
        if not api_key:
            raise ValueError("环境变量 DASHSCOPE_API_KEY 未在 .env 文件中设置！")
        dashscope.api_key = api_key
        print("--- [Bridge] Alibaba Dashscope API configured successfully. ---")
    except Exception as e:
        print(f"--- [Bridge] FATAL ERROR: Could not configure Dashscope API: {e}", file=sys.stderr)
        sys.exit(1)

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


def query_ollama_llm_per_turn(user_prompt: str) -> dict:
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


def query_cloud_llm_per_turn(user_prompt: str) -> dict:
    """使用云端API（Dashscope/Gemini）进行“事无巨细”模式决策的查询函数"""
    global conversation_history
    conversation_history.append({"role": "user", "content": user_prompt})

    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Thinking with CLOUD context (Per-Turn Mode)...{COLOR_RESET}")

    try:
        if LLM_PROVIDER == "DASHSCOPE":
            messages = conversation_history
            # 通义千问的调用
            response = dashscope.Generation.call(
                model=DASHSCOPE_MODEL_NAME,
                messages=messages,
                result_format='json_object',
                temperature=0.2
            )
            if response.status_code == 200:
                llm_message_content = response.output.choices[0]['message']['content']
            else:
                raise Exception(f"Dashscope API Error: {response.message}")

        elif LLM_PROVIDER == "GOOGLE_GEMINI":
            # Gemini的调用 (注意，我们需要转换一下历史记录的格式)
            gemini_history = []
            for msg in conversation_history:
                role = 'user' if msg['role'] == 'user' else 'model'
                gemini_history.append({'role': role, 'parts': [msg['content']]})
            
            model = genai.GenerativeModel(CLOUD_LLM_MODEL_NAME)
            generation_config = genai.types.GenerationConfig(response_mime_type="application/json", temperature=0.2)
            response = model.generate_content(gemini_history, generation_config=generation_config, request_options={"timeout": 60})
            llm_message_content = response.text

        else: # 默认回到Ollama
            # (这里可以省略，因为总调度会处理)
            raise NotImplementedError("This cloud provider is not implemented for per-turn mode yet.")

        # --- 通用的解析、日志、返回逻辑 ---
        conversation_history.append({"role": "assistant", "content": llm_message_content})
        
        # ... (日志记录逻辑，可以从旧的 query_llm_per_turn 复制过来) ...
        logger.info(f"--- LLM Response (Per-Turn) ---\nRaw: {llm_message_content}")

        decision_json = json.loads(llm_message_content)
        return {
            "action_id": str(decision_json.get("action_id", "0")),
            "reasoning": decision_json.get("reasoning", "No reasoning provided.")
        }
        
    except Exception as e:
        logger.error(f"--- ERROR in CLOUD Per-Turn Query ---\nError: {e}")
        print(f"--- [Bridge] ERROR in CLOUD Per-Turn Query: {e}", file=sys.stderr)
        return {"action_id": "0", "reasoning": "Error."}

# 创建新的总调度函数
def query_llm_per_turn(user_prompt: str) -> dict:
    """“事无巨细”模式的总调度函数。"""
    if LLM_PROVIDER == "OLLAMA":
        return query_ollama_llm_per_turn(user_prompt)
    elif LLM_PROVIDER in ["GOOGLE_GEMINI", "DASHSCOPE"]:
        return query_cloud_llm_per_turn(user_prompt)
    else:
        print(f"--- [Bridge] ERROR: Invalid LLM_PROVIDER for Per-Turn mode.", file=sys.stderr)
        return {"action_id": "0", "reasoning": "Configuration Error."}


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
    
    
def query_dashscope_llm_strategy(prompt: str) -> str:
    """使用阿里云通义千问API进行战略决策的查询函数"""
    global marshal_manual
    print(f"\n{COLOR_AI_THINKING}--- [Bridge] Sending report to DASHSCOPE Grand Marshal (Qwen)...{COLOR_RESET}")
    try:
        # 通义千问的System Prompt和多轮对话格式
        messages = [
            {'role': 'system', 'content': marshal_manual},
            {'role': 'user', 'content': prompt}
        ]
        
        # 调用API，强制返回JSON格式
        response = dashscope.Generation.call(
            model=DASHSCOPE_MODEL_NAME,
            messages=messages,
            result_format='json_object', # 强制返回JSON对象
            temperature=0.3
        )

        # 检查响应状态并解析
        if response.status_code == 200:
            llm_message_content = response.output.choices[0]['message']['content']
            decision_json = json.loads(llm_message_content)
            
            parsed_general_id = str(decision_json.get("next_general_id", "0"))
            reasoning = decision_json.get("reasoning", "No reasoning provided.")
            
            final_general_id = verify_and_correct_decision(parsed_general_id, reasoning)

            # ... (打印和记录日志的代码，完全可以复用) ...
            print(f"{COLOR_AI_DECISION}--- [Bridge] Qwen Marshal's Decision: General ID {final_general_id}{COLOR_RESET}")
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
        else:
            # 处理API返回的错误
            raise Exception(f"Dashscope API Error: Code {response.status_code}, Message: {response.message}")

    except Exception as e:
        # ... (异常处理和日志记录) ...
        print(f"--- [Bridge] ERROR in DASHSCOPE Strategy Query: {e}", file=sys.stderr)
        return "0"
    
    
def query_llm_strategy(prompt: str) -> str:
    """战略决策的总调度函数。根据全局配置选择LLM提供商。"""
    if LLM_PROVIDER == "OLLAMA":
        return query_ollama_llm_strategy(prompt)
    elif LLM_PROVIDER == "GOOGLE_GEMINI":
        return query_cloud_llm_strategy(prompt)
    elif LLM_PROVIDER == "DASHSCOPE":
        return query_dashscope_llm_strategy(prompt) # <-- 新增的分支
    else:
        print(f"--- [Bridge] ERROR: Invalid LLM_PROVIDER '{LLM_PROVIDER}' configured.", file=sys.stderr)
        return "0"


def send_llm_result_to_c(future, game_process):
    """
    这是一个“回调”函数。当后台的LLM请求完成后，它会被自动调用。
    它的职责是获取结果，并将其安全地发送回C程序。
    """
    try:
        # .result()在这里调用是安全的，因为future已经完成了
        decision = future.result()
        
        # 打印决策（这部分代码从main函数移动到这里）
        print(f"\n{COLOR_AI_DECISION}--- [Bridge] LLM Decided Action ID: {decision['action_id']}{COLOR_RESET}")
        print(f"{COLOR_AI_DECISION}--- [Bridge] LLM Reasoning: {decision['reasoning']}{COLOR_RESET}")
        
        # 通过指定的game_process，将结果发送给C
        game_process.stdin.write(decision["action_id"] + "\n")
        game_process.stdin.flush()

    except Exception as e:
        # 在回调函数中也必须有健壮的错误处理
        print(f"--- [Bridge] ERROR in LLM callback: {e}", file=sys.stderr)
        # 即使出错，也要给C一个答复，防止C无限等待
        game_process.stdin.write("0\n")
        game_process.stdin.flush()


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
    global llm_mode, conversation_history, marshal_manual
    print(f"--- [Bridge] Starting game process: {GAME_EXECUTABLE} ---")

    game_process = subprocess.Popen(
        [GAME_EXECUTABLE, "--bridge"], # <-- 将命令变成一个列表，并添加参数
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        bufsize=1,
    )

    prompt_buffer = []
    is_buffering_prompt = False
    
    command_prefix = "##CMD##:"
    prompt_end_trigger = "END_OF_PROMPT"

    while game_process.poll() is None:
        try:
            line = game_process.stdout.readline()
            if not line: break

            # --- 命令处理器 ---
            if line.startswith(command_prefix):
                command = line.strip().split(":", 1)[1]

                if command.startswith("NEW_GAME_START"):
                    print(f"\n--- [Bridge] New game detected ({command}). Resetting state. ---")
                    llm_mode, marshal_manual, conversation_history = None, None, []
                    is_buffering_prompt = False
                    # 注意：我们不再需要llm_future，因为回调是自管理的

                elif command == "START_PROMPT":
                    is_buffering_prompt = True
                    prompt_buffer = []

                elif command == "INPUT_REQUIRED":
                    # 在人类模式下，流程是串行的，所以我们仍然在这里等待
                    user_input = input()
                    game_process.stdin.write(user_input + "\n")
                    game_process.stdin.flush()
                    
                    # 人类模式下的LLM决策是后续发生的，所以这里不需要回调
                    # C程序会再次请求，或者我们可以在这里触发一个同步的LLM调用
                    # 为简化，我们假设人类模式的LLM决策会在另一个流程中触发
                
                elif command == "GET_LLM_RESULT_FOR_AI_TURN":
                     # 在AI模式下，这个命令现在只是一个“路标”
                     # 我们不再需要在这里做任何事，因为回调会自动处理
                     print(f"{COLOR_AI_THINKING}--- [Bridge] V2 AI has moved. Awaiting LLM decision from background...{COLOR_RESET}")

                continue

            # --- Prompt 缓冲与任务提交 ---
            if is_buffering_prompt:
                prompt_buffer.append(line)
                
                if prompt_end_trigger in line:
                    full_prompt = "".join(prompt_buffer)
                    is_buffering_prompt = False

                    if llm_mode is None:
                        if "Grand Marshal's Principles of War" in full_prompt:
                            llm_mode = "MARSHAL"
                            marshal_manual = full_prompt
                            print(f"--- [Bridge] AI Mode: {llm_mode} (Manual Stored).")
                        elif "You are a master strategist" in full_prompt:
                            llm_mode = "PER_TURN"
                            print(f"--- [Bridge] AI Mode: {llm_mode}.")
                            conversation_history.append({"role": "user", "content": full_prompt})
                    
                    elif llm_mode == "PER_TURN":
                        # a. 提交后台任务
                        future = executor.submit(query_llm_per_turn, full_prompt)
                        # b. 为这个任务“注册”回调函数
                        #    当future完成后，`send_llm_result_to_c`会被自动调用
                        from functools import partial
                        callback = partial(send_llm_result_to_c, game_process=game_process)
                        future.add_done_callback(callback)
                    
                    elif llm_mode == "MARSHAL":
                        # 将帅模式是同步的，所以逻辑不变
                        new_general_id = query_llm_strategy(full_prompt)
                        game_process.stdin.write(new_general_id + "\n")
                        game_process.stdin.flush()
            else:
                print(line, end="")

        except (BrokenPipeError, KeyboardInterrupt):
            break
        except Exception as e:
            print(f"\n--- [Bridge] An unexpected error occurred: {e} ---", file=sys.stderr)
            break

    executor.shutdown()
    game_process.terminate()
    print("--- [Bridge] Game process terminated. ---")


if __name__ == "__main__":
    main()