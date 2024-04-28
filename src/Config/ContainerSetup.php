<?php

declare(strict_types=1);

namespace ZxMusic\Config;

use DI\Container;
use DI\ContainerBuilder;
use Exception;
use Psr\Container\ContainerInterface;
use ZxMusic\Converter\Arkos;
use ZxMusic\Converter\ChipNSfx;
use ZxMusic\Converter\FfmpegConverter;
use ZxMusic\Converter\Furnace;
use ZxMusic\Converter\ZxTune;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Factory\ConverterFactory;
use ZxMusic\Response\ResponseHandler;
use ZxMusic\Service\ConverterType;
use function DI\create;

class ContainerSetup
{
    /**
     * @throws Exception
     */
    public static function setup(string $rootPath): Container
    {
        $containerBuilder = new ContainerBuilder();
        $containerBuilder->addDefinitions([
            PathConfig::class => static function () use ($rootPath) {
                return new PathConfig(
                    uploadPath: $rootPath . 'uploads' . DIRECTORY_SEPARATOR,
                    resultPath: $rootPath . 'result' . DIRECTORY_SEPARATOR,
                    musicPath: $rootPath . 'public' . DIRECTORY_SEPARATOR . 'music' . DIRECTORY_SEPARATOR,
                );
            },
            ZxTune::class => static function () use ($rootPath) {
                return new ZxTune(
                    $rootPath . 'binaries/zxtune/zxtune123.exe'
                );
            },
            Furnace::class => static function (ContainerInterface $container) use ($rootPath) {
                return new Furnace(
                    $rootPath . 'binaries/furnace/furnace.exe',
                    $container->get(FfmpegConverter::class),
                );
            },
            Arkos::class => static function (ContainerInterface $container) use ($rootPath) {
                return new Arkos(
                    $rootPath . 'binaries/arkos/tools/SongToWav.exe',
                    $container->get(FfmpegConverter::class),
                );
            },
            ChipNSfx::class => static function (ContainerInterface $container) use ($rootPath) {
                return new ChipNSfx(
                    $rootPath . 'binaries/chipnsfx/CHIPNSFX.EXE',
                    $container->get(FfmpegConverter::class),
                );
            },
            FfmpegConverter::class => static function () use ($rootPath) {
                return new FfmpegConverter(
                    $rootPath . 'binaries/ffmpeg/bin/ffmpeg.exe'
                );
            },
            ResponseHandler::class => create(ResponseHandler::class),
            ConverterFactory::class => static function (ContainerInterface $container) {
                return new ConverterFactory(
                    $container,
                    [
                        ConverterType::ZXTUNE->value => ZxTune::class,
                        ConverterType::ARKOS->value => Arkos::class,
                        ConverterType::FURNACE->value => Furnace::class,
                        ConverterType::CHIPNSFX->value => ChipNSfx::class,
                    ]
                );
            }
        ]);

        return $containerBuilder->build();
    }
}
