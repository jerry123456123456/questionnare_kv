-- 插入用户数据
INSERT INTO Users (username, password, is_root) VALUES
('user1', 'hashed_password_1', 0),
('user2', 'hashed_password_2', 0),
('admin', 'hashed_password_admin', 1);  -- 添加 admin 用户

-- 插入问卷数据
INSERT INTO Surveys (title, user_id, description) VALUES
('问卷1', 1, '这是问卷1的描述。'),
('问卷2', 2, '这是问卷2的描述。'),
('问卷3', 3, '这是问卷3的描述。');

-- 插入题目数据
INSERT INTO Questions (survey_id, question_text, question_type) VALUES
(1, '你最喜欢的编程语言是什么？', 'single_choice'),
(1, '你通常使用什么开发工具？', 'multiple_choice'),
(2, '请描述你的职业。', 'fill_in_blank'),
(3, '你喜欢什么样的学习方式？', 'multiple_choice');

-- 插入选项数据
INSERT INTO Options (question_id, option_text) VALUES
(1, 'C++'),
(1, 'Python'),
(1, 'Java'),
(2, 'Visual Studio'),
(2, 'VS Code'),
(2, 'Eclipse'),
(3, '听讲座'),
(3, '看书'),
(3, '动手实践');

-- 插入回答数据
INSERT INTO Responses (survey_id, question_id, user_id, answer) VALUES
(1, 1, 1, 'Python'),
(1, 2, 1, 'Visual Studio, VS Code'),
(2, 3, 2, '软件工程师'),
(3, 4, 3, '动手实践');

