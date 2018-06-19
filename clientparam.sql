/*
 Navicat MySQL Data Transfer

 Source Server         : Assign
 Source Server Type    : MySQL
 Source Server Version : 80011
 Source Host           : localhost:3306
 Source Schema         : air

 Target Server Type    : MySQL
 Target Server Version : 80011
 File Encoding         : 65001

 Date: 19/06/2018 17:16:31
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for clientparam
-- ----------------------------
DROP TABLE IF EXISTS `clientparam`;
CREATE TABLE `clientparam`  (
  `room_id` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `ip` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `port` int(11) NULL DEFAULT NULL,
  `fade_rate` float NULL DEFAULT NULL,
  `id` int(10) NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of clientparam
-- ----------------------------
INSERT INTO `clientparam` VALUES ('306D', '192.168.137.1', 8001, 1, 0);

SET FOREIGN_KEY_CHECKS = 1;
