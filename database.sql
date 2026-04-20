MySQL Schema
CREATE DATABASE IF NOT EXISTS semis_db;
USE semis_db;

-- 1. Bins
CREATE TABLE bins (
    bin_id VARCHAR(20) PRIMARY KEY,
    location VARCHAR(100) NOT NULL,
    latitude DECIMAL(10,8),
    longitude DECIMAL(11,8),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2. Sensor Readings (raw + processed)
CREATE TABLE sensor_readings (
    reading_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    bin_id VARCHAR(20) NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    fill_percentage DECIMAL(5,2) NOT NULL,
    temperature DECIMAL(5,2) NOT NULL,
    raw_json JSON,
    FOREIGN KEY (bin_id) REFERENCES bins(bin_id) ON DELETE CASCADE
);

-- 3. Rule-based Classifications
CREATE TABLE classifications (
    classification_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    reading_id BIGINT NOT NULL,
    category ENUM('hazardous', 'recyclable', 'reusable') NOT NULL,
    reason TEXT,
    FOREIGN KEY (reading_id) REFERENCES sensor_readings(reading_id)
);

-- 4. Users (3 roles)
CREATE TABLE users (
    user_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    role ENUM('citizen', 'recycler', 'regulator') NOT NULL,
    full_name VARCHAR(100),
    points INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 5. Gamification Events
CREATE TABLE gamification_events (
    event_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    points_awarded INT NOT NULL,
    badge VARCHAR(50),
    description TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id)
);

-- 6. Digital Product Passport (one per e-waste item)
CREATE TABLE digital_product_passports (
    passport_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    bin_id VARCHAR(20) NOT NULL,
    item_type VARCHAR(100) NOT NULL,           -- e.g. "Smartphone", "Laptop"
    status ENUM('collected','classified','recycled','disposed') DEFAULT 'collected',
    co2_saved_kg DECIMAL(6,2) DEFAULT 0.00,
    material_value_usd DECIMAL(8,2) DEFAULT 0.00,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (bin_id) REFERENCES bins(bin_id)
);

-- Insert sample data
INSERT INTO bins (bin_id, location, latitude, longitude) VALUES 
('BIN001', 'Glasgow City Centre', 55.8609, -4.2580);

INSERT INTO users (username, role, full_name, points) VALUES 
('citizen1', 'citizen', 'Alice Brown', 250),
('recycler1', 'recycler', 'Green Recycling Ltd', 0),
('regulator1', 'regulator', 'UWS Environment Officer', 0);
