<?php
declare(strict_types=1);

namespace ZxMusic\Service\ZxTune;

use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Exception\ConversionException;
use ZxMusic\Service\Converter\ConverterInterface;
use function sprintf;

readonly class ZxTuneConverter implements ConverterInterface
{
    public function __construct(
        private string $converterPath,
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

        $originalBaseName = basename($config->originalFilePath);
        $output = [];
        $filenameArg = escapeshellarg('filename=' . $config->resultDir . $config->baseName . '_[Subpath],bitrate=320');
        $command = sprintf(
            '%s --quiet --core-options aym.interpolation=2,aym.clockrate=%d,aym.type=%d,aym.layout=%d --frameduration=%d --mp3 %s %s 2>&1',
            escapeshellcmd($this->converterPath),
            $config->frequency,
            $config->chipType,
            $config->channels,
            $config->frameDuration,
            $filenameArg,
            escapeshellarg($config->originalFilePath)
        );
        exec($command, $output);

        $result = $this->parseInfo($output, $originalBaseName);
        if (empty($result)) {
            throw new ConversionException(
                'ZxTune produced no results for: ' . $originalBaseName . PHP_EOL . implode(PHP_EOL, $output)
            );
        }

        return $result;
    }

    /**
     * @param string[] $output
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
            'title'     => '#Title:\s*(.*)#',
            'author'    => '#Author:\s*(.*)#',
            'time'      => '#Time:\s*([^\t]*)#',
            'channels'  => '#Channels:\s*(.*)#',
            'type'      => '#Type:\s*([^\t]*)#',
            'container' => '#Container:\s*([^\t]*)#',
            'program'   => '#Program:\s*(.*)#',
        ];

        foreach ($patterns as $key => $pattern) {
            if (preg_match($pattern, $line, $matches)) {
                $info[$key] = mb_convert_encoding($matches[1], 'Windows-1251', 'UTF-8');
            }
        }
    }
}
