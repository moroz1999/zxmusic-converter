<?php
declare(strict_types=1);

namespace ZxMusic\Converter;

use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;
use ZxMusic\Dto\PathConfig;

readonly class Arkos implements ConverterInterface
{
    public function __construct(
        private PathConfig $pathConfig,
    )
    {
    }

    public function convert(ConversionConfig $config): array
    {
        $results = [];


        return $results;
    }
}