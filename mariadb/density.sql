-- phpMyAdmin SQL Dump
-- version 4.8.0
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: May 16, 2018 at 09:23 AM
-- Server version: 5.7.21
-- PHP Version: 7.1.14

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `Density`
--

-- --------------------------------------------------------

--
-- Table structure for table `configuration`
--

CREATE TABLE `configuration` (
  `config_id` int(10) NOT NULL,
  `camera_id` int(20) NOT NULL,
  `real_street_width` int(11) NOT NULL,
  `real_street_height` int(5) NOT NULL,
  `mask_points_id` int(5) NOT NULL,
  `edge_threshold` int(5) NOT NULL,
  `low_threshold` int(5) NOT NULL,
  `max_threshold` int(5) NOT NULL,
  `morph_closing_iteration` int(5) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `configuration`
--

INSERT INTO `configuration` (`config_id`, `camera_id`, `real_street_width`, `real_street_height`, `mask_points_id`, `edge_threshold`, `low_threshold`, `max_threshold`, `morph_closing_iteration`) VALUES
(1, 1, 8, 12, 1, 1, 0, 200, 7),
(2, 2, 8, 16, 2, 1, 0, 100, 7),
(3, 3, 8, 16, 3, 1, 50, 100, 7),
(4, 4, 8, 18, 4, 1, 50, 100, 7),
(5, 5, 15, 15, 5, 1, 50, 100, 7),
(6, 6, 15, 20, 6, 1, 0, 100, 7);

-- --------------------------------------------------------

--
-- Table structure for table `density_history`
--

CREATE TABLE `density_history` (
  `density_history_id` int(255) NOT NULL,
  `date_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `camera_id` int(20) NOT NULL,
  `density_state` varchar(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `log`
--

CREATE TABLE `log` (
  `id` int(20) NOT NULL,
  `camera_id` varchar(5) NOT NULL,
  `time` int(20) NOT NULL,
  `concurrency` int(5) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `street_mask_points`
--

CREATE TABLE `street_mask_points` (
  `mask_points_id` int(10) NOT NULL,
  `x_lb` int(5) NOT NULL,
  `y_lb` int(5) NOT NULL,
  `x_rb` int(5) NOT NULL,
  `y_rb` int(5) NOT NULL,
  `x_rt` int(5) NOT NULL,
  `y_rt` int(5) NOT NULL,
  `x_lt` int(5) NOT NULL,
  `y_lt` int(5) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `street_mask_points`
--

INSERT INTO `street_mask_points` (`mask_points_id`, `x_lb`, `y_lb`, `x_rb`, `y_rb`, `x_rt`, `y_rt`, `x_lt`, `y_lt`) VALUES
(1, 250, 505, 880, 540, 635, 170, 325, 140),
(2, 365, 295, 545, 275, 703, 490, 300, 540),
(3, 250, 540, 590, 540, 545, 275, 420, 280),
(4, 500, 275, 650, 275, 790, 540, 425, 540),
(5, 280, 543, 960, 540, 650, 275, 355, 280),
(6, 290, 150, 625, 150, 960, 540, 80, 540);

--
-- Indexes for dumped tables
--

--
-- Indexes for table `configuration`
--
ALTER TABLE `configuration`
  ADD PRIMARY KEY (`config_id`),
  ADD KEY `mask_points_id` (`mask_points_id`);

--
-- Indexes for table `density_history`
--
ALTER TABLE `density_history`
  ADD PRIMARY KEY (`density_history_id`);

--
-- Indexes for table `log`
--
ALTER TABLE `log`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `street_mask_points`
--
ALTER TABLE `street_mask_points`
  ADD PRIMARY KEY (`mask_points_id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `configuration`
--
ALTER TABLE `configuration`
  MODIFY `config_id` int(10) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=7;

--
-- AUTO_INCREMENT for table `density_history`
--
ALTER TABLE `density_history`
  MODIFY `density_history_id` int(255) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT for table `log`
--
ALTER TABLE `log`
  MODIFY `id` int(20) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT for table `street_mask_points`
--
ALTER TABLE `street_mask_points`
  MODIFY `mask_points_id` int(10) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=7;

--
-- Constraints for dumped tables
--

--
-- Constraints for table `configuration`
--
ALTER TABLE `configuration`
  ADD CONSTRAINT `configuration_ibfk_1` FOREIGN KEY (`mask_points_id`) REFERENCES `street_mask_points` (`mask_points_id`);
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
