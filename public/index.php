<?php
declare(strict_types=1);

use ZxMusic\Controller\MusicController;
use ZxMusic\Dto\PathConfig;

require_once dirname(__DIR__) . '/vendor/autoload.php';

ini_set('memory_limit', '320M');
ini_set('max_execution_time', '1800');

$rootPath = dirname(__DIR__) . '/';
$pathConfig = new PathConfig(
    uploadPath: $rootPath . 'uploads/',
    resultPath: $rootPath . 'result/',
    musicPath: $rootPath . 'public/music/',
    converterPath: $rootPath . 'binaries/zxtune/'
);

$controller = new MusicController($pathConfig);

try {
    $result = $controller->upload($_POST, $_FILES);
    echo json_encode(['success' => true, 'data' => $result]);
} catch (Exception $e) {
    echo json_encode(['success' => false, 'message' => $e->getMessage()]);
}
