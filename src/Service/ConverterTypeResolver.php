<?php
declare(strict_types=1);

namespace ZxMusic\Service;

use InvalidArgumentException;

readonly final class ConverterTypeResolver
{
    /**
     * Resolves the converter type based on the file extension.
     *
     * @param string $filePath The file path.
     * @return ConverterType The converter type.
     * @throws InvalidArgumentException If the file extension is not supported.
     */
    public static function resolve(string $filePath): ConverterType
    {
        $extension = strtolower(pathinfo($filePath, PATHINFO_EXTENSION));

        return match ($extension) {
            'fur' => ConverterType::FURNACE,
            'aks' => ConverterType::ARKOS,
            'chp' => ConverterType::CHIPNSFX,
            default => ConverterType::ZXTUNE
        };
    }
}