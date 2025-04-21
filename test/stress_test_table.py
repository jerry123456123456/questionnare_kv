import requests
import time
import random
import hashlib

# 定义基础URL
base_url = "http://192.168.186.138"
# 登录接口
login_url = f"{base_url}/api/login"
# 获取问卷接口
get_survey_url = f"{base_url}/api/mytables?cmd=normal"
# 提交问卷接口
submit_survey_url = f"{base_url}/api/upload"

# 模拟的用户名和密码列表
usernames = ["user1", "user2", "user3"]
passwords = ["password1", "password2", "password3"]

# 定义请求次数
num_requests = 100

# 加密函数，模拟前端的 MD5 加密
def md5(input_string):
    return hashlib.md5(input_string.encode()).hexdigest()

# 开始计时
start_time = time.time()

for i in range(num_requests):
    try:
        # 随机选择用户名和密码
        username = random.choice(usernames)
        password = random.choice(passwords)
        encrypted_password = md5(password)

        # 登录
        login_data = {
            "user": username,
            "pwd": encrypted_password
        }
        login_response = requests.post(login_url, json=login_data)
        if login_response.status_code != 200:
            print(f"Request {i} - Login failed with status code {login_response.status_code}")
            continue
        login_result = login_response.json()
        if "code" in login_result and login_result["code"] != 0:
            error_message = login_result.get("message", "Unknown error")
            print(f"Request {i} - Login failed: {error_message}")
            continue
        token = login_result.get("token")
        if not token:
            print(f"Request {i} - Login failed: Token not found in response.")
            continue

        # 获取问卷
        title = "test_title"  # 这里假设问卷标题为 test_title，可根据实际情况修改
        get_survey_data = {
            "user": username,
            "token": token,
            "title": title
        }
        get_survey_response = requests.post(get_survey_url, json=get_survey_data)
        if get_survey_response.status_code != 200:
            print(f"Request {i} - Get survey failed with status code {get_survey_response.status_code}")
            continue
        survey = get_survey_response.json()

        # 填写问卷
        answers = []
        for question in survey["questions"]:
            answer = {
                "question_text": question["question_text"]
            }
            if question["question_type"] == "single_choice":
                selected_option = random.choice(question["options"])
                answer["answer"] = selected_option["option_text"]
            elif question["question_type"] == "multiple_choice":
                selected_options = random.sample(question["options"], random.randint(1, len(question["options"])))
                for option in selected_options:
                    new_answer = answer.copy()
                    new_answer["answer"] = option["option_text"]
                    answers.append(new_answer)
                continue
            elif question["question_type"] == "fill_in_blank":
                answer["answer"] = "test_answer"
            answers.append(answer)

        # 提交问卷
        submit_data = {
            "user_name": username,
            "token": token,
            "table_name": title,
            "answers": answers
        }
        submit_response = requests.post(submit_survey_url, json=submit_data)
        if submit_response.status_code != 200:
            print(f"Request {i} - Submit survey failed with status code {submit_response.status_code}")
            continue
        submit_result = submit_response.json()
        if "code" in submit_result and submit_result["code"] != 0:
            error_message = submit_result.get("message", "Unknown error")
            print(f"Request {i} - Submit survey failed: {error_message}")
    except requests.RequestException as e:
        print(f"Request {i} failed with error: {e}")

# 结束计时
end_time = time.time()

# 计算总时间和平均响应时间
total_time = end_time - start_time
average_time = total_time / num_requests

print(f"Total time: {total_time} seconds")
print(f"Average response time: {average_time} seconds")