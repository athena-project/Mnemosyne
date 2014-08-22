
--
-- Table structure for table `chunk`
--

DROP TABLE IF EXISTS `chunk`;

CREATE TABLE `chunk` (
  `id` BIGINT(11) NOT NULL AUTO_INCREMENT,
  `size` INT(11),
  PRIMARY KEY (`id`),
  KEY key_block_id (block_id)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

