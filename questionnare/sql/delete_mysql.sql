--source /home/jerry/Desktop/questionnare/sql/delete_mysql.sql;
-- 清空Users表数据
--DELETE FROM Users;

-- 重置自增列（如果需要的话，不同数据库重置自增列的方式可能略有不同，以下以常见的MySQL为例）
--ALTER TABLE Users AUTO_INCREMENT = 1;

-- 清空Surveys表数据
DELETE FROM Surveys;
ALTER TABLE Surveys AUTO_INCREMENT = 1;

-- 清空Questions表数据
DELETE FROM Questions;
ALTER TABLE Questions AUTO_INCREMENT = 1;

-- 清空Options表数据
DELETE FROM Options;
ALTER TABLE Options AUTO_INCREMENT = 1;

-- 清空Responses表数据
DELETE FROM Responses;
ALTER TABLE Responses AUTO_INCREMENT = 1;