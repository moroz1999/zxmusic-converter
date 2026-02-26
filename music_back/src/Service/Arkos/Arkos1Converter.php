<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Exception\ConversionException;
use ZxMusic\Service\Converter\ConverterInterface;
use ZxMusic\Service\ZxTune\ZxTuneConverter;

readonly class Arkos1Converter implements ConverterInterface
{
    public function __construct(
        private string               $converterPath,
        private ZxTuneConverter      $zxTuneConverter,
        private AksInformationParser $aksInformationParser,
    )
    {
    }

    /**
     * @return ConversionResult[]
     * @throws ConversionException
     */
    public function convert(ConversionConfig $config): array
    {
        if (!is_file($config->originalFilePath)) {
            throw new ConversionException('Input file not found: ' . $config->originalFilePath);
        }

        $ymName = $config->baseName . '.ym';
        $ymPath = $config->resultDir . $ymName;
        $output = [];
        $command = sprintf(
            '%s %s %s 2>&1',
            escapeshellcmd($this->converterPath),
            escapeshellarg($config->originalFilePath),
            escapeshellarg($ymPath),
        );
        exec($command, $output);

        if (!is_file($ymPath)) {
            throw new ConversionException(
                'AKSToYM failed to produce YM file: ' . $ymName . PHP_EOL . implode(PHP_EOL, $output)
            );
        }

        $info = $this->aksInformationParser->getAksInformation($config->originalFilePath);

        $zxTuneConfig = new ConversionConfig(
            $config->originalDir,
            $ymPath,
            $config->baseName,
            $config->channels,
            $config->chipType,
            $info->frequency,
            $config->frameDuration,
            $config->resultDir
        );

        $zxTuneResults = $this->zxTuneConverter->convert($zxTuneConfig);
        $firstResult = reset($zxTuneResults);

        return [new ConversionResult(
            mp3Name: $firstResult->mp3Name,
            convertedFile: $firstResult->convertedFile,
            title: $info->title,
            author: $info->author,
            time: $firstResult->time,
            channels: $firstResult->channels,
            type: 'AKS',
            container: 'AKS',
            program: $info->trackerVersion,
        )];
    }
}
