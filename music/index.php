<?php
declare(strict_types=1);

use Psr\Log\LoggerInterface;
use ZxMusic\Config\ContainerSetup;
use ZxMusic\Controller\MusicController;

require_once dirname(__DIR__) . '/music_back/vendor/autoload.php';

ini_set('display_errors', '0');
ini_set('memory_limit', '320M');
ini_set('max_execution_time', '1800');

$rootPath = dirname(__DIR__) . DIRECTORY_SEPARATOR . 'music_back' . DIRECTORY_SEPARATOR;
$container = ContainerSetup::setup($rootPath);

/** @var LoggerInterface $logger */
$logger = $container->get(LoggerInterface::class);

set_error_handler(static function (int $errno, string $errstr, string $errfile, int $errline) use ($logger): bool {
    $logger->warning('PHP error', [
        'errno'   => $errno,
        'message' => $errstr,
        'file'    => $errfile,
        'line'    => $errline,
    ]);
    return true;
});

/** @var MusicController $controller */
$controller = $container->get(MusicController::class);

try {
    $controller->upload($_POST, $_FILES);
} catch (Throwable $exception) {
    /** @var LoggerInterface $logger */
    $logger = $container->get(LoggerInterface::class);
    $logger->critical('Unhandled exception', [
        'message' => $exception->getMessage(),
        'exception' => $exception,
    ]);
    http_response_code(500);
    header('Content-Type: application/json');
    echo json_encode(['success' => false, 'data' => [], 'error' => 'Internal server error']);
}
