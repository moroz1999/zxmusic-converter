<?php
declare(strict_types=1);

namespace ZxMusic\Converter;

use Exception;
use RuntimeException;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Service\ConverterInterface;

readonly class Furnace implements ConverterInterface
{
    public function __construct(
        private string          $converterPath,
        private FfmpegConverter $ffmpegConverter,
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
            $wavName = $config->baseName . '.wav';
            $wavPath = $config->resultDir . $wavName;
            $command = sprintf(
                '%s -output %s %s 2>&1',
                escapeshellcmd($this->converterPath),
                escapeshellarg($wavPath),
                escapeshellarg($config->originalFilePath),
            );
            exec($command);
            if (!is_file($wavPath)) {
                throw new RuntimeException("Could not produce wave file {$wavName}");
            }

            $mp3Name = $config->baseName . '.mp3';
            $mp3Path = $config->resultDir . $mp3Name;
            $this->ffmpegConverter->convertToMp3($wavPath, $mp3Path);
            if (!is_file($mp3Path)) {
                throw new RuntimeException("Mp3 file is missing {$mp3Name}");
            }
            $result = new ConversionResult(
                mp3Name: $mp3Name,
                convertedFile: $mp3Name,
                title: '',
                author: '',
                time: '',
                channels: '3',
                type: 'AY',
                container: 'FUR',
                program: 'Furnace 0.6.*',
            );
            $results[] = $result;
        }

        return $results;
    }
}