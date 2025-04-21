数据库表设计

1. **问卷表（Surveys）**：

   - `survey_id`（主键）：问卷ID

   - `title`：问卷标题

   - 隶属用户

   - 其他相关字段，如创建时间、描述等

2. **题目表（Questions）**：

   - `question_id`（主键）：题目ID

   - `survey_id`（外键）：关联到问卷表的问卷ID

   - `question_text`：题目内容

   - `question_type`：题目类型（单选题、多选题、填空题等）

3. **选项表（Options）**（针对单选题和多选题）：

   - `option_id`（主键）：选项ID

   - `question_id`（外键）：关联到题目表的题目ID

   - `option_text`：选项内容

4. **回答表（Responses）**：

   - `response_id`（主键）：回答ID

   - `survey_id`（外键）：关联到问卷表的问卷ID

   - `question_id`（外键）：关联到题目表的题目ID

   - `user_id`：回答用户的ID

   - `answer`：回答内容


/////////////////////////////////////

假设我们要创建一个问卷，主题为“饮食习惯调查”，并在其中添加一个多选题，询问用户“您通常吃哪些类型的食物？”。

1. 插入一个 root 用户
首先，我们插入一个 root 用户：

sql
复制代码
INSERT INTO users (username, password, is_root) VALUES ('admin', 'password123', 1);
2. 插入一个问卷
接下来，我们为问卷插入一条记录。假设我们创建的问卷 ID 是 1（通常在插入后可以通过 LAST_INSERT_ID() 获取，但这里假设我们已经知道它是 1）。

sql
复制代码
INSERT INTO surveys (title, description, created_by) VALUES ('饮食习惯调查', '请告诉我们您的饮食习惯', 1);
3. 插入一个多选题
现在我们在问卷中插入一个多选题。假设该问卷的 ID 是 1，我们要添加的问题是“您通常吃哪些类型的食物？”。

sql
复制代码
INSERT INTO questions (survey_id, content, question_type) VALUES (1, '您通常吃哪些类型的食物？', 'multiple_choice');
4. 插入多个选项
接下来，我们为这个多选题添加几个选项，比如：

蔬菜
水果
肉类
快餐
假设该多选题的 ID 是 1（通常在插入后可以通过 LAST_INSERT_ID() 获取，以下示例直接插入选项）。

sql
复制代码
INSERT INTO options (question_id, content) VALUES (1, '蔬菜');
INSERT INTO options (question_id, content) VALUES (1, '水果');
INSERT INTO options (question_id, content) VALUES (1, '肉类');
INSERT INTO options (question_id, content) VALUES (1, '快餐');
完整的 SQL 示例
将所有步骤整合成一个完整的 SQL 示例：

sql
复制代码
-- 插入 root 用户
INSERT INTO users (username, password, is_root) VALUES ('admin', 'password123', 1);

-- 插入问卷
INSERT INTO surveys (title, description, created_by) VALUES ('饮食习惯调查', '请告诉我们您的饮食习惯', 1);

-- 插入多选题
INSERT INTO questions (survey_id, content, question_type) VALUES (1, '您通常吃哪些类型的食物？', 'multiple_choice');

-- 插入选项
INSERT INTO options (question_id, content) VALUES (1, '蔬菜');
INSERT INTO options (question_id, content) VALUES (1, '水果');
INSERT INTO options (question_id, content) VALUES (1, '肉类');
INSERT INTO options (question_id, content) VALUES (1, '快餐');

//////////////////////////////////
打开终端或命令提示符：

如果你使用的是 Windows，可以按 Win + R，输入 cmd，然后回车。
如果你使用的是 macOS 或 Linux，打开终端。
登录到 MySQL： 输入以下命令，替换 username 和 password 为你的 MySQL 用户名和密码：

bash
复制代码
mysql -u username -p
输入密码后按回车。

创建数据库（如果还没有创建）： 如果你还没有创建数据库，可以在 MySQL 提示符下输入：

sql
复制代码
CREATE DATABASE survey_system;
USE survey_system;
运行 SQL 脚本： 如果你将 SQL 脚本保存到一个文件中，比如 setup.sql，可以通过以下命令运行它：

bash
复制代码
SOURCE /home/jerry/Desktop/questionnare/questionnare.sql;
将 /path/to/your/setup.sql 替换为实际文件路径。