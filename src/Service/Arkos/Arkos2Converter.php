<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

use Exception;
use RuntimeException;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Service\Converter\ConverterInterface;
use ZxMusic\Service\FfmpegConverter\FfmpegConverter;

readonly class Arkos2Converter implements ConverterInterface
{
    public function __construct(
        private string               $converterPath,
        private FfmpegConverter      $ffmpegConverter,
        private AksInformationParser $aksInformationParser,
    )
    {
    }

    /**
     * @throws RuntimeException|Exception
     */
    public function convert(ConversionConfig $config): array
    {
        $results = [];

        if (is_file($config->originalFilePath)) {
            $wavName = $config->baseName . '.wav';
            $wavPath = $config->resultDir . $wavName;
            $command = sprintf(
                '%s %s %s 2>&1',
                escapeshellcmd($this->converterPath),
                escapeshellarg($config->originalFilePath),
                escapeshellarg($wavPath),
            );
            exec($command);
            if (!is_file($wavPath)) {
                throw new RuntimeException("Could not produce wave file {$wavName}");
            }
            $info = $this->aksInformationParser->getAksInformation($config->originalFilePath);

            $mp3Name = $config->baseName . '.mp3';
            $mp3Path = $config->resultDir . $mp3Name;
            $this->ffmpegConverter->convertToMp3($wavPath, $mp3Path);
            if (!is_file($mp3Path)) {
                throw new RuntimeException("Mp3 file is missing {$mp3Name}");
            }
            $result = new ConversionResult(
                mp3Name: $mp3Name,
                convertedFile: $mp3Name,
                title: $info->title,
                author: $info->author,
                time: '',
                channels: '3',
                type: 'AKS',
                container: 'AKS',
                program: 'Arkos Tracker 2.*',
            );
            $results[] = $result;
        }

        return $results;
    }
}