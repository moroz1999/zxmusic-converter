<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

use Exception;
use RuntimeException;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
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
     * @throws RuntimeException
     */
    public function convert(ConversionConfig $config): array
    {
        $results = [];
        if (is_file($config->originalFilePath)) {
            $ymName = $config->baseName . '.ym';
            $ymPath = $config->resultDir . $ymName;
            $command = sprintf(
                '%s %s %s 2>&1',
                escapeshellcmd($this->converterPath),
                escapeshellarg($config->originalFilePath),
                escapeshellarg($ymPath),
            );
            $output = [];
            exec($command, $output);
            if (!is_file($ymPath)) {
                throw new RuntimeException("Could not produce YM file {$ymName}: " . implode($output));
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
            /**
             * @var ConversionResult $firstResult
             */
            $firstResult = reset($zxTuneResults);
            $result = new ConversionResult(
                mp3Name: $firstResult->mp3Name,
                convertedFile: $firstResult->convertedFile,
                title: $info->title,
                author: $info->author,
                time: $firstResult->time,
                channels: $firstResult->channels,
                type: 'AKS',
                container: 'AKS',
                program: 'Arkos Tracker 1.*',
            );
            $results[] = $result;
        }

        return $results;

    }
}