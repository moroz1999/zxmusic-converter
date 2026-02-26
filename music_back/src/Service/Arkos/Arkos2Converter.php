<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Exception\ConversionException;
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
     * @return ConversionResult[]
     * @throws ConversionException
     */
    public function convert(ConversionConfig $config): array
    {
        if (!is_file($config->originalFilePath)) {
            throw new ConversionException('Input file not found: ' . $config->originalFilePath);
        }

        $wavName = $config->baseName . '.wav';
        $wavPath = $config->resultDir . $wavName;
        $output = [];
        $command = sprintf(
            '%s %s %s 2>&1',
            escapeshellcmd($this->converterPath),
            escapeshellarg($config->originalFilePath),
            escapeshellarg($wavPath),
        );
        exec($command, $output);

        if (!is_file($wavPath)) {
            throw new ConversionException(
                'SongToWav failed to produce WAV file: ' . $wavName . PHP_EOL . implode(PHP_EOL, $output)
            );
        }

        $mp3Name = $config->baseName . '.mp3';
        $mp3Path = $config->resultDir . $mp3Name;
        $this->ffmpegConverter->convertToMp3($wavPath, $mp3Path);

        $info = $this->aksInformationParser->getAksInformation($config->originalFilePath);

        return [new ConversionResult(
            mp3Name: $mp3Name,
            convertedFile: $mp3Name,
            title: $info->title,
            author: $info->author,
            time: '',
            channels: '3',
            type: 'AKS',
            container: 'AKS',
            program: 'Arkos Tracker 2.*',
        )];
    }
}
