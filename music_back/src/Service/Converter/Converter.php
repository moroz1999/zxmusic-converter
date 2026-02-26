<?php

declare(strict_types=1);

namespace ZxMusic\Service\Converter;

use Psr\Container\ContainerExceptionInterface;
use Psr\Container\NotFoundExceptionInterface;
use Psr\Log\LoggerInterface;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Exception\ConversionException;
use ZxMusic\Factory\ConverterFactory;
use ZxMusic\Service\Directories;

readonly class Converter
{
    public function __construct(
        private PathConfig            $pathConfig,
        private Directories           $directories,
        private ConverterFactory      $converterFactory,
        private ConverterTypeResolver $converterTypeResolver,
        private LoggerInterface       $logger,
    )
    {
    }

    /**
     * @return ConversionResult[]
     * @throws ContainerExceptionInterface
     * @throws NotFoundExceptionInterface
     * @throws ConversionException
     */
    public function convert(ConversionConfig $config): array
    {
        $converterType = $this->converterTypeResolver->resolve($config->originalFilePath);
        $this->logger->info('Converting file', [
            'file' => basename($config->originalFilePath),
            'type' => $converterType->name,
        ]);

        $converter = $this->converterFactory->getConverter($converterType);
        $this->directories->prepareDirectory($config->resultDir);

        $result = $converter->convert($config);

        if (empty($result)) {
            throw new ConversionException(
                'Conversion produced no results for: ' . basename($config->originalFilePath)
            );
        }

        $this->moveGeneratedFiles($result, $config);
        $this->cleanupGeneratedFiles($config->resultDir, $config->baseName);

        $this->logger->info('Conversion succeeded', ['count' => count($result)]);

        return $result;
    }


    /**
     * @param ConversionResult[] $result
     * @param ConversionConfig $config
     * @return void
     */
    private function moveGeneratedFiles(array $result, ConversionConfig $config): void
    {
        foreach ($result as $item) {
            $resultPathFile = $config->resultDir . $item->convertedFile;
            $newPath = $this->pathConfig->musicPath . $item->mp3Name;

            $this->logger->debug('Moving converted file', [
                'from'   => $resultPathFile,
                'to'     => $newPath,
                'exists' => is_file($resultPathFile),
            ]);

            if (is_file($resultPathFile)) {
                rename($resultPathFile, $newPath);
            } else {
                $dirFiles = glob($config->resultDir . '*') ?: [];
                $this->logger->error('Converted file not found, cannot move', [
                    'expected'  => $resultPathFile,
                    'resultDir' => $config->resultDir,
                    'dirFiles'  => array_map('basename', $dirFiles),
                ]);
            }
        }
    }

    private function cleanupGeneratedFiles(string $resultPath, string $baseName): void
    {
        $pattern = $resultPath . $baseName . '*';
        array_map('unlink', glob($pattern));
        rmdir($resultPath);
    }
}
