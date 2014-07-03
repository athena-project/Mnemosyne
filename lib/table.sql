
--
-- Table structure for table `block`
--

DROP TABLE IF EXISTS `block`;

CREATE TABLE `block` (
  `id` BIGINT(11) NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

--
-- Table structure for table `chunk`
--

DROP TABLE IF EXISTS `chunk`;

CREATE TABLE `chunk` (
  `id` BIGINT(11) NOT NULL AUTO_INCREMENT,
  `block_id` BIGINT(11),
  `size` INT(11),
  PRIMARY KEY (`id`),
  KEY key_block_id (block_id)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

