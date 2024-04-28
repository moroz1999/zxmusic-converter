<?php

declare(strict_types=1);

namespace ZxMusic\Controller;

use JsonException;
use RuntimeException;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Response\ResponseHandler;
use ZxMusic\Converter;

readonly class MusicController
{
    public function __construct(
        private Converter       $converter,
        private ResponseHandler $responseHandler,
        private PathConfig      $pathConfig
    )
    {
    }

    /**
     * @throws JsonException
     */
    public function upload(array $postData, array $fileData): void
    {
        $id = (int)($postData['id'] ?? 0);
        if ($id === 0) {
            $this->responseHandler->sendError('ID is required');
            return;
        }

        $originalFile = isset($fileData['original']['tmp_name']) ? (string)$fileData['original']['tmp_name'] : null;

        if ($originalFile === null || !is_uploaded_file($originalFile)) {
            $this->responseHandler->sendError('Invalid file uploaded');
            return;
        }
        $config = $this->createConfig($postData, $id);

        $uploadPath = $this->pathConfig->uploadPath . $id . '/';

        if (!is_dir($uploadPath) && !mkdir($uploadPath) && !is_dir($uploadPath)) {
            throw new RuntimeException(sprintf('Directory "%s" was not created', $uploadPath));
        }

        move_uploaded_file($originalFile, $config->originalFilePath);

        $result = $this->converter->convert($config);

        $this->responseHandler->sendSuccess($result);
    }

    private function createConfig(array $postData, int $id): ConversionConfig
    {
        $resultPath = $this->pathConfig->resultPath . $id . '/';
        $baseName = (string)($postData['baseName'] ?? 'default_name');
        $originalFilePath = $this->pathConfig->uploadPath . $id . '/' . $baseName;

        return new ConversionConfig(
            originalFilePath: $originalFilePath,
            baseName: $baseName,
            channels: (int)($postData['channels'] ?? 1),
            chipType: (int)($postData['chipType'] ?? 0),
            frequency: (int)($postData['frequency'] ?? 1750000),
            frameDuration: (int)($postData['frameDuration'] ?? 20000),
            resultPath: $resultPath
        );
    }
}
