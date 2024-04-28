<?php
declare(strict_types=1);

use DI\ContainerBuilder;
use ZxMusic\Controller\MusicController;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Factory\ConverterFactory;
use ZxMusic\Response\ResponseHandler;
use function DI\create;

require_once dirname(__DIR__) . '/vendor/autoload.php';

ini_set('memory_limit', '320M');
ini_set('max_execution_time', '1800');

$containerBuilder = new ContainerBuilder();
$containerBuilder->addDefinitions([
    PathConfig::class => static function () {
        $rootPath = dirname(__DIR__) . '/';
        return new PathConfig(
            uploadPath: $rootPath . 'uploads/',
            resultPath: $rootPath . 'result/',
            musicPath: $rootPath . 'public/music/',
            converterPath: $rootPath . 'binaries/zxtune/'
        );
    },
    ResponseHandler::class => create(ResponseHandler::class),
    ConverterFactory::class => static function ($container) {
        return new ConverterFactory(
            $container,
            [
                ZxMusic\Converter\Constants::ZXTUNE => ZxMusic\Converter\ZxTune::class,
                ZxMusic\Converter\Constants::ARKOS => ZxMusic\Converter\Arkos::class,
            ]);
    }
]);

$container = $containerBuilder->build();

/** @var MusicController $controller */
$controller = $container->get(MusicController::class);
$controller->upload($_POST, $_FILES);
