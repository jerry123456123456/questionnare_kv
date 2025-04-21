--source /home/jerry/Desktop/questionnare/sql/create_mysql.sql;
USE survey_system;

-- 查询root、jerry和tom的user_id
SET @root_id := (SELECT user_id FROM Users WHERE user_name = 'root');
SET @jerry_id := (SELECT user_id FROM Users WHERE user_name = 'jerry');
SET @tom_id := (SELECT user_id FROM Users WHERE user_name = 'tom');

-- 向Surveys表插入同一个问卷名的三条记录（分别关联root、jerry和tom的user_id）
INSERT INTO Surveys (title, user_id)
VALUES
('通用问卷标题', @root_id),
('通用问卷标题', @jerry_id),
('通用问卷标题', @tom_id);

-- 获取刚刚插入的三条记录对应的survey_id（使用MySQL的LAST_INSERT_ID函数结合偏移来准确获取，假设按插入顺序依次获取对应值）
SET @survey_id_1 := LAST_INSERT_ID();
SET @survey_id_2 := LAST_INSERT_ID() + 1;
SET @survey_id_3 := LAST_INSERT_ID() + 2;

-- 向Questions表插入相同的问题（这里示例插入两个不同类型的问题，你可以根据需求修改）
INSERT INTO Questions (survey_id, question_text, question_type)
VALUES
-- 针对第一个survey_id插入问题
(@survey_id_1, '问题1，这是一个单选题', 'single_choice'),
(@survey_id_1, '问题2，这是一个多选题', 'multiple_choice'),
-- 针对第二个survey_id插入相同的问题
(@survey_id_2, '问题1，这是一个单选题', 'single_choice'),
(@survey_id_2, '问题2，这是一个多选题', 'multiple_choice'),
-- 针对第三个survey_id插入相同的问题
(@survey_id_3, '问题1，这是一个单选题', 'single_choice'),
(@survey_id_3, '问题2，这是一个多选题', 'multiple_choice');

-- 获取刚刚插入的属于第一个survey_id的问题的question_id（使用LAST_INSERT_ID函数结合偏移准确获取）
SET @question_id_1_1 := LAST_INSERT_ID() - 1; -- 对应第一个survey_id的第一个问题（单选题）
SET @question_id_1_2 := LAST_INSERT_ID(); -- 对应第一个survey_id的第二个问题（多选题）

-- 获取刚刚插入的属于第二个survey_id的问题的question_id
SET @question_id_2_1 := LAST_INSERT_ID() + 1; -- 对应第二个survey_id的第一个问题（单选题）
SET @question_id_2_2 := LAST_INSERT_ID() + 2; -- 对应第二个survey_id的第二个问题（多选题）

-- 获取刚刚插入的属于第三个survey_id的问题的question_id
SET @question_id_3_1 := LAST_INSERT_ID() + 3; -- 对应第三个survey_id的第一个问题（单选题）
SET @question_id_3_2 := LAST_INSERT_ID() + 4; -- 对应第三个survey_id的第二个问题（多选题）

-- 向Options表插入对应单选题和多选题的选项（以下示例每个问题插入3个选项，可按需调整）

-- 为第一个survey_id的单选题插入选项
INSERT INTO Options (question_id, option_text)
VALUES
(@question_id_1_1, '选项A（问题1）'),
(@question_id_1_1, '选项B（问题1）'),
(@question_id_1_1, '选项C（问题1）');

-- 为第一个survey_id的多选题插入选项
INSERT INTO Options (question_id, option_text)
VALUES
(@question_id_1_2, '选项A（问题2）'),
(@question_id_1_2, '选项B（问题2）'),
(@question_id_1_2, '选项C（问题2）');

-- 为第二个survey_id的单选题插入选项
INSERT INTO Options (question_id, option_text)
VALUES
(@question_id_2_1, '选项A（问题1）'),
(@question_id_2_1, '选项B（问题1）'),
(@question_id_2_1, '选项C（问题1）');

-- 为第二个survey_id的多选题插入选项
INSERT INTO Options (question_id, option_text)
VALUES
(@question_id_2_2, '选项A（问题2）'),
(@question_id_2_2, '选项B（问题2）'),
(@question_id_2_2, '选项C（问题2）');

-- 为第三个survey_id的单选题插入选项
INSERT INTO Options (question_id, option_text)
VALUES
(@question_id_3_1, '选项A（问题1）'),
(@question_id_3_1, '选项B（问题1）'),
(@question_id_3_1, '选项C（问题1）');

-- 为第三个survey_id的多选题插入选项
INSERT INTO Options (question_id, option_text)
VALUES
(@question_id_3_2, '选项A（问题2）'),
(@question_id_3_2, '选项B（问题2）'),
(@question_id_3_2, '选项C（问题2）');