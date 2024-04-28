<?php

declare(strict_types=1);

namespace ZxMusic;

use RuntimeException;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Dto\ConversionConfig;

readonly class Converter
{
    public function __construct(
        private PathConfig $pathConfig
    )
    {

    }

    public function convert(ConversionConfig $config): array
    {
        $this->prepareDirectories($config->resultPath);

        $result = [];

        if (is_file($config->originalFilePath)) {
            $originalBaseName = basename($config->originalFilePath);

            $convertedType = 'mp3';
            /**
             * @var string[] $output
             */
            $output = [];
            $command = sprintf(
                '%s --quiet --core-options aym.interpolation=2,aym.clockrate=%d,aym.type=%d,aym.layout=%d --frameduration=%d --%s filename="%s%s_[Subpath]",bitrate=320 "%s" 2>&1',
                escapeshellcmd($this->pathConfig->converterPath . 'zxtune123.exe'),
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
            $result = $this->parseInfo($output, $originalBaseName);
            $this->moveGeneratedFiles($result, $config);
            $this->cleanupGeneratedFiles($config->resultPath, $config->baseName);
        }
        return $result;
    }

    private function prepareDirectories(string $resultPath): void
    {
        if (!is_dir($resultPath) && !mkdir($resultPath, 0777, true) && !is_dir($resultPath)) {
            throw new RuntimeException(sprintf('Directory "%s" was not created', $resultPath));
        }
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
                $info['mp3Name'] = $baseName . (isset($matches[2]) ? str_ireplace(['/', '#'], ['_', '_'], $matches[2]) : '') . '.mp3';
                $info['convertedFile'] = (isset($matches[0]) ? str_ireplace('?', '_', $matches[0]) : '') . '.mp3';
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

    /**
     * @param ConversionResult[] $result
     * @return void
     */
    private function moveGeneratedFiles(array $result, ConversionConfig $config): void
    {
        foreach ($result as $item) {
            $resultPathFile = $config->resultPath . $item->convertedFile;
            if (is_file($resultPathFile)) {
                $newPath = $this->pathConfig->musicPath . $item->mp3Name;
                rename($resultPathFile, $newPath);
            }
        }
    }

    private function cleanupGeneratedFiles(string $resultPath, string $baseName): void
    {
        $pattern = $resultPath . $baseName . '*'; // Assume generated files follow some naming convention
        array_map('unlink', glob($pattern));
        rmdir($resultPath);
    }
}