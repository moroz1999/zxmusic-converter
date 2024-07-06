<?php
declare(strict_types=1);

namespace ZxMusic\Service\Arkos;

use Exception;
use RuntimeException;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Service\Converter\ConverterInterface;
use ZxMusic\Service\ZxTune\ZxTuneConverter;

readonly class Arkos1Converter implements ConverterInterface
{
    public function __construct(
        private string          $converterPath,
        private ZxTuneConverter $zxTuneConverter,
    )
    {
    }

    /**
     * @throws RuntimeException
     */
    public function convert(ConversionConfig $config): array
    {
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

            $zxTuneConfig = new ConversionConfig(
                $config->originalDir,
                $config->originalFilePath,
                $config->baseName,
                $config->channels,
                $config->chipType,
                $config->frequency,
                $config->frameDuration,
                $config->resultDir
            );

            return $this->zxTuneConverter->convert($zxTuneConfig);
        }

        throw new RuntimeException("File not found {$config->originalFilePath}");

    }
}