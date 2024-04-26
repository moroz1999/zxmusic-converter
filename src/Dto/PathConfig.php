<?php

declare(strict_types=1);

namespace ZxMusic\Dto;

final readonly class PathConfig
{
    public function __construct(
        public string $uploadPath,
        public string $resultPath,
        public string $musicPath,
        public string $converterPath
    )
    {
    }
}
