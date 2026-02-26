<?php

declare(strict_types=1);

namespace ZxMusic\Controller;

use JsonException;
use Psr\Log\LoggerInterface;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Exception\ConversionException;
use ZxMusic\Exception\UploadException;
use ZxMusic\Response\ResponseHandler;
use ZxMusic\Service\Converter\Converter;
use ZxMusic\Service\Directories;

readonly class MusicController
{
    public function __construct(
        private Converter       $converter,
        private Directories     $directories,
        private ResponseHandler $responseHandler,
        private PathConfig      $pathConfig,
        private LoggerInterface $logger,
    )
    {
    }

    /**
     * @throws JsonException
     */
    public function upload(array $postData, array $fileData): void
    {
        try {
            $id = (int)($postData['id'] ?? 0);
            if ($id === 0) {
                throw new UploadException('ID is required');
            }

            $tmpFilePath = isset($fileData['original']['tmp_name']) ? (string)$fileData['original']['tmp_name'] : null;
            $originalFilename = isset($fileData['original']['name']) ? (string)$fileData['original']['name'] : null;

            if ($tmpFilePath === null || $originalFilename === null || !is_uploaded_file($tmpFilePath)) {
                throw new UploadException('Invalid file uploaded');
            }

            $config = $this->createConfig($postData, $originalFilename, $id);

            $this->directories->prepareDirectory($config->originalDir);
            $this->directories->prepareDirectory($config->resultDir);

            if (!move_uploaded_file($tmpFilePath, $config->originalFilePath)) {
                throw new UploadException('Failed to save uploaded file');
            }

            $result = $this->converter->convert($config);
            $this->responseHandler->sendSuccess($result);
        } catch (UploadException $exception) {
            $this->responseHandler->sendError($exception->getMessage());
        } catch (ConversionException $exception) {
            $this->logger->error($exception->getMessage(), ['exception' => $exception]);
            $this->responseHandler->sendError($exception->getMessage(), 500);
        }
    }

    private function createConfig(
        array  $postData,
        string $uploadedFileName,
        int    $id
    ): ConversionConfig
    {
        $extension = strtolower(pathinfo($uploadedFileName, PATHINFO_EXTENSION));
        if (!$extension) {
            $extension = 'tmp';
        }

        $resultPath = $this->pathConfig->resultPath . $id . DIRECTORY_SEPARATOR;
        $baseName = (string)($postData['baseName'] ?? 'default_name');
        $baseName = basename($baseName);

        $originalDir = $this->pathConfig->uploadPath . $id . DIRECTORY_SEPARATOR;
        $originalFilePath = $originalDir . $baseName . ".{$extension}";

        return new ConversionConfig(
            originalDir: $originalDir,
            originalFilePath: $originalFilePath,
            baseName: $baseName,
            channels: (int)($postData['channels'] ?? 0),
            chipType: (int)($postData['chipType'] ?? 0),
            frequency: (int)($postData['frequency'] ?? 1770000),
            frameDuration: (int)($postData['frameDuration'] ?? 20000),
            resultDir: $resultPath,
        );
    }
}
