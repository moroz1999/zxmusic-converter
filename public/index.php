<?php
declare(strict_types=1);

use ZxMusic\Config\ContainerSetup;
use ZxMusic\Controller\MusicController;

require_once dirname(__DIR__) . '/vendor/autoload.php';

ini_set('memory_limit', '320M');
ini_set('max_execution_time', '1800');

$rootPath = dirname(__DIR__) . '/';
$container = ContainerSetup::setup($rootPath);

/** @var MusicController $controller */
$controller = $container->get(MusicController::class);
$controller->upload($_POST, $_FILES);