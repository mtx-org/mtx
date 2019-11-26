-- phpMyAdmin SQL Dump
-- version 2.9.2
-- http://www.phpmyadmin.net
-- 
-- Host: localhost
-- Generation Time: Feb 02, 2007 at 01:17 PM
-- Server version: 5.0.27
-- PHP Version: 5.1.6
-- 
-- Database: `mtx`
-- 

-- --------------------------------------------------------

-- 
-- Table structure for table `hosts`
-- 

CREATE TABLE IF NOT EXISTS `hosts` (
  `osname` varchar(25) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`osname`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

-- 
-- Table structure for table `loaders`
-- 

CREATE TABLE IF NOT EXISTS `loaders` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `enabled` tinyint(1) NOT NULL default '0',
  `worked` tinyint(1) NOT NULL default '0',
  `osname` varchar(25) character set ascii NOT NULL,
  `osversion` varchar(100) character set ascii default NULL,
  `description` varchar(100) character set ascii default NULL,
  `vendorid` char(8) character set ascii default NULL,
  `productid` char(16) character set ascii default NULL,
  `revision` char(4) character set ascii default NULL,
  `barcodes` tinyint(1) NOT NULL default '0',
  `eaap` tinyint(1) NOT NULL default '0',
  `transports` smallint(5) unsigned NOT NULL default '0',
  `slots` smallint(5) unsigned NOT NULL default '0',
  `imports` smallint(5) unsigned NOT NULL default '0',
  `transfers` smallint(5) unsigned NOT NULL default '0',
  `tgdp` tinyint(1) NOT NULL default '0',
  `canxfer` tinyint(1) NOT NULL default '0',
  `serialnum` varchar(25) character set ascii default NULL,
  `email` varchar(80) character set ascii default NULL,
  `name` varchar(80) collate utf8_unicode_ci default NULL,
  `contributed` datetime default NULL,
  `mtxversion` varchar(8) character set ascii NOT NULL,
  `comments` text collate utf8_unicode_ci,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

-- 
-- Table structure for table `versions`
-- 

CREATE TABLE IF NOT EXISTS `versions` (
  `key` mediumint(8) unsigned NOT NULL,
  `version` varchar(8) character set ascii NOT NULL,
  PRIMARY KEY  (`key`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
