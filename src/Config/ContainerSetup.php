<?php

declare(strict_types=1);

namespace ZxMusic\Config;

use DI\Container;
use DI\ContainerBuilder;
use Exception;
use Psr\Container\ContainerInterface;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Factory\ConverterFactory;
use ZxMusic\Response\ResponseHandler;
use ZxMusic\Service\Arkos\AksInformationParser;
use ZxMusic\Service\Arkos\Arkos1Converter;
use ZxMusic\Service\Arkos\Arkos2Converter;
use ZxMusic\Service\ChipNSfx\ChipNSfxConverter;
use ZxMusic\Service\Converter\ConverterType;
use ZxMusic\Service\FfmpegConverter\FfmpegConverter;
use ZxMusic\Service\Furnace\FurnaceConverter;
use ZxMusic\Service\ZxTune\ZxTuneConverter;
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
            ZxTuneConverter::class => static function () use ($rootPath) {
                return new ZxTuneConverter(
                    $rootPath . 'binaries/zxtune/zxtune123.exe'
                );
            },
            FurnaceConverter::class => static function (ContainerInterface $container) use ($rootPath) {
                return new FurnaceConverter(
                    $rootPath . 'binaries/furnace/furnace.exe',
                    $container->get(FfmpegConverter::class),
                );
            },
            Arkos1Converter::class => static function (ContainerInterface $container) use ($rootPath) {
                return new Arkos1Converter(
                    $rootPath . 'binaries/arkos1/Tools/AKSToYM.exe',
                    $container->get(ZxTuneConverter::class),
                    $container->get(AksInformationParser::class),
                );
            },
            Arkos2Converter::class => static function (ContainerInterface $container) use ($rootPath) {
                return new Arkos2Converter(
                    $rootPath . 'binaries/arkos2/tools/SongToWav.exe',
                    $container->get(FfmpegConverter::class),
                    $container->get(AksInformationParser::class),
                );
            },
            ChipNSfxConverter::class => static function (ContainerInterface $container) use ($rootPath) {
                return new ChipNSfxConverter(
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
                        ConverterType::ZXTUNE->value => ZxTuneConverter::class,
                        ConverterType::ARKOS1->value => Arkos1Converter::class,
                        ConverterType::ARKOS2->value => Arkos2Converter::class,
                        ConverterType::FURNACE->value => FurnaceConverter::class,
                        ConverterType::CHIPNSFX->value => ChipNSfxConverter::class,
                    ]
                );
            }
        ]);

        return $containerBuilder->build();
    }
}
