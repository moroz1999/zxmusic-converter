<?php

declare(strict_types=1);

namespace ZxMusic;

use ZxMusic\Dto\ConversionConfig;

class Converter
{
    private string $converterPath;

    public function __construct(string $converterPath)
    {
        $this->converterPath = $converterPath;
    }

    public function convert(ConversionConfig $config): array
    {
        $types = ['mp3'];
        $result = [];

        if (is_file($config->originalFilePath)) {
            foreach ($types as $convertedType) {
                $output = [];
                $command = sprintf(
                    '%s --quiet --core-options aym.interpolation=2,aym.clockrate=%d,aym.type=%d,aym.layout=%d --frameduration=%d --%s filename="%s%s_[Subpath]",bitrate=320 "%s" 2>&1',
                    escapeshellcmd($this->converterPath . 'zxtune123.exe'),
                    $config->frequency,
                    $config->chipType,
                    $config->channels,
                    $config->frameDuration,
                    $convertedType,
                    escapeshellarg($config->resultPath),
                    $config->baseName,
                    escapeshellarg($config->originalFilePath)
                );
                exec($command, $output);
                $result = array_merge($result, $this->parseInfo($output, $config->resultPath, $config->baseName));
            }
        }
        return $result;
    }

    private function parseInfo(array $output, string $resultPath, string $baseName): array
    {
        $infoList = [];

        foreach ($output as $line) {
            if (preg_match('#\[' . preg_quote($baseName, '#') . '\]_(.*)#', $line, $matches)) {
                $info = [
                    'convertedFile' => $resultPath . $matches[1],
                    'details' => $this->extractDetails($line)
                ];
                $infoList[] = $info;
            }
        }

        return $infoList;
    }

    private function extractDetails(string $line): array
    {
        $details = [];
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
                $details[$key] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
        }

        return $details;
    }
}