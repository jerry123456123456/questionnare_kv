CREATE DATABASE IF NOT EXISTS survey_system;

USE survey_system;

CREATE TABLE Users (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    user_name VARCHAR(255) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    is_root TINYINT(1) DEFAULT 0,
    INDEX idx_user_name (user_name)
);

CREATE TABLE Surveys (
    survey_id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    user_id INT NOT NULL,  
    is_filled TINYINT(1) DEFAULT 0,
    root_survey_id INT,  
    is_dead TINYINT(1) DEFAULT 0,
    deadline DATETIME,  
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    INDEX idx_surveys_user_id (user_id),
    INDEX idx_surveys_title (title)
);

CREATE TABLE Questions (
    question_id INT AUTO_INCREMENT PRIMARY KEY,
    survey_id INT,
    question_text TEXT NOT NULL,
    question_type ENUM('single_choice', 'multiple_choice', 'fill_in_blank') NOT NULL,
    root_question_id INT, 
    is_dead TINYINT(1) DEFAULT 0,
    FOREIGN KEY (survey_id) REFERENCES Surveys(survey_id) ON DELETE CASCADE,
    INDEX idx_questions_survey_id (survey_id),
    INDEX idx_questions_question_type (question_type)
);

CREATE TABLE Options (
    option_id INT AUTO_INCREMENT PRIMARY KEY,
    question_id INT,
    option_text VARCHAR(255) NOT NULL,
    root_option_id INT,
    is_dead TINYINT(1) DEFAULT 0,
    FOREIGN KEY (question_id) REFERENCES Questions(question_id) ON DELETE CASCADE,
    INDEX idx_options_question_id (question_id)
);

CREATE TABLE Responses (
    response_id INT AUTO_INCREMENT PRIMARY KEY,
    survey_id INT,
    question_id INT,
    user_id INT,
    answer TEXT,
    option_id INT,  
    FOREIGN KEY (survey_id) REFERENCES Surveys(survey_id) ON DELETE CASCADE,
    FOREIGN KEY (question_id) REFERENCES Questions(question_id) ON DELETE CASCADE,
    INDEX idx_responses_survey_id (survey_id),
    INDEX idx_responses_question_id (question_id),
    INDEX idx_responses_user_id (user_id)
);