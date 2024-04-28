<?php

declare(strict_types=1);

namespace ZxMusic\Dto;

final readonly class ConversionConfig
{
    public function __construct(
        public string $originalFilePath,
        public string $baseName,
        public int    $channels,
        public int    $chipType,
        public int    $frequency,
        public int    $frameDuration,
        public string $resultDir
    )
    {
    }
}
