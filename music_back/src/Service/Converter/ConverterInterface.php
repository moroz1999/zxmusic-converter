<?php
declare(strict_types=1);

namespace ZxMusic\Service\Converter;

use ZxMusic\Dto\ConversionConfig;
use ZxMusic\Dto\ConversionResult;

interface ConverterInterface
{
    /**
     * @param ConversionConfig $config
     * @return ConversionResult[]
     */
    public function convert(ConversionConfig $config): array;
}