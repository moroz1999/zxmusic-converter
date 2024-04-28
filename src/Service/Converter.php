<?php

declare(strict_types=1);

namespace ZxMusic\Service;

use Psr\Container\ContainerExceptionInterface;
use Psr\Container\NotFoundExceptionInterface;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Dto\PathConfig;
use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Factory\ConverterFactory;

readonly class Converter
{
    public function __construct(
        private PathConfig       $pathConfig,
        private Directories      $directories,
        private ConverterFactory $converterFactory,
    )
    {
    }

    /**
     * @throws ContainerExceptionInterface
     * @throws NotFoundExceptionInterface
     */
    public function convert(ConversionConfig $config): array
    {
        $converterType = ConverterTypeResolver::resolve($config->originalFilePath);
        $converter = $this->converterFactory->getConverter($converterType);
        $this->directories->prepareDirectory($config->resultDir);

        $result = $converter->convert($config);

        $this->moveGeneratedFiles($result, $config);
        $this->cleanupGeneratedFiles($config->resultDir, $config->baseName);

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