<?php
declare(strict_types=1);

namespace ZxMusic\Converter;

use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Service\ConverterInterface;

readonly class ZxTune implements ConverterInterface
{
    public function __construct(
        private string $converterPath,
    )
    {
    }

    public function convert(ConversionConfig $config): array
    {
        $result = [];

        if (is_file($config->originalFilePath)) {
            $originalBaseName = basename($config->originalFilePath);

            $convertedType = 'mp3';

            $output = [];
            $command = sprintf(
                '%s --quiet --core-options aym.interpolation=2,aym.clockrate=%d,aym.type=%d,aym.layout=%d --frameduration=%d --%s filename="%s%s_[Subpath]",bitrate=320 "%s" 2>&1',
                escapeshellcmd($this->converterPath),
                $config->frequency,
                $config->chipType,
                $config->channels,
                $config->frameDuration,
                $convertedType,
                escapeshellarg($config->resultDir),
                $config->baseName,
                escapeshellarg($config->originalFilePath)
            );
            exec($command, $output);
            /**
             * @var string[] $output
             */
            $result = $this->parseInfo($output, $originalBaseName);
        }
        return $result;
    }


    /**
     * @param string[] $output
     * @param string $baseName
     * @return ConversionResult[]
     */
    private function parseInfo(array $output, string $baseName): array
    {
        $results = [];
        $info = [];

        foreach ($output as $line) {
            $pattern = '#' . preg_quote($baseName, '#') . '(\?(.*))*#';
            if (preg_match($pattern, $line, $matches)) {
                if (!empty($info)) {
                    $results[] = $this->createConversionResult($info);
                    $info = [];
                }
                $fileName = pathinfo($baseName, PATHINFO_FILENAME);
                $info['mp3Name'] = $fileName . (isset($matches[2]) ? str_ireplace(['/', '#'], ['_', '_'], $matches[2]) : '') . '.mp3';
                $info['convertedFile'] = $fileName . '_' . (isset($matches[2]) ? str_ireplace('/', '_', $matches[2]) : '') . '.mp3';
            }
            $this->fillInfoFromLine($line, $info);
        }
        if (!empty($info)) {
            $results[] = $this->createConversionResult($info);
        }

        return $results;
    }

    private function createConversionResult(array $info): ConversionResult
    {
        return new ConversionResult(
            mp3Name: (string)($info['mp3Name'] ?? ''),
            convertedFile: (string)($info['convertedFile'] ?? ''),
            title: (string)($info['title'] ?? ''),
            author: (string)($info['author'] ?? ''),
            time: (string)($info['time'] ?? ''),
            channels: (string)($info['channels'] ?? ''),
            type: (string)($info['type'] ?? ''),
            container: (string)($info['container'] ?? ''),
            program: (string)($info['program'] ?? ''),
        );
    }

    private function fillInfoFromLine(string $line, array &$info): void
    {
        $patterns = [
            'title' => '#Title:\s*(.*)#',
            'author' => '#Author:\s*(.*)#',
            'time' => '#Time:\s*([^\t]*)#',
            'channels' => '#Channels:\s*(.*)#',
            'type' => '#Type:\s*([^\t]*)#',
            'container' => '#Container:\s*([^\t]*)#',
            'program' => '#Program:\s*(.*)#'
        ];

        foreach ($patterns as $key => $pattern) {
            if (preg_match($pattern, $line, $matches)) {
                $info[$key] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
        }
    }

}