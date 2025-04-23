-- ./wrk -t10 -c100 -d30 -s your_script.lua http://192.168.186.138/api/login

-- MD5 加密函数
local function md5(str)
    local f = io.popen('echo -n "' .. str .. '" | md5sum', 'r')
    local result = f:read('*a')
    f:close()
    return result:sub(1, 32)
end

-- 服务器 IP 地址
local SERVER_IP = "192.168.186.138"
-- 测试用户名
local test_user = "jerry"
-- 测试密码
local test_password = "123456jerry"
-- 对密码进行 MD5 加密
local hashed_password = md5(test_password)

-- 请求函数
request = function()
    -- 构建完整的请求 URL
    local url = string.format("http://%s/api/login", SERVER_IP)
    -- 构建符合后端要求的 JSON 请求体
    local request_body = string.format('{"user": "%s", "pwd": "%s"}', test_user, hashed_password)
    -- 设置请求头
    local headers = {
        ["Content-Type"] = "application/json"
    }
    -- 打印请求信息，方便调试
    print(string.format("请求 URL: %s", url))
    print(string.format("请求体: %s", request_body))
    -- 返回格式化后的请求
    return wrk.format("POST", url, headers, request_body)
end

-- 响应处理函数
response = function(status, headers, body)
    -- 打印响应状态码和响应体，方便查看详细信息
    print(string.format("响应状态码: %d", status))
    print(string.format("响应体: %s", body))
    -- 检查状态码是否为 200
    if status ~= 200 then
        print(string.format(
            "登录请求失败 | 状态码: %d | 响应体: %s",
            status, body
        ))
        return
    end
    -- 检查响应中是否包含 "code": 0
    if string.find(body, '"code":0') then
        -- 提取 token
        local start_index, end_index = string.find(body, '"token":"([^"]+)"')
        if start_index then
            local token = string.sub(body, start_index + 8, end_index - 1)
            -- 去除可能包含的引号
            token = token:gsub('^"', ''):gsub('"$', '')
            wrk.thread:set("token", token)
            print(string.format("登录成功，获取到的 token: %s", token))
        else
            print("登录成功，但响应中未找到 token")
        end
    else
        -- 尝试从响应中提取错误信息
        local error_start, error_end = string.find(body, '"message": "([^"]+)"')
        local error_message = error_start and string.sub(body, error_start + 11, error_end - 1) or "无错误描述"
        print(string.format(
            "登录业务失败 | 错误信息: %s",
            error_message
        ))
    end
end