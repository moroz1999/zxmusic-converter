<?php
ini_set("memory_limit", "320M");
ini_set("max_execution_time", "1800");

define('ROOT_PATH', realpath(dirname(__FILE__) . '/../'));
const UPLOADS_PATH = ROOT_PATH . 'uploads/';
const RESULT_PATH = ROOT_PATH . 'result/';
const CONVERTER_PATH = ROOT_PATH . 'binaries/zxtune/';
const MUSIC_PATH = ROOT_PATH . 'htdocs/music/';

include_once('../src/run.php');
