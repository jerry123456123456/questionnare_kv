import requests
import time
import random

# 定义请求的 URL 和请求次数
login_url = "http://192.168.186.138/api/login"
num_requests = 1000

# 模拟的用户名和密码列表
usernames = ["user1", "user2", "user3", "root"]
passwords = ["password1", "password2", "password3", "root_password"]


# 加密函数，模拟前端的 MD5 加密
def md5(input_string):
    import hashlib
    return hashlib.md5(input_string.encode()).hexdigest()


# 开始计时
start_time = time.time()

# 发送请求
for i in range(num_requests):
    # 随机选择用户名和密码
    username = random.choice(usernames)
    password = random.choice(passwords)
    encrypted_password = md5(password)

    json_data = {
        "user": username,
        "pwd": encrypted_password
    }

    try:
        response = requests.post(login_url, json=json_data)
        if response.status_code != 200:
            print(f"Request {i} failed with status code {response.status_code}")
    except requests.RequestException as e:
        print(f"Request {i} failed with error: {e}")

# 结束计时
end_time = time.time()

# 计算总时间和平均响应时间
total_time = end_time - start_time
average_time = total_time / num_requests

print(f"Total time: {total_time} seconds")
print(f"Average response time: {average_time} seconds")