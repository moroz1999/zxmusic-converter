<?php

declare(strict_types=1);

namespace ZxMusic\Controller;

use ZxMusic\Dto\PathConfig;
use ZxMusic\Converter;
use ZxMusic\Dto\ConversionConfig;
use Exception;

class MusicController
{
    private Converter $converter;
    private PathConfig $pathConfig;

    public function __construct(PathConfig $pathConfig)
    {
        $this->pathConfig = $pathConfig;
        $this->converter = new Converter($pathConfig->converterPath);
    }

    public function upload(array $postData, array $fileData): array
    {
        $id = intval($postData['id'] ?? 0);
        $channels = intval($postData['channels'] ?? 1);
        $chipType = intval($postData['chipType'] ?? 0);
        $frequency = intval($postData['frequency'] ?? 1750000);
        $frameDuration = intval($postData['frameDuration'] ?? 20000);
        $baseName = $postData['baseName'] ?? 'default_name';

        $originalFile = $fileData['original']['tmp_name'] ?? null;
        if (!$originalFile || !is_uploaded_file($originalFile)) {
            throw new Exception("No file uploaded or file is not valid.");
        }

        $originalFilePath = $this->pathConfig->uploadPath . $id . '/originalfile';
        $resultPath = $this->pathConfig->resultPath . $id . '/';

        $this->prepareDirectories([$this->pathConfig->uploadPath . $id, $resultPath]);

        move_uploaded_file($originalFile, $originalFilePath);

        $config = new ConversionConfig(
            originalFilePath: $originalFilePath,
            baseName: $baseName,
            channels: $channels,
            chipType: $chipType,
            frequency: $frequency,
            frameDuration: $frameDuration,
            resultPath: $resultPath
        );

        $result = $this->converter->convert($config);

        $this->cleanup($originalFilePath, $resultPath);

        return $result;
    }

    private function prepareDirectories(array $paths): void
    {
        foreach ($paths as $path) {
            if (!is_dir($path)) {
                mkdir($path, 0777, true);
            }
        }
    }

    private function cleanup(string $originalFilePath, string $resultPath): void
    {
        unlink($originalFilePath);
        array_map('unlink', glob($resultPath . '*.*')); // Deletes all files in the result directory
        rmdir($resultPath); // Finally, remove the directory itself
    }
}
