<?php
declare(strict_types=1);

use DI\ContainerBuilder;
use ZxMusic\Controller\MusicController;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Response\ResponseHandler;
use function DI\create;

require_once dirname(__DIR__) . '/vendor/autoload.php';

ini_set('memory_limit', '320M');
ini_set('max_execution_time', '1800');

$containerBuilder = new ContainerBuilder();
$containerBuilder->addDefinitions([
    PathConfig::class => function () {
        $rootPath = dirname(__DIR__) . '/';
        return new PathConfig(
            uploadPath: $rootPath . 'uploads/',
            resultPath: $rootPath . 'result/',
            musicPath: $rootPath . 'public/music/',
            converterPath: $rootPath . 'binaries/zxtune/'
        );
    },
    ResponseHandler::class => create(ResponseHandler::class),
]);

$container = $containerBuilder->build();

/** @var MusicController $controller */
$controller = $container->get(MusicController::class);
$controller->upload($_POST, $_FILES);
