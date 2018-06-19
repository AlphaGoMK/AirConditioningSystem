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

 Date: 19/06/2018 17:16:57
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for parameters
-- ----------------------------
DROP TABLE IF EXISTS `parameters`;
CREATE TABLE `parameters`  (
  `ecost_high` int(11) NULL DEFAULT NULL,
  `ecost_mid` int(11) NULL DEFAULT NULL,
  `ecost_low` int(11) NULL DEFAULT NULL,
  `yuan_per_degree` float NULL DEFAULT NULL,
  `cgtp` float NULL DEFAULT NULL,
  `mode` int(11) NULL DEFAULT NULL,
  `t_high` float NULL DEFAULT NULL,
  `t_low` float NULL DEFAULT NULL,
  `maxserve` int(11) NULL DEFAULT NULL,
  `sec_per_minite` int(11) NULL DEFAULT NULL,
  `timeslot` int(11) NULL DEFAULT NULL,
  `threshold` int(11) NULL DEFAULT NULL,
  `default_t` double NULL DEFAULT NULL,
  `default_fan` int(11) NULL DEFAULT NULL,
  `id` int(10) NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of parameters
-- ----------------------------
INSERT INTO `parameters` VALUES (8, 4, 2, 0.02, 0.03, 1, 28, 20, 5, 3, 2, 4, 20, 3, 0);

SET FOREIGN_KEY_CHECKS = 1;
